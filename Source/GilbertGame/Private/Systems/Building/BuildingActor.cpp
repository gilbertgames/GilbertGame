#include "Systems/Building/BuildingActor.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

ABuildingActor::ABuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetMobility(EComponentMobility::Static);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
}

#if WITH_EDITOR

// ------------------------------
// Editor widget location readback
// ------------------------------
bool ABuildingActor::TryGetTranslateWidgetWorldLocation(FVector& OutWorld) const
{
	if (!GEditor)
	{
		return false;
	}

	FViewport* ActiveViewport = GEditor->GetActiveViewport();
	if (!ActiveViewport)
	{
		return false;
	}

	FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(ActiveViewport->GetClient());
	if (!ViewportClient)
	{
		return false;
	}

	OutWorld = ViewportClient->GetWidgetLocation();
	return true;
}

// ------------------------------
// Gizmo axis -> direction axis lock
// ------------------------------
ABuildingActor::EDirAxisLock ABuildingActor::GetDirectionAxisLockFromGizmo() const
{
	if (!GEditor)
	{
		return EDirAxisLock::None;
	}

	FViewport* ActiveViewport = GEditor->GetActiveViewport();
	if (!ActiveViewport)
	{
		return EDirAxisLock::None;
	}

	FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(ActiveViewport->GetClient());
	if (!ViewportClient)
	{
		return EDirAxisLock::None;
	}

	const EAxisList::Type Axis = ViewportClient->GetCurrentWidgetAxis();

	// Translate gizmo intent:
	// - X or XZ => user sliding in X => wall should remain Y-facing (PosY/NegY)
	// - Y or YZ => user sliding in Y => wall should remain X-facing (PosX/NegX)
	// - XY / Screen / None => ambiguous/free => handled elsewhere (session latch + fallback)
	switch (Axis)
	{
	case EAxisList::X:
	case EAxisList::XZ:
		return EDirAxisLock::ForceYFacing;

	case EAxisList::Y:
	case EAxisList::YZ:
		return EDirAxisLock::ForceXFacing;

	case EAxisList::XY:
	case EAxisList::Screen:
	case EAxisList::None:
	default:
		return EDirAxisLock::None;
	}
}

ABuildingActor::EDirAxisLock ABuildingActor::GetFallbackAxisLockFromCurrentDir(EGridDirection CurrentDir) const
{
	const bool bIsX = (CurrentDir == EGridDirection::PosX || CurrentDir == EGridDirection::NegX);
	const bool bIsY = (CurrentDir == EGridDirection::PosY || CurrentDir == EGridDirection::NegY);

	if (bIsX) return EDirAxisLock::ForceXFacing;
	if (bIsY) return EDirAxisLock::ForceYFacing;
	return EDirAxisLock::None;
}

// ------------------------------
// Grid conversions
// ------------------------------
FIntVector ABuildingActor::WorldToCell(const FVector& World) const
{
	return FIntVector(
		FMath::RoundToInt(World.X / GridXY),
		FMath::RoundToInt(World.Y / GridXY),
		FMath::RoundToInt(World.Z / GridZ)
	);
}

FVector ABuildingActor::CellToWorld(const FIntVector& Cell) const
{
	return FVector(
		Cell.X * GridXY,
		Cell.Y * GridXY,
		Cell.Z * GridZ
	);
}

// ------------------------------
// Direction helpers
// ------------------------------
bool ABuildingActor::UsesDirection() const
{
	return GridSlot == EGridSlot::Wall;
}

EGridDirection ABuildingActor::ComputeDirectionFromRotation() const
{
	const FVector Fwd = GetActorForwardVector();
	const float AbsX = FMath::Abs(Fwd.X);
	const float AbsY = FMath::Abs(Fwd.Y);

	if (AbsX >= AbsY)
	{
		return (Fwd.X >= 0.f) ? EGridDirection::PosX : EGridDirection::NegX;
	}
	return (Fwd.Y >= 0.f) ? EGridDirection::PosY : EGridDirection::NegY;
}

