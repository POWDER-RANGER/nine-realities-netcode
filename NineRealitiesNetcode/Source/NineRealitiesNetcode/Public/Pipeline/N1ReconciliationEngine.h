// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1ReconciliationEngine.generated.h"

class UN1PredictionBuffer;

/**
 * @enum EN1ReconciliationStrategy
 * @brief Strategy for reconciling client prediction with server authority
 */
UENUM(BlueprintType)
enum class EN1ReconciliationStrategy : uint8
{
	/** Full rollback + replay — most accurate, most expensive */
	FullRollbackReplay		UMETA(DisplayName = "Full Rollback + Replay"),
	/** State interpolation — smooth blend to server state */
	StateInterpolation		UMETA(DisplayName = "State Interpolation"),
	/** Delta correction — apply position/velocity delta */
	DeltaCorrection			UMETA(DisplayName = "Delta Correction"),
	/** Snap immediate — instant correction (debug only) */
	SnapImmediate			UMETA(DisplayName = "Snap Immediate"),
	/** Adaptive — choose strategy based on divergence magnitude */
	Adaptive				UMETA(DisplayName = "Adaptive")
};

/**
 * @struct FN1ReconciliationResult
 * @brief Result of a reconciliation operation
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1ReconciliationResult
{
	GENERATED_BODY()

	/** Whether reconciliation was performed */
	UPROPERTY(BlueprintReadOnly)
	bool bWasCorrected = false;

	/** Strategy used */
	UPROPERTY(BlueprintReadOnly)
	EN1ReconciliationStrategy StrategyUsed = EN1ReconciliationStrategy::Adaptive;

	/** Magnitude of the divergence detected */
	UPROPERTY(BlueprintReadOnly)
	float DivergenceMagnitude = 0.0f;

	/** Number of frames rolled back */
	UPROPERTY(BlueprintReadOnly)
	int32 RollbackFrames = 0;

	/** Number of input frames replayed */
	UPROPERTY(BlueprintReadOnly)
	int32 ReplayedInputs = 0;

	/** Time spent in reconciliation (ms) */
	UPROPERTY(BlueprintReadOnly)
	float ReconciliationTimeMs = 0.0f;
};

/**
 * @class UN1ReconciliationEngine
 * @brief Core engine for reconciling N client predictions with server authority.
 *
 * When a server snapshot arrives, the reconciliation engine:
 * 1. Identifies the snapshot tick in the prediction buffer
 * 2. Compares predicted state with server state
 * 3. If divergence exceeds threshold, triggers rollback
 * 4. Replays all inputs from snapshot tick to present
 * 5. Blends the corrected state to avoid visual pops
 *
 * This is the heart of the N+1 model — the mechanism that resolves
 * the conflict between N predicted realities and 1 authoritative reality.
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Reconciliation Engine"))
class NINEREALITIESNETCODE_API UN1ReconciliationEngine : public UObject
{
	GENERATED_BODY()

public:
	UN1ReconciliationEngine();

	/** Initialize with configuration */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Reconciliation")
	void Initialize(float InRollbackThreshold, int32 InMaxRollbackFrames);

	/**
	 * Process an incoming server snapshot.
	 * @return Reconciliation result detailing what correction was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Reconciliation")
	FN1ReconciliationResult ProcessServerSnapshot(
		const FN1WorldSnapshot& ServerSnapshot,
		UN1PredictionBuffer* PredictionBuffer,
		float CurrentServerTime
	);

	/** Set the active reconciliation strategy */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Reconciliation")
	void SetStrategy(EN1ReconciliationStrategy NewStrategy) { ActiveStrategy = NewStrategy; }

	/** Get the active strategy */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Reconciliation")
	EN1ReconciliationStrategy GetStrategy() const { return ActiveStrategy; }

	/** @return Number of reconciliations performed this session */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Reconciliation")
	int32 GetTotalReconciliations() const { return TotalReconciliations; }

	/** @return Average divergence magnitude */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Reconciliation")
	float GetAverageDivergence() const;

private:
	/** Full rollback + replay reconciliation */
	FN1ReconciliationResult PerformFullRollback(
		const FN1WorldSnapshot& Snapshot,
		UN1PredictionBuffer* Buffer,
		int32 SnapshotTick
	);

	/** State interpolation reconciliation */
	FN1ReconciliationResult PerformStateInterpolation(
		const FN1WorldSnapshot& Snapshot,
		UN1PredictionBuffer* Buffer,
		int32 SnapshotTick
	);

	/** Delta correction reconciliation */
	FN1ReconciliationResult PerformDeltaCorrection(
		const FN1WorldSnapshot& Snapshot,
		UN1PredictionBuffer* Buffer,
		int32 SnapshotTick
	);

	/** Adaptive strategy selector */
	FN1ReconciliationResult PerformAdaptiveReconciliation(
		const FN1WorldSnapshot& Snapshot,
		UN1PredictionBuffer* Buffer,
		int32 SnapshotTick,
		float Divergence
	);

	/** Calculate divergence at a specific tick */
	float CalculateTickDivergence(const FN1WorldSnapshot& Snapshot, UN1PredictionBuffer* Buffer, int32 Tick);

private:
	UPROPERTY()
	EN1ReconciliationStrategy ActiveStrategy = EN1ReconciliationStrategy::Adaptive;

	float RollbackThreshold = 2.5f;
	int32 MaxRollbackFrames = 16;
	int32 TotalReconciliations = 0;
	float TotalDivergence = 0.0f;

	/** Thresholds for adaptive strategy selection */
	static constexpr float DELTA_CORRECTION_MAX = 1.0f;
	static constexpr float INTERPOLATION_MAX = 5.0f;
};
