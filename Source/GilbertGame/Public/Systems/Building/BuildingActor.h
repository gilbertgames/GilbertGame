#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Systems/Building/EGridSlot.h"
#include "Systems/Building/EGridDirection.h"
#include "BuildingActor.generated.h"

UCLASS()
class GILBERTGAME_API ABuildingActor : public AActor
{
	GENERATED_BODY()

public:
	ABuildingActor();

#if WITH_EDITOR
	virtual void EditorApplyTranslation(
		const FVector& DeltaTranslation,
		bool bAltDown,
		bool bShiftDown,
		bool bCtrlDown
	) override;

	virtual void PostActorCreated() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

public:

	UPROPERTY(EditAnywhere, Category = "Instance")
	bool bMirrored = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;



	UPROPERTY(EditDefaultsOnly, Category = "Building")
	EGridSlot GridSlot = EGridSlot::Floor;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	EGridDirection GridDirection = EGridDirection::None;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid")
	float GridXY = 512.f;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid")
	float GridZ = 384.f;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0"))
	float DirectionBiasUnits = 64.f;

#if WITH_EDITOR
private:
	FVector DragRawLocation = FVector::ZeroVector;
	FIntVector DragCurrentCell = FIntVector::ZeroValue;

	bool bDragging = false;
	bool bApplying = false;

	// Grid helpers
	FIntVector WorldToCell(const FVector& World) const;
	FVector CellToWorld(const FIntVector& Cell) const;

	// Direction helpers (Option B)
	bool UsesDirection() const;
	EGridDirection ComputeDirectionFromLocal(const FVector& Local) const;

	FVector DirectionToNormal(EGridDirection Dir) const;
	FRotator DirectionToRotation(EGridDirection Dir) const;

	void ApplySnappedTransform(const FVector& Loc, const FRotator& Rot);

	void SnapToGridNow();
	void ApplyMirror();
#endif
};