FVector ABuildingActor::DirectionToNormal(EGridDirection Dir) const
{
	switch (Dir)
	{
	case EGridDirection::PosX: return FVector(1, 0, 0);
	case EGridDirection::NegX: return FVector(-1, 0, 0);
	case EGridDirection::PosY: return FVector(0, 1, 0);
	case EGridDirection::NegY: return FVector(0, -1, 0);
	default: return FVector::ZeroVector;
	}
}

FRotator ABuildingActor::DirectionToRotation(EGridDirection Dir) const
{
	const FVector N = DirectionToNormal(Dir);
	if (N.IsNearlyZero())
	{
		return GetActorRotation();
	}

	FRotator Rot = FRotationMatrix::MakeFromXZ(N, FVector::UpVector).Rotator();

	// Mesh forward-axis correction
	Rot.Yaw += 90.f;

	return Rot;
}

// ------------------------------
// Local vector for direction solving
// ------------------------------
FVector ABuildingActor::MakeLocalForDirection(const FVector& RawWorld, const FVector& CellCenter) const
{
	return RawWorld - CellCenter;
}

// ------------------------------
// Wall direction solve (axis lock + sign deadzone)
// ------------------------------
EGridDirection ABuildingActor::ComputeWallDirectionFromLocal(EGridDirection CurrentDir, const FVector& LocalForDir, EDirAxisLock AxisLock) const
{
	const float Fx = LocalForDir.X / GridXY;
	const float Fy = LocalForDir.Y / GridXY;

	const float AbsFx = FMath::Abs(Fx);
	const float AbsFy = FMath::Abs(Fy);

	const bool bCurrentIsX = (CurrentDir == EGridDirection::PosX || CurrentDir == EGridDirection::NegX);
	const bool bCurrentIsY = (CurrentDir == EGridDirection::PosY || CurrentDir == EGridDirection::NegY);

	const float AxisBiasFrac = DirectionBiasUnits / GridXY;
	const float FlipT = FMath::Clamp(FlipThresholdFrac, 0.0f, 0.49f);

	auto SolveSignX = [&]() -> EGridDirection
		{
			const bool bWantsPos = bCentered ? (Fx <= 0.f) : (Fx >= 0.f);

			if (bCurrentIsX)
			{
				const bool bCurrentPos = (CurrentDir == EGridDirection::PosX);
				if (bWantsPos != bCurrentPos && AbsFx < FlipT)
				{
					return CurrentDir;
				}
			}

			return bWantsPos ? EGridDirection::PosX : EGridDirection::NegX;
		};

	auto SolveSignY = [&]() -> EGridDirection
		{
			const bool bWantsPos = bCentered ? (Fy <= 0.f) : (Fy >= 0.f);

			if (bCurrentIsY)
			{
				const bool bCurrentPos = (CurrentDir == EGridDirection::PosY);
				if (bWantsPos != bCurrentPos && AbsFy < FlipT)
				{
					return CurrentDir;
				}
			}

			return bWantsPos ? EGridDirection::PosY : EGridDirection::NegY;
		};

	// 1) Respect axis lock when present.
	switch (AxisLock)
	{
	case EDirAxisLock::ForceXFacing: return SolveSignX();
	case EDirAxisLock::ForceYFacing: return SolveSignY();
	case EDirAxisLock::None:
	default:
		break;
	}

	// 2) Free/XY mode: choose axis by dominance, with stickiness.
	// Extra centered-mode stability: near the cell center, freeze the axis to avoid flicker.
	const float FreezeT = FMath::Clamp(CenterAxisFreezeFrac, 0.0f, 0.49f);

	// "Near center" means both axes are close to 0 in normalized tile space.
	const bool bNearCenter = (AbsFx < FreezeT) && (AbsFy < FreezeT);

	if (bCentered && bNearCenter)
	{
		// Freeze axis choice near centerline (keep current axis).
		if (bCurrentIsX) return SolveSignX();
		if (bCurrentIsY) return SolveSignY();

		// If we somehow don't have a valid current axis, fall through to dominance.
	}

	// 2) Free/XY mode: choose axis by dominance, with stronger stickiness toward current axis.
	bool bChooseX = (AbsFx >= AbsFy);

	// NEW: extra “stay on current axis” factor.
	// > 1.0 means you need the other axis to be that much stronger before switching.
	const float PreferCurrentAxis = 1.20f; // try 1.10 - 1.35

	if (bCurrentIsX)
	{
		// Only switch to Y if Y is clearly stronger than X (+ bias + multiplier)
		if (AbsFy > (AbsFx * PreferCurrentAxis) + AxisBiasFrac)
		{
			bChooseX = false;
		}
		else
		{
			bChooseX = true;
		}
	}
	else if (bCurrentIsY)
	{
		// Only switch to X if X is clearly stronger than Y (+ bias + multiplier)
		if (AbsFx > (AbsFy * PreferCurrentAxis) + AxisBiasFrac)
		{
			bChooseX = true;
		}
		else
		{
			bChooseX = false;
		}
	}
	else
	{
		// If current direction is None, fall back to simple dominance
		bChooseX = (AbsFx >= AbsFy);
	}

	return bChooseX ? SolveSignX() : SolveSignY();


}

