// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1NetworkClock.generated.h"

/**
 * @class UN1NetworkClock
 * @brief High-precision network-synchronized clock.
 *
 * Provides time synchronization between client and server using
 * a modified Cristian's algorithm with jitter buffering. This is
 * the temporal foundation of the N+1 model — all N realities
 * must agree on a shared timeline for reconciliation to work.
 *
 * UE6 Note: When running under UE6, this automatically integrates
 * with the engine's NetworkTimeSubsystem if available.
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Network Clock"))
class NINEREALITIESNETCODE_API UN1NetworkClock : public UObject
{
	GENERATED_BODY()

public:
	UN1NetworkClock();

	/** Initialize the clock for client or server */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Clock")
	void Initialize(bool bIsServer);

	/** Process a time sync response from the server */
	void ProcessTimeSyncResponse(float ClientSendTime, float ServerReceiveTime, float ServerSendTime, float ClientReceiveTime);

	/** Get the current server-simulated time */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float GetServerTime() const;

	/** Get the time delta between local and server time */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float GetTimeDelta() const { return TimeDelta; }

	/** Get current RTT estimate */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float GetRTT() const { return CurrentRTT; }

	/** Get smoothed jitter estimate */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float GetJitter() const { return JitterEstimate; }

	/** Get the current tick number on the server timeline */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	int32 GetServerTick() const;

	/** Convert server time to local time */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float ServerToLocalTime(float ServerTime) const;

	/** Convert local time to server time */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	float LocalToServerTime(float LocalTime) const;

	/** Check if clock is sufficiently synchronized */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Clock")
	bool IsSynchronized() const { return bSynchronized; }

	/** Update the clock (call every frame) */
	void Tick(float DeltaTime);

private:
	/** Calculate time delta from sync samples */
	void RecalculateTimeDelta();

	/** Update jitter estimate from RTT variance */
	void UpdateJitter(float NewRTT);

private:
	bool bIsServer = false;
	bool bSynchronized = false;

	/** Time delta between local and server (LocalTime + Delta = ServerTime) */
	float TimeDelta = 0.0f;

	/** Smoothed RTT estimate */
	float CurrentRTT = 0.0f;

	/** Jitter estimate (standard deviation of RTT) */
	float JitterEstimate = 0.0f;

	/** Number of sync samples for confidence */
	int32 SyncSampleCount = 0;

	/** History of RTT samples for jitter calculation */
	TArray<float> RTTSamples;

	/** Server start time for tick calculation */
	float ServerStartTime = 0.0f;

	/** Tick rate for tick number calculation */
	float TickRate = 120.0f;

	/** Maximum RTT samples to keep */
	static constexpr int32 MAX_RTT_SAMPLES = 30;

	/** Required samples before declaring synchronized */
	static constexpr int32 REQUIRED_SYNC_SAMPLES = 8;

	/** RTT smoothing factor (exponential moving average) */
	static constexpr float RTT_SMOOTHING = 0.7f;
};
