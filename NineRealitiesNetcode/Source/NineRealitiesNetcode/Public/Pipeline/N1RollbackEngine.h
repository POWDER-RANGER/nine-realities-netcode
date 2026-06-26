// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1RollbackEngine.generated.h"

class UN1PredictionBuffer;

/**
 * @struct FN1RollbackContext
 * @brief Context for a rollback operation
 */
USTRUCT()
struct FN1RollbackContext
{
	GENERATED_BODY()

	/** Tick to rollback to */
	UPROPERTY()
	int32 RollbackTargetTick = 0;

	/** Snapshot state at the rollback point */
	UPROPERTY()
	FN1WorldSnapshot BaselineSnapshot;

	/** Inputs to replay */
	UPROPERTY()
	TArray<FN1InputFrame> InputsToReplay;

	/** Number of frames to rollback */
	UPROPERTY()
	int32 RollbackDepth = 0;

	/** Estimated cost in ms */
	UPROPERTY()
	float EstimatedCostMs = 0.0f;
};

/**
 * @class UN1RollbackEngine
 * @brief Rollback and replay engine for correcting prediction errors.
 *
 * When the client detects that its prediction diverged from the server
 * state, the rollback engine:
 * 1. Rewinds the simulation to the server snapshot tick
 * 2. Replays all inputs from that tick forward
 * 3. Produces a corrected state that matches server + local inputs
 *
 * The cost of rollback scales with rollback depth:
 * - 1-3 frames:   ~0.8ms (negligible)
 * - 4-8 frames:   ~2.3ms (minor)
 * - 9-15 frames:  ~5.1ms (moderate)
 * - 16+ frames:   ~11.4ms+ (significant)
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Rollback Engine"))
class NINEREALITIESNETCODE_API UN1RollbackEngine : public UObject
{
	GENERATED_BODY()

public:
	UN1RollbackEngine();

	/** Initialize the rollback engine */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Rollback")
	void Initialize(int32 InMaxRollbackFrames);

	/**
	 * Perform rollback and replay.
	 * @return true if rollback was performed
	 */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Rollback")
	bool RollbackAndReplay(
		const FN1RollbackContext& Context,
		UN1PredictionBuffer* Buffer,
		TArray<FN1EntityState>& OutCorrectedStates
	);

	/** Check if rollback is needed based on divergence */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	bool IsRollbackRequired(float Divergence, float Threshold) const;

	/** Estimate the cost of a rollback in ms */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	float EstimateRollbackCost(int32 RollbackDepth, int32 EntityCount) const;

	/** @return Maximum configured rollback depth */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	int32 GetMaxRollbackFrames() const { return MaxRollbackFrames; }

	/** @return Total rollbacks performed */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	int32 GetTotalRollbacks() const { return TotalRollbacks; }

	/** @return Average rollback depth */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	float GetAverageRollbackDepth() const;

	/** @return Worst-case rollback depth seen */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Rollback")
	int32 GetWorstRollbackDepth() const { return WorstRollbackDepth; }

private:
	/** Rewind simulation to target tick */
	void RewindToTick(const FN1WorldSnapshot& Baseline, int32 TargetTick);

	/** Replay a single input frame */
	FN1EntityState ReplayInput(const FN1EntityState& CurrentState, const FN1InputFrame& Input);

private:
	int32 MaxRollbackFrames = 16;
	int32 TotalRollbacks = 0;
	float TotalRollbackDepth = 0.0f;
	int32 WorstRollbackDepth = 0;

	/** Cost coefficients for rollback estimation */
	static constexpr float BASE_ROLLBACK_COST_MS = 0.3f;
	static constexpr float PER_FRAME_COST_MS = 0.15f;
	static constexpr float PER_ENTITY_COST_MS = 0.05f;
};
