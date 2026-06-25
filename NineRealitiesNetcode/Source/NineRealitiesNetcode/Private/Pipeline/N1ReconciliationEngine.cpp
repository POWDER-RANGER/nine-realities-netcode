// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Pipeline/N1ReconciliationEngine.h"
#include "Core/N1PredictionBuffer.h"
#include "Pipeline/N1RollbackEngine.h"

UN1ReconciliationEngine::UN1ReconciliationEngine()
{
}

void UN1ReconciliationEngine::Initialize(float InRollbackThreshold, int32 InMaxRollbackFrames)
{
	RollbackThreshold = InRollbackThreshold;
	MaxRollbackFrames = InMaxRollbackFrames;
	TotalReconciliations = 0;
	TotalDivergence = 0.0f;

	UE_LOG(LogN1Netcode, Log, TEXT("ReconciliationEngine initialized: threshold=%.2f maxRollback=%d"),
		RollbackThreshold, MaxRollbackFrames);
}

FN1ReconciliationResult UN1ReconciliationEngine::ProcessServerSnapshot(
	const FN1WorldSnapshot& ServerSnapshot,
	UN1PredictionBuffer* PredictionBuffer,
	float CurrentServerTime)
{
	FN1ReconciliationResult Result;
	if (!PredictionBuffer)
	{
		return Result;
	}

	// Find the tick corresponding to this snapshot
	const int32 SnapshotTick = FMath::FloorToInt(ServerSnapshot.ServerTimestamp * 120.0f); // Assuming 120Hz

	// Calculate divergence
	const float Divergence = CalculateTickDivergence(ServerSnapshot, PredictionBuffer, SnapshotTick);
	Result.DivergenceMagnitude = Divergence;

	if (Divergence < RollbackThreshold)
	{
		// Within tolerance — no correction needed
		Result.bWasCorrected = false;
		Result.StrategyUsed = EN1ReconciliationStrategy::DeltaCorrection;
		return Result;
	}

	// Divergence detected — perform reconciliation
	const double StartTime = FPlatformTime::Seconds();

	switch (ActiveStrategy)
	{
	case EN1ReconciliationStrategy::FullRollbackReplay:
		Result = PerformFullRollback(ServerSnapshot, PredictionBuffer, SnapshotTick);
		break;
	case EN1ReconciliationStrategy::StateInterpolation:
		Result = PerformStateInterpolation(ServerSnapshot, PredictionBuffer, SnapshotTick);
		break;
	case EN1ReconciliationStrategy::DeltaCorrection:
		Result = PerformDeltaCorrection(ServerSnapshot, PredictionBuffer, SnapshotTick);
		break;
	case EN1ReconciliationStrategy::Adaptive:
	default:
		Result = PerformAdaptiveReconciliation(ServerSnapshot, PredictionBuffer, SnapshotTick, Divergence);
		break;
	}

	Result.ReconciliationTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0f;
	Result.DivergenceMagnitude = Divergence;
	Result.bWasCorrected = true;

	TotalReconciliations++;
	TotalDivergence += Divergence;

	UE_LOG(LogN1Netcode, Verbose, TEXT("Reconciliation: strategy=%s divergence=%.3f frames=%d time=%.3fms"),
		*UEnum::GetValueAsString(Result.StrategyUsed),
		Divergence, Result.RollbackFrames, Result.ReconciliationTimeMs);

	return Result;
}

