// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Pipeline/N1BlendInterpolator.h"

UN1BlendInterpolator::UN1BlendInterpolator()
{
}

void UN1BlendInterpolator::Initialize(int32 InBlendFrames, EN1BlendCurve InCurve)
{
	BlendFrames = FMath::Clamp(InBlendFrames, 1, 30);
	BlendCurve = InCurve;
	BlendProgress = 0.0f;
	bIsBlending = false;

	UE_LOG(LogN1Netcode, Log, TEXT("BlendInterpolator initialized: %d frames, curve=%s"),
		BlendFrames, *UEnum::GetValueAsString(BlendCurve));
}

void UN1BlendInterpolator::BeginBlend(const FN1EntityState& CorrectedState, const FN1EntityState& CurrentVisualState)
{
	BlendStartState = CurrentVisualState;
	BlendTargetState = CorrectedState;
	BlendProgress = 0.0f;
	bIsBlending = true;
}

FN1EntityState UN1BlendInterpolator::GetBlendedState(const FN1EntityState& CorrectedState) const
{
	if (!bIsBlending)
	{
		return CorrectedState;
	}

	const float T = ApplyCurve(BlendProgress);

	FN1EntityState Result;
	Result.EntityId = BlendTargetState.EntityId;
	Result.EntityType = BlendTargetState.EntityType;

	// Lerp position
	const FVector StartPos = BlendStartState.GetPosition();
	const FVector EndPos = BlendTargetState.GetPosition();
	Result.SetPosition(FMath::Lerp(StartPos, EndPos, T));

	// Lerp velocity
	const FVector StartVel = BlendStartState.GetLinearVelocity();
	const FVector EndVel = BlendTargetState.GetLinearVelocity();
	Result.SetLinearVelocity(FMath::Lerp(StartVel, EndVel, T));

	// Lerp rotation
	const FRotator StartRot = BlendStartState.GetRotation();
	const FRotator EndRot = BlendTargetState.GetRotation();
	Result.SetRotation(FMath::Lerp(StartRot, EndRot, T));

	Result.ServerTimestamp = BlendTargetState.ServerTimestamp;
	Result.InputSequence = BlendTargetState.InputSequence;

	return Result;
}

void UN1BlendInterpolator::Tick(float DeltaTime)
{
	if (!bIsBlending)
	{
		return;
	}

	// Advance blend progress
	const float ProgressPerFrame = 1.0f / BlendFrames;
	BlendProgress += ProgressPerFrame;

	if (BlendProgress >= 1.0f)
	{
		BlendProgress = 1.0f;
		bIsBlending = false;

		UE_LOG(LogN1Netcode, Verbose, TEXT("Blend complete"));
	}
}

float UN1BlendInterpolator::ApplyCurve(float T) const
{
	// Clamp input
	T = FMath::Clamp(T, 0.0f, 1.0f);

	switch (BlendCurve)
	{
	case EN1BlendCurve::Linear:
		return T;

	case EN1BlendCurve::SmoothStep:
		// Smoothstep: 3t^2 - 2t^3
		return T * T * (3.0f - 2.0f * T);

	case EN1BlendCurve::Exponential:
		// Exponential decay: 1 - e^(-5t)
		return 1.0f - FMath::Exp(-5.0f * T);

	case EN1BlendCurve::CriticalDamping:
		// Critical damping approximation
		return 1.0f - FMath::Exp(-8.0f * T) * (1.0f + 8.0f * T);

	default:
		return T;
	}
}
