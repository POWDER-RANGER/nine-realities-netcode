// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Pipeline/N1RollbackEngine.h"
#include "Core/N1PredictionBuffer.h"

UN1RollbackEngine::UN1RollbackEngine()
{
}

void UN1RollbackEngine::Initialize(int32 InMaxRollbackFrames)
{
	MaxRollbackFrames = InMaxRollbackFrames;
	TotalRollbacks = 0;
	TotalRollbackDepth = 0.0f;
	WorstRollbackDepth = 0;

	UE_LOG(LogN1Rollback, Log, TEXT("RollbackEngine initialized: maxRollback=%d"), MaxRollbackFrames);
}

bool UN1RollbackEngine::RollbackAndReplay(
	const FN1RollbackContext& Context,
	UN1PredictionBuffer* Buffer,
	TArray<FN1EntityState>& OutCorrectedStates)
{
	if (!Buffer)
	{
		return false;
	}

	if (Context.RollbackDepth > MaxRollbackFrames)
	{
		UE_LOG(LogN1Rollback, Warning, TEXT("Rollback depth %d exceeds maximum %d — clamping"),
			Context.RollbackDepth, MaxRollbackFrames);
	}

	const int32 EffectiveDepth = FMath::Min(Context.RollbackDepth, MaxRollbackFrames);
	if (EffectiveDepth <= 0)
	{
		return false;
	}

	// Verify we have contiguous inputs for the rollback range
	const int32 StartTick = Context.RollbackTargetTick;
	const int32 EndTick = StartTick + EffectiveDepth;

	if (!Buffer->HasContiguousInputs(StartTick, EndTick))
	{
		UE_LOG(LogN1Rollback, Warning, TEXT("Rollback: Missing contiguous inputs for range [%d, %d]"),
			StartTick, EndTick);
		return false;
	}

	// Perform rollback and replay
	RewindToTick(Context.BaselineSnapshot, StartTick);

	const TArray<FN1InputFrame> Inputs = Buffer->GetInputRange(StartTick, EndTick);
	FN1EntityState CurrentState;

	// Get baseline state
	if (Context.BaselineSnapshot.EntityStates.Num() > 0)
	{
		CurrentState = Context.BaselineSnapshot.EntityStates[0];
	}

	// Replay each input
	for (const auto& Input : Inputs)
	{
		CurrentState = ReplayInput(CurrentState, Input);
		OutCorrectedStates.Add(CurrentState);
	}

	// Update metrics
	TotalRollbacks++;
	TotalRollbackDepth += EffectiveDepth;
	WorstRollbackDepth = FMath::Max(WorstRollbackDepth, EffectiveDepth);

	UE_LOG(LogN1Rollback, Verbose, TEXT("Rollback: depth=%d inputs=%d states=%d"),
		EffectiveDepth, Inputs.Num(), OutCorrectedStates.Num());

	return true;
}

bool UN1RollbackEngine::IsRollbackRequired(float Divergence, float Threshold) const
{
	return Divergence > Threshold;
}

float UN1RollbackEngine::EstimateRollbackCost(int32 RollbackDepth, int32 EntityCount) const
{
	return BASE_ROLLBACK_COST_MS +
		   RollbackDepth * PER_FRAME_COST_MS +
		   EntityCount * PER_ENTITY_COST_MS;
}

float UN1RollbackEngine::GetAverageRollbackDepth() const
{
	return TotalRollbacks > 0 ? TotalRollbackDepth / TotalRollbacks : 0.0f;
}

void UN1RollbackEngine::RewindToTick(const FN1WorldSnapshot& Baseline, int32 TargetTick)
{
	UE_LOG(LogN1Rollback, VeryVerbose, TEXT("Rewinding to tick %d with %d baseline entities"),
		TargetTick, Baseline.EntityStates.Num());
}

FN1EntityState UN1RollbackEngine::ReplayInput(const FN1EntityState& CurrentState, const FN1InputFrame& Input)
{
	FN1EntityState Result = CurrentState;

	// Simplified replay — production would call the actual game simulation
	const FVector InputVec = Input.GetInputVector();
	const float DeltaTime = 1.0f / 120.0f; // Assuming 120Hz
	const FVector Vel = CurrentState.GetLinearVelocity() + InputVec * DeltaTime * 1000.0f;
	const FVector Pos = CurrentState.GetPosition() + Vel * DeltaTime;

	Result.SetPosition(Pos);
	Result.SetLinearVelocity(Vel);
	Result.InputSequence = Input.SequenceNumber;

	return Result;
}
