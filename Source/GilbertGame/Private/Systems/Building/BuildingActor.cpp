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
	}
	else if (
		PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bHalfHeight) ||
		PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bCentered)
		)
	{
		// Re-apply snap immediately when these change.
		SnapToGridNow();
	}
}

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


bool ABuildingActor::UsesDirection() const
{
	return GridSlot == EGridSlot::Wall;
}

EGridDirection ABuildingActor::ComputeDirectionFromLocal(const FVector& Local) const
{
	const float AbsX = FMath::Abs(Local.X);
	const float AbsY = FMath::Abs(Local.Y);

	// If we don't have a current direction, pick one normally.
	if (GridDirection == EGridDirection::None)
	{
		if (AbsX > AbsY)
		{
			return (Local.X >= 0.f) ? EGridDirection::PosX : EGridDirection::NegX;
		}
		return (Local.Y >= 0.f) ? EGridDirection::PosY : EGridDirection::NegY;
	}

	// Determine which axis the current direction belongs to.
	const bool bCurrentIsX =
		(GridDirection == EGridDirection::PosX || GridDirection == EGridDirection::NegX);

	if (bCurrentIsX)
	{
		// Stay X unless Y clearly wins by a margin.
		if (AbsY > AbsX + DirectionBiasUnits)
		{
			return (Local.Y >= 0.f) ? EGridDirection::PosY : EGridDirection::NegY;
		}
		return (Local.X >= 0.f) ? EGridDirection::PosX : EGridDirection::NegX;
	}
	else
	{
		// Stay Y unless X clearly wins by a margin.
		if (AbsX > AbsY + DirectionBiasUnits)
		{
			return (Local.X >= 0.f) ? EGridDirection::PosX : EGridDirection::NegX;
		}
		return (Local.Y >= 0.f) ? EGridDirection::PosY : EGridDirection::NegY;
	}
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

	// Mesh forward-axis correction (Blender → UE)
	Rot.Yaw += 90.f;   // or -90.f depending on what you need

	return Rot;
}

void ABuildingActor::ApplySnappedTransform(const FVector& Loc, const FRotator& Rot)
{
	if (bApplying)
		return;

	bApplying = true;
	SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	bApplying = false;
}

FVector ABuildingActor::ApplyHalfHeightOffset(const FVector& InLoc) const
{
	if (!bHalfHeight)
	{
		return InLoc;
	}

	FVector Out = InLoc;
	Out.Z += (GridZ * 0.5f);
	return Out;
}

void ABuildingActor::EditorApplyTranslation(
	const FVector& DeltaTranslation,
	bool, bool, bool
)
{
	if (bApplying)
		return;

	if (!bDragging)
	{
		bDragging = true;

		// Prefer the editor widget's true world position (more consistent than accumulating deltas)
		// but fall back to the actor location if we can't read the widget yet.
		DragRawLocation = GetActorLocation();
		DragWidgetLocation = FVector::ZeroVector;
		bHasWidgetLocation = TryGetTranslateWidgetWorldLocation(DragWidgetLocation);
		if (bHasWidgetLocation)
		{
			DragRawLocation = DragWidgetLocation;
		}

		DragCurrentCell = WorldToCell(DragRawLocation);
	}

	// On each tick, try to read the widget location again.
	// If available, use that as the source-of-truth for snapping decisions.
	FVector WidgetLoc = FVector::ZeroVector;
	if (TryGetTranslateWidgetWorldLocation(WidgetLoc))
	{
		bHasWidgetLocation = true;
		DragWidgetLocation = WidgetLoc;
		DragRawLocation = WidgetLoc;
	}
	else
	{
		// Fallback: accumulate deltas (original behavior)
		bHasWidgetLocation = false;
		DragRawLocation += DeltaTranslation;
	}

	const FIntVector NewCell = WorldToCell(DragRawLocation);
	if (NewCell != DragCurrentCell)
	{
		DragCurrentCell = NewCell;
	}

	const FVector CellCenter = CellToWorld(DragCurrentCell);

	FVector FinalLoc = CellCenter;
	FRotator FinalRot = GetActorRotation();

	if (UsesDirection())
	{
		const FVector Local = DragRawLocation - CellCenter;
		GridDirection = ComputeDirectionFromLocal(Local);

		// If centered: don't push to edge. Otherwise: normal wall edge offset.
		if (!bCentered)
		{
			const FVector Normal = DirectionToNormal(GridDirection);
			FinalLoc = CellCenter + Normal * (GridXY * 0.5f);
		}
		else
		{
			FinalLoc = CellCenter;
		}

		FinalRot = DirectionToRotation(GridDirection);
	}
	else
	{
		GridDirection = EGridDirection::None;
	}

	FinalLoc = ApplyHalfHeightOffset(FinalLoc);
	ApplySnappedTransform(FinalLoc, FinalRot);
}

void ABuildingActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		// Reset our drag state so the next drag starts clean.
		bDragging = false;
		bHasWidgetLocation = false;
		DragWidgetLocation = FVector::ZeroVector;

		// Ensure we finish snapped (covers any edge cases where the last EditorApplyTranslation
		// call didn't run with the final widget position).
		SnapToGridNow();
	}
}

void ABuildingActor::PostActorCreated()
{
	Super::PostActorCreated();

	if (GIsEditor && !GetWorld()->IsGameWorld())
	{
		// Defer one tick so placement final transform is applied first
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					UE_LOG(LogTemp, Warning, TEXT("Deferred Snap %s"), *GetName());
					SnapToGridNow();
				}));
		}
	}
}

void ABuildingActor::SnapToGridNow()
{
	// Avoid re-entry
	if (bApplying)
	{
		return;
	}

	const FIntVector Cell = WorldToCell(GetActorLocation());
	const FVector CellCenter = CellToWorld(Cell);

	if (UsesDirection())
	{
		if (GridDirection == EGridDirection::None)
		{
			const FVector Local = GetActorLocation() - CellCenter;
			GridDirection = ComputeDirectionFromLocal(Local);
		}

		FVector Loc = CellCenter;

		if (!bCentered)
		{
			const FVector Normal = DirectionToNormal(GridDirection);
			Loc = CellCenter + Normal * (GridXY * 0.5f);
		}

		Loc = ApplyHalfHeightOffset(Loc);
		ApplySnappedTransform(Loc, DirectionToRotation(GridDirection));
	}
	else
	{
		GridDirection = EGridDirection::None;

		const FVector Loc = ApplyHalfHeightOffset(CellCenter);
		ApplySnappedTransform(Loc, GetActorRotation());
	}
}

void ABuildingActor::ApplyMirror()
{
	if (!MeshComponent)
		return;

	FVector Scale = FVector(1.f, 1.f, 1.f);
	Scale.X = bMirrored ? -1 : 1;
	MeshComponent->SetRelativeScale3D(Scale);
}

#endif
