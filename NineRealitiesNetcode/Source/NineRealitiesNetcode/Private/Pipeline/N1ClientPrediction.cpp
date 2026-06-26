// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Pipeline/N1ClientPrediction.h"
#include "Core/N1PredictionBuffer.h"
#include "Core/N1NetworkClock.h"

UN1ClientPrediction::UN1ClientPrediction()
{
}

void UN1ClientPrediction::Initialize(UN1PredictionBuffer* InBuffer, UN1NetworkClock* InClock, float InTickRate)
{
	Buffer = InBuffer;
	Clock = InClock;
	TickRate = InTickRate;
	TotalPredictions = 0;
	SuccessfulPredictions = 0;
	FailedPredictions = 0;

	UE_LOG(LogN1Prediction, Log, TEXT("ClientPrediction initialized @ %.0f Hz"), TickRate);
}

FN1EntityState UN1ClientPrediction::PredictLocalInput(const FN1InputFrame& Input, const FN1EntityState& CurrentState)
{
	FN1EntityState Predicted = CurrentState;

	const float Aggression = GetPredictionAggression();
	const FVector InputVec = Input.GetInputVector();
	const FRotator CamRot = Input.GetCameraRotation();

	// Apply input to predict next state
	// This is a simplified prediction — production would use the actual game simulation
	const float DeltaTime = 1.0f / TickRate;
	const FVector CurrentVel = CurrentState.GetLinearVelocity();
	const FVector NewVel = CurrentVel + InputVec * Aggression * DeltaTime * 1000.0f;
	const FVector NewPos = CurrentState.GetPosition() + NewVel * DeltaTime;

	Predicted.SetPosition(NewPos);
	Predicted.SetLinearVelocity(NewVel);
	Predicted.InputSequence = Input.SequenceNumber;

	// Store in prediction buffer
	if (Buffer)
	{
		const int32 CurrentTick = Clock ? Clock->GetServerTick() : TotalPredictions;
		Buffer->StorePredictedState(CurrentTick, Predicted);
		Buffer->StoreInputFrame(CurrentTick, Input);
	}

	TotalPredictions++;

	UE_LOG(LogN1Prediction, VeryVerbose, TEXT("Predict: tick=%d pos=%s vel=%s"),
		TotalPredictions, *NewPos.ToString(), *NewVel.ToString());

	return Predicted;
}

void UN1ClientPrediction::Tick(float DeltaTime)
{
	if (bAdaptiveEnabled && Clock)
	{
		UpdateAdaptiveMode(
			Clock->GetRTT() * 1000.0f,
			Clock->GetJitter() * 1000.0f,
			0.0f // Packet loss would come from a network stats provider
		);
	}
}

float UN1ClientPrediction::GetPredictionAccuracy() const
{
	return TotalPredictions > 0
		? (SuccessfulPredictions / (float)TotalPredictions) * 100.0f
		: 100.0f;
}

void UN1ClientPrediction::RecordSuccessfulPrediction()
{
	SuccessfulPredictions++;
}

void UN1ClientPrediction::RecordFailedPrediction()
{
	FailedPredictions++;
}

float UN1ClientPrediction::GetPredictionAggression() const
{
	switch (PredictionMode)
	{
	case EN1PredictionMode::Conservative:
		return 0.7f * AdaptiveAggression;
	case EN1PredictionMode::Balanced:
		return 1.0f * AdaptiveAggression;
	case EN1PredictionMode::Aggressive:
		return 1.4f * AdaptiveAggression;
	case EN1PredictionMode::Adaptive:
	default:
		return AdaptiveAggression;
	}
}

void UN1ClientPrediction::UpdateAdaptiveMode(float Latency, float Jitter, float Loss)
{
	// Adjust aggression based on connection quality
	// Lower aggression for poor connections to reduce corrections
	float TargetAggression = 1.0f;

	if (Latency > 100.0f || Jitter > 20.0f || Loss > 3.0f)
	{
		TargetAggression = 0.7f; // Conservative
	}
	else if (Latency < 40.0f && Jitter < 5.0f && Loss < 1.0f)
	{
		TargetAggression = 1.2f; // Aggressive
	}

	// Smooth transition
	AdaptiveAggression = FMath::Lerp(AdaptiveAggression, TargetAggression, 0.1f);
}
