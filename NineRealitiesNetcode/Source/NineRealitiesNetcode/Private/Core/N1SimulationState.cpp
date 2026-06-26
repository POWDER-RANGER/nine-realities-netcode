// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Core/N1SimulationState.h"
#include "Net/Core/PushModel/PushModel.h"

// Quantization precision: 0.01 units per integer unit
static constexpr float POSITION_QUANT = 100.0f;
static constexpr float ROTATION_QUANT = 182.0444f;  // 65536 / 360
static constexpr float VELOCITY_QUANT = 1000.0f;

// --- FN1EntityState ---

FVector FN1EntityState::GetPosition() const
{
	return FVector(
		QuantizedPosition.X / POSITION_QUANT,
		QuantizedPosition.Y / POSITION_QUANT,
		QuantizedPosition.Z / POSITION_QUANT
	);
}

void FN1EntityState::SetPosition(const FVector& Pos)
{
	QuantizedPosition.X = FMath::RoundToInt(Pos.X * POSITION_QUANT);
	QuantizedPosition.Y = FMath::RoundToInt(Pos.Y * POSITION_QUANT);
	QuantizedPosition.Z = FMath::RoundToInt(Pos.Z * POSITION_QUANT);
}

FRotator FN1EntityState::GetRotation() const
{
	return FRotator(
		QuantizedRotation.Y / ROTATION_QUANT,
		QuantizedRotation.Z / ROTATION_QUANT,
		QuantizedRotation.X / ROTATION_QUANT
	);
}

void FN1EntityState::SetRotation(const FRotator& Rot)
{
	QuantizedRotation.X = FMath::RoundToInt(Rot.Roll * ROTATION_QUANT);
	QuantizedRotation.Y = FMath::RoundToInt(Rot.Pitch * ROTATION_QUANT);
	QuantizedRotation.Z = FMath::RoundToInt(Rot.Yaw * ROTATION_QUANT);
}

FVector FN1EntityState::GetLinearVelocity() const
{
	return FVector(
		QuantizedLinearVelocity.X / VELOCITY_QUANT,
		QuantizedLinearVelocity.Y / VELOCITY_QUANT,
		QuantizedLinearVelocity.Z / VELOCITY_QUANT
	);
}

void FN1EntityState::SetLinearVelocity(const FVector& Vel)
{
	QuantizedLinearVelocity.X = FMath::RoundToInt(Vel.X * VELOCITY_QUANT);
	QuantizedLinearVelocity.Y = FMath::RoundToInt(Vel.Y * VELOCITY_QUANT);
	QuantizedLinearVelocity.Z = FMath::RoundToInt(Vel.Z * VELOCITY_QUANT);
}

void FN1EntityState::NetSerialize(FArchive& Ar)
{
	Ar << EntityId;
	Ar << DeltaMask;

	if (Ar.IsLoading())
	{
		// Read entity type
		FString TypeStr;
		Ar << TypeStr;
		EntityType = FName(*TypeStr);
	}
	else
	{
		FString TypeStr = EntityType.ToString();
		Ar << TypeStr;
	}

	Ar << ServerTimestamp;
	Ar << InputSequence;

	// Conditional serialization based on delta mask
	if (DeltaMask & 0x01) { Ar << QuantizedPosition.X; Ar << QuantizedPosition.Y; Ar << QuantizedPosition.Z; }
	if (DeltaMask & 0x02) { Ar << QuantizedRotation.X; Ar << QuantizedRotation.Y; Ar << QuantizedRotation.Z; }
	if (DeltaMask & 0x04) { Ar << QuantizedLinearVelocity.X; Ar << QuantizedLinearVelocity.Y; Ar << QuantizedLinearVelocity.Z; }
	if (DeltaMask & 0x08) { Ar << QuantizedAngularVelocity.X; Ar << QuantizedAngularVelocity.Y; Ar << QuantizedAngularVelocity.Z; }
}

float FN1EntityState::CalculateDivergence(const FN1EntityState& Other) const
{
	float PosDivergence = FVector::Dist(GetPosition(), Other.GetPosition());
	float VelDivergence = FVector::Dist(GetLinearVelocity(), Other.GetLinearVelocity());
	return PosDivergence + VelDivergence * 0.1f;
}