// ------------------------------
// Offsets (explicit-state versions)
// ------------------------------
FVector ABuildingActor::ApplyHalfHeightOffset(const FVector& InLoc, bool bUseHalfHeight) const
{
	if (!bUseHalfHeight)
	{
		return InLoc;
	}

	FVector Out = InLoc;
	Out.Z += GridZ * 0.5f;
	return Out;
}

FVector ABuildingActor::RemoveHalfHeightOffset(const FVector& InLoc, bool bUseHalfHeight) const
{
	if (!bUseHalfHeight)
	{
		return InLoc;
	}

	FVector Out = InLoc;
	Out.Z -= GridZ * 0.5f;
	return Out;
}

FVector ABuildingActor::ApplyWallEdgeOffset(const FVector& InLoc, EGridDirection Dir, bool bUseCentered) const
{
	if (!UsesDirection() || bUseCentered)
	{
		return InLoc;
	}

	const FVector N = DirectionToNormal(Dir);
	if (N.IsNearlyZero())
	{
		return InLoc;
	}

	return InLoc + N * (GridXY * 0.5f);
}

FVector ABuildingActor::RemoveWallEdgeOffset(const FVector& InLoc, EGridDirection Dir, bool bUseCentered) const
{
	if (!UsesDirection() || bUseCentered)
	{
		return InLoc;
	}

	const FVector N = DirectionToNormal(Dir);
	if (N.IsNearlyZero())
	{
		return InLoc;
	}

	return InLoc - N * (GridXY * 0.5f);
}

// ------------------------------
// Apply snapped transform
// ------------------------------
void ABuildingActor::ApplySnappedTransform(const FVector& Loc, const FRotator& Rot)
{
	if (bApplying)
	{
		return;
	}

	bApplying = true;
	SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	bApplying = false;
}

// ------------------------------
// Cached flag maintenance
// ------------------------------
void ABuildingActor::UpdatePrevAppliedFlags()
{
	bPrevAppliedHalfHeight = bHalfHeight;
	bPrevAppliedCentered = bCentered;
}

void ABuildingActor::ResetDragSessionState()
{
	bSessionIsXY = false;
	XYAccumulatedMove = FVector2D::ZeroVector;

	DragStartDirection = EGridDirection::None;

	bHasPrevDragRawWorld = false;
	PrevDragRawWorld = FVector::ZeroVector;

	PrevDragCellForDirLock = FIntVector::ZeroValue;
	bLockDirectionUntilInsideCell = false;
}

