#include "Systems/Building/BuildingActor.h"

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

	if (PropName == GET_MEMBER_NAME_CHECKED(ABuildingActor, bMirror))
	{
		ApplyMirror();
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
	//return FRotationMatrix::MakeFromXZ(-N, FVector::UpVector).Rotator();

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
		DragRawLocation = GetActorLocation();
		DragCurrentCell = WorldToCell(DragRawLocation);
	}

	DragRawLocation += DeltaTranslation;

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

		const FVector Normal = DirectionToNormal(GridDirection);
		FinalLoc = CellCenter + Normal * (GridXY * 0.5f);
		FinalRot = DirectionToRotation(GridDirection);
	}
	else
	{
		GridDirection = EGridDirection::None;
	}

	ApplySnappedTransform(FinalLoc, FinalRot);
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

		const FVector Normal = DirectionToNormal(GridDirection);
		ApplySnappedTransform(CellCenter + Normal * (GridXY * 0.5f), DirectionToRotation(GridDirection));
	}
	else
	{
		GridDirection = EGridDirection::None;
		ApplySnappedTransform(CellCenter, GetActorRotation());
	}
}

void ABuildingActor::ApplyMirror()
{
	if (!MeshComponent)
		return;

	FVector Scale = FVector(1.f, 1.f, 1.f);
	Scale.X = bMirror ? -1 : 1;
	MeshComponent->SetRelativeScale3D(Scale);
}

#endif
