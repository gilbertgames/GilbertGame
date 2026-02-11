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

	virtual void PostEditMove(bool bFinished) override;

	virtual void PostActorCreated() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(EditAnywhere, Category = "Instance")
	bool bMirrored = false;

	// When true: snap normally, then add GridZ * 0.5 to final Z.
	UPROPERTY(EditAnywhere, Category = "Instance")
	bool bHalfHeight = false;

	// Walls only: when true, do NOT apply the +/- GridXY * 0.5 edge offset (place at cell center).
	UPROPERTY(EditAnywhere, Category = "Instance", meta = (EditCondition = "GridSlot == EGridSlot::Wall", EditConditionHides))
	bool bCentered = false;

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

	FVector DragWidgetLocation = FVector::ZeroVector;
	bool bHasWidgetLocation = false;

	bool bDragging = false;
	bool bApplying = false;

	// Grid helpers
	FIntVector WorldToCell(const FVector& World) const;
	FVector CellToWorld(const FIntVector& Cell) const;

	// Direction helpers
	bool UsesDirection() const;
	EGridDirection ComputeDirectionFromLocal(const FVector& Local) const;

	FVector DirectionToNormal(EGridDirection Dir) const;
	FRotator DirectionToRotation(EGridDirection Dir) const;

	void ApplySnappedTransform(const FVector& Loc, const FRotator& Rot);

	// If possible, read the editor translate widget's true world position.
	bool TryGetTranslateWidgetWorldLocation(FVector& OutWorld) const;

	// Consistent half-height offset application
	FVector ApplyHalfHeightOffset(const FVector& InLoc) const;

	void SnapToGridNow();
	void ApplyMirror();
#endif
};
