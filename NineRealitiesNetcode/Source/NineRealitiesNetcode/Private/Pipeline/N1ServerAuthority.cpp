// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Pipeline/N1ServerAuthority.h"
#include "Core/N1NetworkClock.h"

UN1ServerAuthority::UN1ServerAuthority()
{
	Clients.Reserve(64);
}

void UN1ServerAuthority::Initialize(float InTickRate, float InInputBufferMs)
{
	TickRate = InTickRate;
	InputBufferMs = InInputBufferMs;
	TotalTicks = 0;

	UE_LOG(LogN1Netcode, Log, TEXT("ServerAuthority initialized: %.0f Hz, %.0fms input buffer"),
		TickRate, InputBufferMs);
}

void UN1ServerAuthority::ProcessClientInput(int32 ClientId, const FN1InputFrame& Input)
{
	FN1ClientConnection* Client = Clients.Find(ClientId);
	if (!Client)
	{
		UE_LOG(LogN1Netcode, Warning, TEXT("Received input from unregistered client %d"), ClientId);
		return;
	}

	// Validate sequence number (detect out-of-order inputs)
	if (Input.SequenceNumber <= Client->LastInputSequence)
	{
		UE_LOG(LogN1Netcode, Verbose, TEXT("Out-of-order input from client %d: seq %d <= last %d"),
			ClientId, Input.SequenceNumber, Client->LastInputSequence);
		return;
	}

	// Validate input (anti-cheat: reject physically impossible inputs)
	if (!ValidateInput(Input, ClientId))
	{
		UE_LOG(LogN1Netcode, Warning, TEXT("Invalid input rejected from client %d"), ClientId);
		return;
	}

	Client->LastInputSequence = Input.SequenceNumber;
	Client->LastInputTime = Input.ClientTimestamp;

	// Store in input history for lag compensation
	Client->InputHistory.Add(Input);
	if (Client->InputHistory.Num() > MAX_STATE_HISTORY)
	{
		Client->InputHistory.RemoveAt(0);
	}

	// Update smoothed RTT
	const float InputRTT = FMath::Max(0.0f, Input.ClientTimestamp - Client->LastInputTime);
	Client->SmoothedRTT = FMath::Lerp(Client->SmoothedRTT, InputRTT, 0.3f);
}

void UN1ServerAuthority::TickServer(float DeltaTime)
{
	TotalTicks++;

	// Update current authoritative state timestamp
	CurrentAuthoritativeState.ServerTimestamp = TotalTicks / TickRate;
	CurrentAuthoritativeState.SequenceNumber = (uint32)TotalTicks;

	// Update adaptive snapshot rates if enabled
	if (bAdaptiveSnapshots && TotalTicks % TickRate == 0)
	{
		UpdateAdaptiveSnapshotRates();
	}

	// Trim state history
	if (StateHistory.Num() > MAX_STATE_HISTORY)
	{
		StateHistory.RemoveAt(0);
	}
}

FN1WorldSnapshot UN1ServerAuthority::GenerateSnapshotForClient(int32 ClientId) const
{
	FN1WorldSnapshot Snapshot = CurrentAuthoritativeState;

	const FN1ClientConnection* Client = Clients.Find(ClientId);
	if (Client)
	{
		Client->LastAcknowledgedSnapshot = Snapshot.SequenceNumber;
	}

	return Snapshot;
}

void UN1ServerAuthority::RegisterClient(int32 ClientId)
{
	if (Clients.Contains(ClientId))
	{
		UE_LOG(LogN1Netcode, Warning, TEXT("Client %d already registered"), ClientId);
		return;
	}

	FN1ClientConnection NewClient;
	NewClient.ClientId = ClientId;
	Clients.Add(ClientId, NewClient);

	UE_LOG(LogN1Netcode, Log, TEXT("Client %d registered. Total clients: %d"), ClientId, Clients.Num());
}

void UN1ServerAuthority::UnregisterClient(int32 ClientId)
{
	Clients.Remove(ClientId);
	UE_LOG(LogN1Netcode, Log, TEXT("Client %d unregistered. Total clients: %d"), ClientId, Clients.Num());
}

const FN1ClientConnection* UN1ServerAuthority::GetClientConnection(int32 ClientId) const
{
	return Clients.Find(ClientId);
}

void UN1ServerAuthority::UpdateAdaptiveSnapshotRates()
{
	for (auto& Pair : Clients)
	{
		FN1ClientConnection& Client = Pair.Value;

		// Adjust snapshot rate based on RTT
		if (Client.SmoothedRTT > 0.15f) // >150ms
		{
			Client.AdaptiveSnapshotRate = 30; // Lower rate for high latency
		}
		else if (Client.SmoothedRTT > 0.08f) // >80ms
		{
			Client.AdaptiveSnapshotRate = 45;
		}
		else
		{
			Client.AdaptiveSnapshotRate = FMath::FloorToInt(TickRate);
		}
	}
}

FN1WorldSnapshot UN1ServerAuthority::RewindStateForClient(int32 ClientId, float TargetTime) const
{
	// Lag compensation: find the state closest to the target time
	FN1WorldSnapshot Result = CurrentAuthoritativeState;

	float BestDelta = MAX_FLT;
	for (const auto& HistoricalState : StateHistory)
	{
		const float Delta = FMath::Abs(HistoricalState.ServerTimestamp - TargetTime);
		if (Delta < BestDelta)
		{
			BestDelta = Delta;
			Result = HistoricalState;
		}
	}

	return Result;
}

bool UN1ServerAuthority::ValidateInput(const FN1InputFrame& Input, int32 ClientId) const
{
	// Anti-cheat: validate that input is physically possible
	const FVector InputVec = Input.GetInputVector();
	if (InputVec.SizeSquared() > 1.5f * 1.5f) // Input magnitude check
	{
		return false;
	}

	// Additional validation would go here (speed checks, teleport detection, etc.)
	return true;
}
