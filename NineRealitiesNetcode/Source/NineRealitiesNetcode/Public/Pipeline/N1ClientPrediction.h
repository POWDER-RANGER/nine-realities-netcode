// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1ClientPrediction.generated.h"

class UN1PredictionBuffer;
class UN1NetworkClock;

/**
 * @enum EN1PredictionMode
 * @brief Aggressiveness level for client-side prediction
 */
UENUM(BlueprintType)
enum class EN1PredictionMode : uint8
{
	/** Conservative — minimal corrections but higher perceived latency */
	Conservative	UMETA(DisplayName = "Conservative"),
	/** Balanced — default sweet spot */
	Balanced		UMETA(DisplayName = "Balanced"),
	/** Aggressive — maximum responsiveness but more corrections */
	Aggressive		UMETA(DisplayName = "Aggressive"),
	/** Adaptive — dynamically adjusts based on connection quality */
	Adaptive		UMETA(DisplayName = "Adaptive")
};

/**
 * @class UN1ClientPrediction
 * @brief Client-side prediction system for the N+1 model.
 *
 * Each client runs its own local simulation (one of the N realities).
 * When local input arrives, the client immediately simulates the result
 * and renders it — providing sub-frame responsiveness. The predicted
 * states are stored in the prediction buffer for later reconciliation.
 *
 * Key principle: "Every client predicts the future to maintain
 * responsive gameplay."
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Client Prediction"))
class NINEREALITIESNETCODE_API UN1ClientPrediction : public UObject
{
	GENERATED_BODY()

public:
	UN1ClientPrediction();

	/** Initialize the prediction system */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Prediction")
	void Initialize(UN1PredictionBuffer* InBuffer, UN1NetworkClock* InClock, float InTickRate);

	/** Process local input and generate a predicted state */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Prediction")
	FN1EntityState PredictLocalInput(const FN1InputFrame& Input, const FN1EntityState& CurrentState);

	/** Tick the prediction system — called every frame */
	void Tick(float DeltaTime);

	/** Set prediction mode */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Prediction")
	void SetPredictionMode(EN1PredictionMode Mode) { PredictionMode = Mode; }

	/** Get current prediction mode */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Prediction")
	EN1PredictionMode GetPredictionMode() const { return PredictionMode; }

	/** @return Total predictions made this session */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Prediction")
	int32 GetTotalPredictions() const { return TotalPredictions; }

	/** @return Current prediction accuracy (0-100%) */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Prediction")
	float GetPredictionAccuracy() const;

	/** Record a successful prediction (no correction needed) */
	void RecordSuccessfulPrediction();

	/** Record a failed prediction (correction was applied) */
	void RecordFailedPrediction();

	/** Enable/disable adaptive prediction based on connection quality */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Prediction")
	void SetAdaptivePrediction(bool bEnabled) { bAdaptiveEnabled = bEnabled; }

private:
	/** Calculate prediction aggression factor (0-2 range) */
	float GetPredictionAggression() const;

	/** Update adaptive mode based on current metrics */
	void UpdateAdaptiveMode(float Latency, float Jitter, float Loss);

private:
	UPROPERTY()
	TObjectPtr<UN1PredictionBuffer> Buffer;

	UPROPERTY()
	TObjectPtr<UN1NetworkClock> Clock;

	UPROPERTY()
	EN1PredictionMode PredictionMode = EN1PredictionMode::Balanced;

	float TickRate = 120.0f;
	int32 TotalPredictions = 0;
	int32 SuccessfulPredictions = 0;
	int32 FailedPredictions = 0;
	bool bAdaptiveEnabled = true;

	/** Adaptive mode tracking */
	float AdaptiveAggression = 1.0f;
};
