// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1BlendInterpolator.generated.h"

/**
 * @enum EN1BlendCurve
 * @brief Interpolation curve for correction blending
 */
UENUM(BlueprintType)
enum class EN1BlendCurve : uint8
{
	/** Linear blend — constant rate */
	Linear			UMETA(DisplayName = "Linear"),
	/** Smooth step — ease in/out */
	SmoothStep		UMETA(DisplayName = "Smooth Step"),
	/** Exponential decay — fast start, slow end */
	Exponential		UMETA(DisplayName = "Exponential Decay"),
	/** Critical damping — no overshoot */
	CriticalDamping	UMETA(DisplayName = "Critical Damping")
};

/**
 * @class UN1BlendInterpolator
 * @brief Visual smoothing system for correction blending.
 *
 * When a rollback corrects the client state, snapping instantly would
 * create jarring "rubber-banding." The blend interpolator smoothly
 * transitions from the corrected state to the visual state over N frames.
 *
 * This is the final step in the reconciliation pipeline:
 * Prediction -> Rollback -> Blend -> Render
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Blend Interpolator"))
class NINEREALITIESNETCODE_API UN1BlendInterpolator : public UObject
{
	GENERATED_BODY()

public:
	UN1BlendInterpolator();

	/** Initialize with configuration */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Blend")
	void Initialize(int32 InBlendFrames, EN1BlendCurve InCurve);

	/** Start a blend from corrected to target state */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Blend")
	void BeginBlend(const FN1EntityState& CorrectedState, const FN1EntityState& CurrentVisualState);

	/** Get the blended state for the current frame */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Blend")
	FN1EntityState GetBlendedState(const FN1EntityState& CorrectedState) const;

	/** Tick the interpolator — advance blend progress */
	void Tick(float DeltaTime);

	/** Check if a blend is currently active */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Blend")
	bool IsBlending() const { return bIsBlending; }

	/** Get current blend progress (0-1) */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Blend")
	float GetBlendProgress() const { return BlendProgress; }

	/** Set blend curve type */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Blend")
	void SetBlendCurve(EN1BlendCurve NewCurve) { BlendCurve = NewCurve; }

	/** Set number of blend frames */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Blend")
	void SetBlendFrames(int32 Frames) { BlendFrames = FMath::Clamp(Frames, 1, 30); }

private:
	/** Apply the selected blend curve to progress */
	float ApplyCurve(float T) const;

private:
	UPROPERTY()
	FN1EntityState BlendStartState;

	UPROPERTY()
	FN1EntityState BlendTargetState;

	UPROPERTY()
	EN1BlendCurve BlendCurve = EN1BlendCurve::SmoothStep;

	int32 BlendFrames = 5;
	float BlendProgress = 0.0f;
	bool bIsBlending = false;
};
