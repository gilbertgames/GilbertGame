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

	UPROPERTY(EditAnywhere, Category = "Instance")
	bool bHalfHeight = false;

	UPROPERTY(EditAnywhere, Category = "Instance", meta = (EditCondition = "GridSlot == EGridSlot::Wall", EditConditionHides))
	bool bCentered = false;

	// Canonical snapped cell location. The actor's world transform is derived from this + other flags.
	// This is the source of truth during dragging (prevents float drift / boundary jitter).
	UPROPERTY(VisibleInstanceOnly, Category = "Instance|Grid", meta = (DisplayName = "Grid Cell"))
	FIntVector GridCell = FIntVector::ZeroValue;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	EGridSlot GridSlot = EGridSlot::Floor;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	EGridDirection GridDirection = EGridDirection::None;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid")
	float GridXY = 512.f;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid")
	float GridZ = 384.f;

	// During dragging, require a little extra movement past the half-cell boundary
	// before switching to the next cell (prevents boundary flicker).
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float CellSwitchHysteresisFrac = 0.05f;

	// Axis stickiness when choosing between X-facing and Y-facing (only used in free/XY mode).
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0"))
	float DirectionBiasUnits = 64.f;

	// How far from center (fraction of tile) before sign flips (+/-) are allowed.
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0", ClampMax = "0.49"))
	float FlipThresholdFrac = 0.2f;

	// When centered and close to tile center, freeze axis (prevents flicker).
	// Fraction of GridXY radius around center.
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0", ClampMax = "0.49"))
	float CenterAxisFreezeFrac = 0.2f;

	// When centered and near the diagonal (|AbsFx-AbsFy| small), keep the current axis to prevent rapid X/Y swaps.
	// Expressed in normalized tile units (Local/GridXY).
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0", ClampMax = "0.49"))
	float DiagonalAxisDeadzoneFrac = 0.1f;

	// Session-latched XY intent detection:
	// amount of accumulated motion required in BOTH X and Y before we declare this drag session "XY".
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0"))
	float XYIntentMinUnits = 16.f;

	// After entering a new cell, require some movement into the cell before allowing direction changes.
	UPROPERTY(EditDefaultsOnly, Category = "Building|Grid", meta = (ClampMin = "0.0", ClampMax = "0.49"))
	float CenteredDirectionLockInFrac = 0.25f;


#if WITH_EDITOR
private:
	// Drag state
	FVector DragRefLocation = FVector::ZeroVector; // offsets removed
	FIntVector DragCurrentCell = FIntVector::ZeroValue;

	FVector DragWidgetLocation = FVector::ZeroVector;
	bool bHasWidgetLocation = false;

	bool bDragging = false;
	bool bApplying = false;

	// Cached "previous applied" offset state (used to undo offsets correctly when toggling)
	bool bPrevAppliedHalfHeight = false;
	bool bPrevAppliedCentered = false;

	// For "no snap on click"
	FVector PrevDragRawWorld = FVector::ZeroVector;
	bool bHasPrevDragRawWorld = false;

	FIntVector PrevDragCellForDirLock = FIntVector::ZeroValue;
	bool bLockDirectionUntilInsideCell = false;

	EGridDirection DragStartDirection = EGridDirection::None;

	// -----------------------------
	// NEW: Session-latched XY intent
	// -----------------------------
	bool bSessionIsXY = false;
	FVector2D XYAccumulatedMove = FVector2D::ZeroVector;

	// Direction axis lock choices (derived from gizmo axis)
	enum class EDirAxisLock : uint8
	{
		None,
		ForceXFacing, // PosX/NegX only
		ForceYFacing  // PosY/NegY only
	};

	// Grid helpers
	// Compute the canonical cell from a reference world location (offsets removed).
	FIntVector WorldToCell(const FVector& World) const;
	FVector CellToWorld(const FIntVector& Cell) const;

	// Rebuild GridCell from the actor's current world transform, removing the *previously applied* offsets.
	void SyncGridCellFromActorWorld(bool bPrevHalfHeight, bool bPrevCentered);

	// Apply actor transform from GridCell + current flags.
	void ApplyFromGridCell();

	FIntVector WorldToCell_StableDuringDrag(const FVector& World, const FIntVector& PrevCell) const;

	// Direction helpers
	bool UsesDirection() const;
	EGridDirection ComputeDirectionFromRotation() const;
	FVector DirectionToNormal(EGridDirection Dir) const;
	FRotator DirectionToRotation(EGridDirection Dir) const;

	// Local vector used for direction solving (relative to the current cell center)
	FVector MakeLocalForDirection(const FVector& RawWorld, const FVector& CellCenter) const;

	// Gizmo axis lock
	EDirAxisLock GetDirectionAxisLockFromGizmo() const;

	// If gizmo axis is ambiguous, keep the current axis locked (prevents flicker before XY is proven).
	EDirAxisLock GetFallbackAxisLockFromCurrentDir(EGridDirection CurrentDir) const;

	// Wall direction solve (sign deadzone + optional axis lock)
	EGridDirection ComputeWallDirectionFromLocal(EGridDirection CurrentDir, const FVector& LocalForDir, EDirAxisLock AxisLock) const;

	// Apply transform
	void ApplySnappedTransform(const FVector& Loc, const FRotator& Rot);

	// If possible, read editor translate widget world position.
	bool TryGetTranslateWidgetWorldLocation(FVector& OutWorld) const;

	// Offsets (explicit-state versions)
	FVector ApplyHalfHeightOffset(const FVector& InLoc, bool bUseHalfHeight) const;
	FVector RemoveHalfHeightOffset(const FVector& InLoc, bool bUseHalfHeight) const;

	FVector ApplyWallEdgeOffset(const FVector& InLoc, EGridDirection Dir, bool bUseCentered) const;
	FVector RemoveWallEdgeOffset(const FVector& InLoc, EGridDirection Dir, bool bUseCentered) const;

	// Snap entrypoints
	void SnapToGridNow();
	void SnapToGridNowWithPrev(bool bPrevHalfHeight, bool bPrevCentered);
	void UpdatePrevAppliedFlags();

	void ApplyMirror();

	void ResetDragSessionState();
#endif
};
