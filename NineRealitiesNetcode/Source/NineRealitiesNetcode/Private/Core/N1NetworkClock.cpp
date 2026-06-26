// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Core/N1NetworkClock.h"
#include "Engine/Engine.h"

UN1NetworkClock::UN1NetworkClock()
{
	RTTSamples.Reserve(MAX_RTT_SAMPLES);
}

void UN1NetworkClock::Initialize(bool bInIsServer)
{
	bIsServer = bInIsServer;
	RTTSamples.Reset();

	if (bIsServer)
	{
		ServerStartTime = GEngine ? GEngine->GetCurrentPlayTimeInSeconds() : 0.0f;
		bSynchronized = true;
		TimeDelta = 0.0f;
		UE_LOG(LogN1Netcode, Log, TEXT("NetworkClock initialized in SERVER mode"));
	}
	else
	{
		bSynchronized = false;
		TimeDelta = 0.0f;
		UE_LOG(LogN1Netcode, Log, TEXT("NetworkClock initialized in CLIENT mode (awaiting sync)"));
	}
}

void UN1NetworkClock::ProcessTimeSyncResponse(float ClientSendTime, float ServerReceiveTime, float ServerSendTime, float ClientReceiveTime)
{
	if (bIsServer) return;

	const float RTT = ClientReceiveTime - ClientSendTime;
	const float ServerProcessingTime = ServerSendTime - ServerReceiveTime;
	const float OneWayLatency = (RTT - ServerProcessingTime) * 0.5f;

	// Add sample
	RTTSamples.Add(RTT);
	if (RTTSamples.Num() > MAX_RTT_SAMPLES)
	{
		RTTSamples.RemoveAt(0);
	}

	// Update jitter
	UpdateJitter(RTT);

	// Recalculate time delta using Cristian's algorithm
	const float EstimatedServerTimeAtReceipt = ServerSendTime + OneWayLatency;
	const float NewDelta = EstimatedServerTimeAtReceipt - ClientReceiveTime;

	if (!bSynchronized)
	{
		// First sync: use directly
		TimeDelta = NewDelta;
		CurrentRTT = RTT;
	}
	else
	{
		// Subsequent syncs: exponential moving average
		TimeDelta = FMath::Lerp(TimeDelta, NewDelta, RTT_SMOOTHING);
		CurrentRTT = FMath::Lerp(CurrentRTT, RTT, RTT_SMOOTHING);
	}

	SyncSampleCount++;

	if (SyncSampleCount >= REQUIRED_SYNC_SAMPLES)
	{
		bSynchronized = true;
	}

	UE_LOG(LogN1Netcode, Verbose, TEXT("TimeSync: RTT=%.2fms Latency=%.2fms Delta=%.4f Sync=%d/%d"),
		RTT * 1000.0f, OneWayLatency * 1000.0f, TimeDelta, SyncSampleCount, REQUIRED_SYNC_SAMPLES);
}

float UN1NetworkClock::GetServerTime() const
{
	if (bIsServer)
	{
		return GEngine ? GEngine->GetCurrentPlayTimeInSeconds() : 0.0f;
	}
	return GetLocalTime() + TimeDelta;
}

int32 UN1NetworkClock::GetServerTick() const
{
	return FMath::FloorToInt((GetServerTime() - ServerStartTime) * TickRate);
}

float UN1NetworkClock::ServerToLocalTime(float ServerTime) const
{
	return ServerTime - TimeDelta;
}

float UN1NetworkClock::LocalToServerTime(float LocalTime) const
{
	return LocalTime + TimeDelta;
}

void UN1NetworkClock::Tick(float DeltaTime)
{
	// Periodic sync check — in production, would trigger time sync requests
	if (!bIsServer && !bSynchronized && SyncSampleCount < REQUIRED_SYNC_SAMPLES)
	{
		// Request another time sync sample
	}
}

void UN1NetworkClock::RecalculateTimeDelta()
{
	if (RTTSamples.Num() < 2) return;

	// Use median of recent samples for stability
	TArray<float> Sorted = RTTSamples;
	Sorted.Sort();
	const float MedianRTT = Sorted[Sorted.Num() / 2];
	CurrentRTT = MedianRTT;
}

void UN1NetworkClock::UpdateJitter(float NewRTT)
{
	if (RTTSamples.Num() < 2) return;

	// Calculate standard deviation of RTT samples
	float Sum = 0.0f;
	for (float Sample : RTTSamples)
	{
		Sum += Sample;
	}
	const float Mean = Sum / RTTSamples.Num();

	float Variance = 0.0f;
	for (float Sample : RTTSamples)
	{
		Variance += FMath::Square(Sample - Mean);
	}
	Variance /= RTTSamples.Num();
	JitterEstimate = FMath::Sqrt(Variance);
}

float UN1NetworkClock::GetLocalTime() const
{
	return GEngine ? GEngine->GetCurrentPlayTimeInSeconds() : 0.0f;
}
