// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1ServerAuthority.generated.h"

class UN1NetworkClock;

/**
 * @struct FN1ClientConnection
 * @brief Server-side tracking data for each connected client
 */
USTRUCT()
struct FN1ClientConnection
{
	GENERATED_BODY()

	/** Network connection identifier */
	UPROPERTY()
	int32 ClientId = INDEX_NONE;

	/** Last received input sequence */
	UPROPERTY()
	uint32 LastInputSequence = 0;

	/** Last acknowledged snapshot sequence */
	UPROPERTY()
	uint32 LastAcknowledgedSnapshot = 0;

	/** Smoothed RTT */
	UPROPERTY()
	float SmoothedRTT = 0.0f;

	/** Per-client snapshot rate (adaptive) */
	UPROPERTY()
	int32 AdaptiveSnapshotRate = 60;

	/** Input buffer for lag compensation */
	UPROPERTY()
	TArray<FN1InputFrame> InputHistory;

	/** Last input timestamp */
	UPROPERTY()
	float LastInputTime = 0.0f;
};

/**
 * @class UN1ServerAuthority
 * @brief Server-side authoritative simulation for the +1 reality.
 *
 * The server maintains the single source of truth. It collects inputs
 * from all N clients, runs the authoritative simulation, and broadcasts
 * snapshots. It also handles lag compensation by rewinding state for
 * hit validation.
 *
 * Key principle: "The server reconstructs the past to validate fairness."
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Server Authority"))
class NINEREALITIESNETCODE_API UN1ServerAuthority : public UObject
{
	GENERATED_BODY()

public:
	UN1ServerAuthority();

	/** Initialize the server authority */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void Initialize(float InTickRate, float InInputBufferMs);

	/** Process an input frame from a client */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void ProcessClientInput(int32 ClientId, const FN1InputFrame& Input);

	/** Run the authoritative server tick */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void TickServer(float DeltaTime);

	/** Generate a world snapshot for a specific client */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	FN1WorldSnapshot GenerateSnapshotForClient(int32 ClientId) const;

	/** Register a new client connection */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void RegisterClient(int32 ClientId);

	/** Remove a client connection */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void UnregisterClient(int32 ClientId);

	/** Get client connection data */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Server")
	const FN1ClientConnection* GetClientConnection(int32 ClientId) const;

	/** Set adaptive snapshot rates enabled */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|Server")
	void SetAdaptiveSnapshots(bool bEnabled) { bAdaptiveSnapshots = bEnabled; }

	/** @return Current number of connected clients */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Server")
	int32 GetClientCount() const { return Clients.Num(); }

	/** @return Total server ticks processed */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|Server")
	int32 GetTotalTicks() const { return TotalTicks; }

private:
	/** Update adaptive snapshot rates per client */
	void UpdateAdaptiveSnapshotRates();

	/** Rewind state for lag compensation */
	FN1WorldSnapshot RewindStateForClient(int32 ClientId, float TargetTime) const;

	/** Validate that an input is physically possible */
	bool ValidateInput(const FN1InputFrame& Input, int32 ClientId) const;

private:
	UPROPERTY()
	TMap<int32, FN1ClientConnection> Clients;

	UPROPERTY()
	FN1WorldSnapshot CurrentAuthoritativeState;

	UPROPERTY()
	TArray<FN1WorldSnapshot> StateHistory;

	float TickRate = 120.0f;
	float InputBufferMs = 100.0f;
	int32 TotalTicks = 0;
	bool bAdaptiveSnapshots = true;

	/** Maximum state history to keep (for lag compensation) */
	static constexpr int32 MAX_STATE_HISTORY = 600;
};