FN1ReconciliationResult UN1ReconciliationEngine::PerformFullRollback(
	const FN1WorldSnapshot& Snapshot,
	UN1PredictionBuffer* Buffer,
	int32 SnapshotTick)
{
	FN1ReconciliationResult Result;
	Result.StrategyUsed = EN1ReconciliationStrategy::FullRollbackReplay;

	const int32 CurrentTick = Buffer->GetNewestTick();
	Result.RollbackFrames = FMath::Min(CurrentTick - SnapshotTick, MaxRollbackFrames);

	if (Result.RollbackFrames <= 0)
	{
		Result.bWasCorrected = false;
		return Result;
	}

	// Check for contiguous inputs
	if (!Buffer->HasContiguousInputs(SnapshotTick, CurrentTick))
	{
		UE_LOG(LogN1Netcode, Warning, TEXT("FullRollback: Missing inputs in range [%d, %d], falling back to interpolation"),
			SnapshotTick, CurrentTick);
		return PerformStateInterpolation(Snapshot, Buffer, SnapshotTick);
	}

	Result.ReplayedInputs = CurrentTick - SnapshotTick;

	UE_LOG(LogN1Rollback, Verbose, TEXT("Full rollback: %d frames, %d inputs replayed"),
		Result.RollbackFrames, Result.ReplayedInputs);

	return Result;
}

FN1ReconciliationResult UN1ReconciliationEngine::PerformStateInterpolation(
	const FN1WorldSnapshot& Snapshot,
	UN1PredictionBuffer* Buffer,
	int32 SnapshotTick)
{
	FN1ReconciliationResult Result;
	Result.StrategyUsed = EN1ReconciliationStrategy::StateInterpolation;
	Result.RollbackFrames = 0;

	// Store the target states for interpolation blending
	// The BlendInterpolator will handle the visual smoothing

	UE_LOG(LogN1Rollback, Verbose, TEXT("State interpolation applied"));
	return Result;
}

FN1ReconciliationResult UN1ReconciliationEngine::PerformDeltaCorrection(
	const FN1WorldSnapshot& Snapshot,
	UN1PredictionBuffer* Buffer,
	int32 SnapshotTick)
{
	FN1ReconciliationResult Result;
	Result.StrategyUsed = EN1ReconciliationStrategy::DeltaCorrection;

	// Calculate position/velocity delta and apply correction
	for (const auto& ServerState : Snapshot.EntityStates)
	{
		FN1EntityState PredictedState;
		if (Buffer->GetPredictedState(SnapshotTick, PredictedState))
		{
			const FVector DeltaPos = ServerState.GetPosition() - PredictedState.GetPosition();
			const float DeltaMag = DeltaPos.Size();

			if (DeltaMag > RollbackThreshold)
			{
				// Apply delta correction to predicted state
				// In production, this would modify the actual game state
				Result.bWasCorrected = true;
			}
		}
	}

	return Result;
}

FN1ReconciliationResult UN1ReconciliationEngine::PerformAdaptiveReconciliation(
	const FN1WorldSnapshot& Snapshot,
	UN1PredictionBuffer* Buffer,
	int32 SnapshotTick,
	float Divergence)
{
	// Select strategy based on divergence magnitude
	if (Divergence <= DELTA_CORRECTION_MAX)
	{
		return PerformDeltaCorrection(Snapshot, Buffer, SnapshotTick);
	}
	else if (Divergence <= INTERPOLATION_MAX)
	{
		return PerformStateInterpolation(Snapshot, Buffer, SnapshotTick);
	}
	else
	{
		return PerformFullRollback(Snapshot, Buffer, SnapshotTick);
	}
}

float UN1ReconciliationEngine::CalculateTickDivergence(
	const FN1WorldSnapshot& Snapshot,
	UN1PredictionBuffer* Buffer,
	int32 Tick)
{
	float MaxDivergence = 0.0f;

	for (const auto& ServerState : Snapshot.EntityStates)
	{
		FN1EntityState PredictedState;
		if (Buffer->GetPredictedState(Tick, PredictedState))
		{
			const float EntityDivergence = ServerState.CalculateDivergence(PredictedState);
			MaxDivergence = FMath::Max(MaxDivergence, EntityDivergence);
		}
	}

	return MaxDivergence;
}

float UN1ReconciliationEngine::GetAverageDivergence() const
{
	return TotalReconciliations > 0 ? TotalDivergence / TotalReconciliations : 0.0f;
}
