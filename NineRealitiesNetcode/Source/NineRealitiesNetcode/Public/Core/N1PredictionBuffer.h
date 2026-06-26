// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1PredictionBuffer.generated.h"

/**
 * @class UN1PredictionBuffer
 * @brief Ring buffer storing predicted states and inputs for rollback.
 *
 * Each client maintains a prediction buffer containing their local
 * simulation history. When a server snapshot arrives, the client
 * can rewind to the snapshot timestamp and replay all inputs since
 * then — this is the core of the rollback mechanism.
 *
 * The buffer is sized to accommodate the maximum expected latency
 * plus the maximum rollback depth.
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Prediction Buffer"))
class NINEREALITIESNETCODE_API UN1PredictionBuffer : public UObject
{
	GENERATED_BODY()

public:
	UN1PredictionBuffer();

	/** Initialize the buffer with given capacity */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Buffer")
	void Initialize(int32 MaxHistoryFrames, float InTickRate);

	/** Store a predicted state at the given tick */
	void StorePredictedState(int32 Tick, const FN1EntityState& State);

	/** Store an input frame at the given tick */
	void StoreInputFrame(int32 Tick, const FN1InputFrame& Input);

	/** Retrieve a predicted state by tick */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	bool GetPredictedState(int32 Tick, FN1EntityState& OutState) const;

	/** Retrieve an input frame by tick */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	bool GetInputFrame(int32 Tick, FN1InputFrame& OutInput) const;

	/** Get the oldest tick still in the buffer */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	int32 GetOldestTick() const;

	/** Get the most recent tick in the buffer */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	int32 GetNewestTick() const;

	/** Discard all entries older than the given tick */
	void DiscardOlderThan(int32 Tick);

	/** Check if we have a contiguous input history from StartTick to EndTick */
	bool HasContiguousInputs(int32 StartTick, int32 EndTick) const;

	/** Get the predicted states as an array for replay */
	TArray<FN1EntityState> GetStateRange(int32 StartTick, int32 EndTick) const;

	/** Get input frames as an array for replay */
	TArray<FN1InputFrame> GetInputRange(int32 StartTick, int32 EndTick) const;

	/** @return Current buffer size in frames */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	int32 GetBufferSize() const { return Buffer.Num(); }

	/** @return Memory footprint in bytes */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Buffer")
	int32 GetMemoryFootprint() const;

private:
	struct FBufferEntry
	{
		int32 Tick;
		FN1EntityState PredictedState;
		FN1InputFrame Input;
		bool bHasState = false;
		bool bHasInput = false;
	};

	TArray<FBufferEntry> Buffer;
	int32 MaxFrames = 180;
	float TickRate = 120.0f;
	int32 CurrentOldestTick = 0;
	int32 CurrentNewestTick = 0;
};