// --- FN1WorldSnapshot ---

void FN1WorldSnapshot::NetSerialize(FArchive& Ar, const FN1WorldSnapshot* Baseline)
{
	Ar << ServerTimestamp;
	Ar << SequenceNumber;
	Ar << bIsFullSnapshot;

	if (!bIsFullSnapshot && Baseline)
	{
		Ar << BaselineSequence;
	}

	int32 EntityCount = EntityStates.Num();
	Ar << EntityCount;

	if (Ar.IsLoading())
	{
		EntityStates.SetNum(EntityCount);
	}

	for (int32 i = 0; i < EntityCount; ++i)
	{
		if (!bIsFullSnapshot && Baseline)
		{
			// Delta compression: only serialize changed fields
			const FN1EntityState* BaselineState = Baseline->FindEntityState(EntityStates[i].EntityId);
			if (BaselineState)
			{
				// Calculate delta mask
				if (Ar.IsSaving())
				{
					EntityStates[i].DeltaMask = 0;
					if (EntityStates[i].QuantizedPosition != BaselineState->QuantizedPosition) EntityStates[i].DeltaMask |= 0x01;
					if (EntityStates[i].QuantizedRotation != BaselineState->QuantizedRotation) EntityStates[i].DeltaMask |= 0x02;
					if (EntityStates[i].QuantizedLinearVelocity != BaselineState->QuantizedLinearVelocity) EntityStates[i].DeltaMask |= 0x04;
				}
			}
		}
		EntityStates[i].NetSerialize(Ar);
	}
}

const FN1EntityState* FN1WorldSnapshot::FindEntityState(int32 EntityId) const
{
	for (const auto& State : EntityStates)
	{
		if (State.EntityId == EntityId)
		{
			return &State;
		}
	}
	return nullptr;
}

void FN1WorldSnapshot::SetEntityState(const FN1EntityState& State)
{
	for (auto& Existing : EntityStates)
	{
		if (Existing.EntityId == State.EntityId)
		{
			Existing = State;
			return;
		}
	}
	EntityStates.Add(State);
}

int32 FN1WorldSnapshot::GetSerializedSize() const
{
	// Rough estimate: header + entity count * average entity size
	return sizeof(float) + sizeof(uint32) * 2 + sizeof(bool) +
		   sizeof(int32) +
		   EntityStates.Num() * (sizeof(int32) + sizeof(uint16) + sizeof(float) + sizeof(uint32) +
		   3 * sizeof(int32) * 4);  // 4 quantized vectors
}

// --- FN1InputFrame ---

FVector FN1InputFrame::GetInputVector() const
{
	return FVector(
		QuantizedInputVector.X / POSITION_QUANT,
		QuantizedInputVector.Y / POSITION_QUANT,
		QuantizedInputVector.Z / POSITION_QUANT
	);
}

void FN1InputFrame::SetInputVector(const FVector& Vec)
{
	QuantizedInputVector.X = FMath::RoundToInt(Vec.X * POSITION_QUANT);
	QuantizedInputVector.Y = FMath::RoundToInt(Vec.Y * POSITION_QUANT);
	QuantizedInputVector.Z = FMath::RoundToInt(Vec.Z * POSITION_QUANT);
}

FRotator FN1InputFrame::GetCameraRotation() const
{
	return FRotator(
		QuantizedCameraRotation.Y / ROTATION_QUANT,
		QuantizedCameraRotation.Z / ROTATION_QUANT,
		QuantizedCameraRotation.X / ROTATION_QUANT
	);
}

void FN1InputFrame::SetCameraRotation(const FRotator& Rot)
{
	QuantizedCameraRotation.X = FMath::RoundToInt(Rot.Roll * ROTATION_QUANT);
	QuantizedCameraRotation.Y = FMath::RoundToInt(Rot.Pitch * ROTATION_QUANT);
	QuantizedCameraRotation.Z = FMath::RoundToInt(Rot.Yaw * ROTATION_QUANT);
}
