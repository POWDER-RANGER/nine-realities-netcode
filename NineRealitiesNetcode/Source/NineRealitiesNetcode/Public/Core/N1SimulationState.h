// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.generated.h"

/**
 * @struct FN1EntityState
 * @brief Serialized state for a single entity in the simulation.
 *
 * Compact representation optimized for network serialization.
 * Uses quantized vectors for bandwidth efficiency.
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1EntityState
{
	GENERATED_BODY()

	/** Unique entity identifier */
	UPROPERTY()
	int32 EntityId = INDEX_NONE;

	/** Entity type tag for classification */
	UPROPERTY()
	FName EntityType;

	/** Quantized position (0.01 unit precision) */
	UPROPERTY()
	FIntVector QuantizedPosition;

	/** Quantized rotation (compressed to 3x int16) */
	UPROPERTY()
	FIntVector QuantizedRotation;

	/** Quantized linear velocity */
	UPROPERTY()
	FIntVector QuantizedLinearVelocity;

	/** Quantized angular velocity */
	UPROPERTY()
	FIntVector QuantizedAngularVelocity;

	/** Server timestamp when this state was authoritative */
	UPROPERTY()
	float ServerTimestamp = 0.0f;

	/** Input sequence number that produced this state */
	UPROPERTY()
	uint32 InputSequence = 0;

	/** Bitmask of changed properties (delta compression) */
	UPROPERTY()
	uint16 DeltaMask = 0xFFFF;

	/** Get dequantized position */
	FVector GetPosition() const;

	/** Set position with quantization */
	void SetPosition(const FVector& Pos);

	/** Get dequantized rotation */
	FRotator GetRotation() const;

	/** Set rotation with quantization */
	void SetRotation(const FRotator& Rot);

	/** Get dequantized linear velocity */
	FVector GetLinearVelocity() const;

	/** Set linear velocity with quantization */
	void SetLinearVelocity(const FVector& Vel);

	/** Serialize to network bit writer */
	void NetSerialize(FArchive& Ar);

	/** Calculate divergence from another state */
	float CalculateDivergence(const FN1EntityState& Other) const;

	/** Empty / default state check */
	bool IsValid() const { return EntityId != INDEX_NONE; }
};

/**
 * @struct FN1WorldSnapshot
 * @brief Complete world state snapshot from the server.
 *
 * Contains all entity states at a given server timestamp.
 * Supports delta compression against a baseline snapshot.
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1WorldSnapshot
{
	GENERATED_BODY()

	/** Server timestamp for this snapshot */
	UPROPERTY()
	float ServerTimestamp = 0.0f;

	/** Sequence number for ordering */
	UPROPERTY()
	uint32 SequenceNumber = 0;

	/** All entity states in this snapshot */
	UPROPERTY()
	TArray<FN1EntityState> EntityStates;

	/** Baseline sequence for delta compression */
	UPROPERTY()
	uint32 BaselineSequence = 0;

	/** Is this a full snapshot or delta? */
	UPROPERTY()
	bool bIsFullSnapshot = true;

	/** Serialize with optional delta compression */
	void NetSerialize(FArchive& Ar, const FN1WorldSnapshot* Baseline = nullptr);

	/** Find state for a specific entity */
	const FN1EntityState* FindEntityState(int32 EntityId) const;

	/** Add or update an entity state */
	void SetEntityState(const FN1EntityState& State);

	/** Get serialized size in bytes (for bandwidth estimation) */
	int32 GetSerializedSize() const;
};

/**
 * @struct FN1InputFrame
 * @brief Client input at a specific tick.
 *
 * Minimal representation for efficient network transmission.
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1InputFrame
{
	GENERATED_BODY()

	/** Monotonically increasing sequence number */
	UPROPERTY()
	uint32 SequenceNumber = 0;

	/** Client timestamp when input was generated */
	UPROPERTY()
	float ClientTimestamp = 0.0f;

	/** Quantized input vector (movement) */
	UPROPERTY()
	FIntVector QuantizedInputVector;

	/** Input action bitmask (jump, shoot, etc) */
	UPROPERTY()
	uint32 InputActions = 0;

	/** Camera rotation (compressed) */
	UPROPERTY()
	FIntVector QuantizedCameraRotation;

	/** Predicted result hash for server validation */
	UPROPERTY()
	uint32 PredictionHash = 0;

	FVector GetInputVector() const;
	void SetInputVector(const FVector& Vec);
	FRotator GetCameraRotation() const;
	void SetCameraRotation(const FRotator& Rot);
};

/**
 * @struct FN1SimulationMetrics
 * @brief Real-time performance metrics for the N+1 simulation.
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1SimulationMetrics
{
	GENERATED_BODY()

	/** Current round-trip time in ms */
	UPROPERTY(BlueprintReadOnly)
	float RTT = 0.0f;

	/** One-way latency estimate (RTT/2) */
	UPROPERTY(BlueprintReadOnly)
	float LatencyMs = 0.0f;

	/** Jitter (standard deviation of latency) */
	UPROPERTY(BlueprintReadOnly)
	float JitterMs = 0.0f;

	/** Packet loss percentage (0-100) */
	UPROPERTY(BlueprintReadOnly)
	float PacketLossPercent = 0.0f;

	/** Predictions per second */
	UPROPERTY(BlueprintReadOnly)
	float PredictionRate = 0.0f;

	/** Corrections per second */
	UPROPERTY(BlueprintReadOnly)
	float CorrectionRate = 0.0f;

	/** Prediction accuracy (0-100%) */
	UPROPERTY(BlueprintReadOnly)
	float PredictionAccuracy = 100.0f;

	/** Average rollback depth in frames */
	UPROPERTY(BlueprintReadOnly)
	float AvgRollbackDepth = 0.0f;

	/** Current bandwidth usage downstream (kbps) */
	UPROPERTY(BlueprintReadOnly)
	float BandwidthDownKbps = 0.0f;

	/** Current bandwidth usage upstream (kbps) */
	UPROPERTY(BlueprintReadOnly)
	float BandwidthUpKbps = 0.0f;

	/** Client frametime spent in netcode (ms) */
	UPROPERTY(BlueprintReadOnly)
	float NetcodeFrameTimeMs = 0.0f;

	/** Server tick time spent in netcode (ms) */
	UPROPERTY(BlueprintReadOnly)
	float ServerNetcodeTimeMs = 0.0f;

	/** Current extrapolation time average */
	UPROPERTY(BlueprintReadOnly)
	float ExtrapolationTimeMs = 0.0f;

	/** Floating-point drift magnitude */
	UPROPERTY(BlueprintReadOnly)
	float FloatDriftMagnitude = 0.0f;
};