// ------------------------------
// Dragging (live snap)
// ------------------------------
void ABuildingActor::EditorApplyTranslation(const FVector& DeltaTranslation, bool, bool, bool)
{
	if (bApplying)
	{
		return;
	}

	FVector RawWorld = GetActorLocation();

	// ----------------------------
	// DRAG START
	// ----------------------------
	if (!bDragging)
	{
		bDragging = true;
		ResetDragSessionState();

		bHasWidgetLocation = TryGetTranslateWidgetWorldLocation(DragWidgetLocation);
		if (bHasWidgetLocation)
		{
			RawWorld = DragWidgetLocation;
		}

		if (UsesDirection() && GridDirection == EGridDirection::None)
		{
			GridDirection = ComputeDirectionFromRotation();
		}

		DragStartDirection = GridDirection;

		PrevDragRawWorld = RawWorld;
		bHasPrevDragRawWorld = true;

		// Remove PREVIOUS offsets so drag start doesn't jump
		FVector RefWorld = RawWorld;
		RefWorld = RemoveHalfHeightOffset(RefWorld, bPrevAppliedHalfHeight);
		RefWorld = RemoveWallEdgeOffset(RefWorld, GridDirection, bPrevAppliedCentered);

		DragRefLocation = RefWorld;
		DragCurrentCell = WorldToCell(DragRefLocation);

		PrevDragCellForDirLock = DragCurrentCell;
		bLockDirectionUntilInsideCell = false;

		return; // avoid first-frame micro-delta
	}

	// ----------------------------
	// NORMAL DRAGGING
	// ----------------------------
	FVector WidgetLoc = FVector::ZeroVector;
	if (TryGetTranslateWidgetWorldLocation(WidgetLoc))
	{
		bHasWidgetLocation = true;
		DragWidgetLocation = WidgetLoc;
		RawWorld = WidgetLoc;
	}
	else
	{
		bHasWidgetLocation = false;
		RawWorld = GetActorLocation() + DeltaTranslation;
	}

	// Drag delta (small-move guard + XY proof)
	FVector DragDelta = FVector::ZeroVector;
	if (bHasPrevDragRawWorld)
	{
		DragDelta = RawWorld - PrevDragRawWorld;
	}
	PrevDragRawWorld = RawWorld;
	bHasPrevDragRawWorld = true;

	if (UsesDirection() && GridDirection == EGridDirection::None)
	{
		GridDirection = ComputeDirectionFromRotation();
	}
	if (UsesDirection() && DragStartDirection == EGridDirection::None)
	{
		DragStartDirection = GridDirection;
	}

	// Prevent "snap on click": until meaningful movement, freeze direction entirely.
	const float SmallMove = 2.0f;
	const bool bHasMeaningfulMove = (DragDelta.Size2D() >= SmallMove);

	// ---------------------------------------------------
	// Session-latched XY intent detection (gizmo-based)
	// ---------------------------------------------------
	if (UsesDirection() && !bSessionIsXY)
	{
		// Only declare XY mode if the user actually grabbed the XY plane handle.
		// This prevents tiny motion noise from unlocking XY.
		if (GetDirectionAxisLockFromGizmo() == EDirAxisLock::None)
		{
			// Peek the actual widget axis (XY plane is the one we care about).
			if (GEditor)
			{
				if (FViewport* ActiveViewport = GEditor->GetActiveViewport())
				{
					if (FEditorViewportClient* VC = static_cast<FEditorViewportClient*>(ActiveViewport->GetClient()))
					{
						if (VC->GetCurrentWidgetAxis() == EAxisList::XY)
						{
							bSessionIsXY = true;
						}
					}
				}
			}
		}
	}

	// Axis lock:
	// - Before XY is latched: use gizmo axis if explicit; otherwise keep current axis locked.
	// - After XY is latched: free mode (no axis lock).
	EDirAxisLock AxisLock = EDirAxisLock::None;
	if (UsesDirection())
	{
		if (!bSessionIsXY)
		{
			AxisLock = GetDirectionAxisLockFromGizmo();
			if (AxisLock == EDirAxisLock::None)
			{
				AxisLock = GetFallbackAxisLockFromCurrentDir(GridDirection);
			}
		}
		else
		{
			AxisLock = EDirAxisLock::None;
		}
	}


	// ============================================================
	// PASS 1 — Direction intent in a temp cell (no edge feedback)
	// ============================================================
	FVector TempRef = RawWorld;
	TempRef = RemoveHalfHeightOffset(TempRef, bHalfHeight);

	const FIntVector TempCell = WorldToCell(TempRef);
	const FVector TempCenter = CellToWorld(TempCell);

	EGridDirection DirectionIntent = GridDirection;
	if (!bHasMeaningfulMove)
	{
		DirectionIntent = DragStartDirection;
	}
	else if (UsesDirection())
	{
		const FVector LocalForDir_Intent = MakeLocalForDirection(RawWorld, TempCenter);
		DirectionIntent = ComputeWallDirectionFromLocal(DirectionIntent, LocalForDir_Intent, AxisLock);
	}

	// ============================================================
	// PASS 2 — Remove edge using intent, then solve TRUE cell
	// ============================================================
	FVector RefWorld = RawWorld;
	RefWorld = RemoveHalfHeightOffset(RefWorld, bHalfHeight);
	RefWorld = RemoveWallEdgeOffset(RefWorld, DirectionIntent, bCentered);

	DragRefLocation = RefWorld;
	DragCurrentCell = WorldToCell_StableDuringDrag(DragRefLocation, DragCurrentCell);

	if (bCentered)
	{
		if (DragCurrentCell != PrevDragCellForDirLock)
		{
			// We just entered a new cell; lock direction until we're sufficiently inside it.
			bLockDirectionUntilInsideCell = true;
			PrevDragCellForDirLock = DragCurrentCell;
		}
	}

	const FVector CellCenter = CellToWorld(DragCurrentCell);

	// ============================================================
	// PASS 3 — Final direction solve in final cell
	// ============================================================
	FVector FinalLoc = CellCenter;
	FRotator FinalRot = GetActorRotation();

	bool bAllowDirectionSolve = true;

	if (bCentered && bLockDirectionUntilInsideCell)
	{
		const FVector Local = MakeLocalForDirection(RawWorld, CellCenter);

		const float AbsFx = FMath::Abs(Local.X / GridXY);
		const float AbsFy = FMath::Abs(Local.Y / GridXY);

		const float LockInT = FMath::Clamp(CenteredDirectionLockInFrac, 0.0f, 0.49f);

		// "Inside enough" means we're not hugging either boundary.
		const bool bInsideEnough = (AbsFx < (0.5f - LockInT)) && (AbsFy < (0.5f - LockInT));

		if (!bInsideEnough)
		{
			bAllowDirectionSolve = false; // freeze direction
		}
		else
		{
			bLockDirectionUntilInsideCell = false; // unlock once we're safely inside
		}
	}

	if (UsesDirection())
	{
		EGridDirection NewDir = DirectionIntent;

		if (!bHasMeaningfulMove)
		{
			NewDir = DragStartDirection;
		}
		else if (bAllowDirectionSolve)
		{
			const FVector LocalForDir_Final = MakeLocalForDirection(RawWorld, CellCenter);
			NewDir = ComputeWallDirectionFromLocal(DirectionIntent, LocalForDir_Final, AxisLock);
		}
		else
		{
			// Freeze direction while hugging the boundary of a newly-entered cell (prevents flicker)
			NewDir = GridDirection;
		}

		GridDirection = NewDir;
		FinalLoc = ApplyWallEdgeOffset(CellCenter, GridDirection, bCentered);
		FinalRot = DirectionToRotation(GridDirection);
	}
	else
	{
		GridDirection = EGridDirection::None;
	}

	FinalLoc = ApplyHalfHeightOffset(FinalLoc, bHalfHeight);
	ApplySnappedTransform(FinalLoc, FinalRot);
}

void ABuildingActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		bDragging = false;
		bHasWidgetLocation = false;
		DragWidgetLocation = FVector::ZeroVector;

		ResetDragSessionState();

		SnapToGridNow();
	}
}

void ABuildingActor::PostActorCreated()
{
	Super::PostActorCreated();

	UpdatePrevAppliedFlags();

	if (GIsEditor && GetWorld() && !GetWorld()->IsGameWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				SnapToGridNow();
			}));
	}
}

void ABuildingActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName =
		PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bMirrored))
	{
		ApplyMirror();
		return;
	}

	if (PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bHalfHeight) ||
		PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bCentered))
	{
		ResetDragSessionState();
		SnapToGridNowWithPrev(bPrevAppliedHalfHeight, bPrevAppliedCentered);
		UpdatePrevAppliedFlags();

#if WITH_EDITOR
		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports(true);
			GEditor->NoteSelectionChange();
		}
#endif

		return;
	}


	ResetDragSessionState();
	SnapToGridNow();
}

void ABuildingActor::SnapToGridNow()
{
	SnapToGridNowWithPrev(bPrevAppliedHalfHeight, bPrevAppliedCentered);
	UpdatePrevAppliedFlags();
}

void ABuildingActor::SnapToGridNowWithPrev(bool bPrevHalfHeight, bool bPrevCentered)
{
	if (bApplying)
	{
		return;
	}

	if (UsesDirection() && GridDirection == EGridDirection::None)
	{
		GridDirection = ComputeDirectionFromRotation();
	}

	FVector RefLoc = GetActorLocation();
	RefLoc = RemoveHalfHeightOffset(RefLoc, bPrevHalfHeight);
	RefLoc = RemoveWallEdgeOffset(RefLoc, GridDirection, bPrevCentered);

	const FIntVector Cell = WorldToCell(RefLoc);
	const FVector CellCenter = CellToWorld(Cell);

	FVector FinalLoc = CellCenter;
	FRotator FinalRot = GetActorRotation();

	if (UsesDirection())
	{
		FinalLoc = ApplyWallEdgeOffset(CellCenter, GridDirection, bCentered);
		FinalRot = DirectionToRotation(GridDirection);
	}
	else
	{
		GridDirection = EGridDirection::None;
	}

	FinalLoc = ApplyHalfHeightOffset(FinalLoc, bHalfHeight);
	ApplySnappedTransform(FinalLoc, FinalRot);
}

static int32 StableCell1D(float WorldCoord, float Grid, int32 PrevCell, float HystFrac)
{
	const float CellFloat = WorldCoord / Grid;             // e.g. 3.49
	const float PrevCellF = (float)PrevCell;               // e.g. 3
	const float Delta = CellFloat - PrevCellF;             // e.g. 0.49

	// Normal switching thresholds are +/-0.5.
	// With hysteresis, require pushing beyond 0.5 + H.
	const float H = FMath::Clamp(HystFrac, 0.f, 0.25f);

	if (Delta > 0.5f + H) return PrevCell + 1;
	if (Delta < -0.5f - H) return PrevCell - 1;
	return PrevCell; // stay put
}

FIntVector ABuildingActor::WorldToCell_StableDuringDrag(const FVector& World, const FIntVector& PrevCell) const
{
	return FIntVector(
		StableCell1D(World.X, GridXY, PrevCell.X, CellSwitchHysteresisFrac),
		StableCell1D(World.Y, GridXY, PrevCell.Y, CellSwitchHysteresisFrac),
		StableCell1D(World.Z, GridZ, PrevCell.Z, CellSwitchHysteresisFrac)
	);
}

void ABuildingActor::ApplyMirror()
{
	if (!MeshComponent)
	{
		return;
	}

	FVector Scale(1.f, 1.f, 1.f);
	Scale.X = bMirrored ? -1.f : 1.f;
	MeshComponent->SetRelativeScale3D(Scale);
}

#endif
