// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// UE6 Forward Compatibility Layer

#include "UE6/N1UE6Compatibility.h"
#include "Misc/EngineVersion.h"
#include "Interfaces/IPluginManager.h"

UN1UE6Compatibility::UN1UE6Compatibility()
{
}

void UN1UE6Compatibility::Initialize()
{
	// Detect runtime engine version
	DetectedEngineVersion = FEngineVersion::Current().ToString();

	// Check if running on UE6+ (major version >= 6)
	const int32 MajorVersion = FEngineVersion::Current().GetMajor();
	bIsUE6Runtime = (MajorVersion >= 6);

	// Check for UE6 NetworkPrediction plugin
	IPluginManager& PluginManager = IPluginManager::Get();
	bNetworkPredictionAvailable = PluginManager.FindPlugin("NetworkPrediction").IsValid();

	// Set default QoS tags
	UE6Config.QosPriorityTags.Add(FName("Input"), 255);
	UE6Config.QosPriorityTags.Add(FName("Snapshot"), 200);
	UE6Config.QosPriorityTags.Add(FName("Heartbeat"), 100);
	UE6Config.QosPriorityTags.Add(FName("Chat"), 50);

	UE_LOG(LogN1UE6Compat, Log, TEXT("UE6 Compatibility Layer initialized"));
	UE_LOG(LogN1UE6Compat, Log, TEXT("  Engine: %s"), *DetectedEngineVersion);
	UE_LOG(LogN1UE6Compat, Log, TEXT("  UE6 Runtime: %s"), bIsUE6Runtime ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogN1UE6Compat, Log, TEXT("  NetworkPrediction Plugin: %s"), bNetworkPredictionAvailable ? TEXT("Available") : TEXT("Not Available"));
}

bool UN1UE6Compatibility::IsUE6OrLater() const
{
	return bIsUE6Runtime;
}

bool UN1UE6Compatibility::HasNetworkPredictionPlugin() const
{
	return bNetworkPredictionAvailable;
}

FString UN1UE6Compatibility::GetEngineVersionString() const
{
	return DetectedEngineVersion;
}

void UN1UE6Compatibility::ApplyUE6Config(const FN1UE6NetworkConfig& Config)
{
	UE6Config = Config;

	// Validate settings based on runtime
	if (!bIsUE6Runtime)
	{
		if (UE6Config.bUseQUICTransport)
		{
			UE_LOG(LogN1UE6Compat, Warning, TEXT("QUIC transport requested but not available on UE5 — disabling"));
			UE6Config.bUseQUICTransport = false;
		}

		if (UE6Config.bNetworkSnapshotsV2)
		{
			UE_LOG(LogN1UE6Compat, Warning, TEXT("Network Snapshots V2 requested but not available on UE5 — using compatibility mode"));
			UE6Config.bNetworkSnapshotsV2 = false;
		}
	}

	if (!bNetworkPredictionAvailable && UE6Config.bNetworkPredictionPlugin)
	{
		UE_LOG(LogN1UE6Compat, Warning, TEXT("NetworkPrediction plugin integration requested but plugin not found — disabling"));
		UE6Config.bNetworkPredictionPlugin = false;
	}

	UE_LOG(LogN1UE6Compat, Log, TEXT("UE6 config applied"));
}

void UN1UE6Compatibility::LogCompatibilityStatus() const
{
	UE_LOG(LogN1Netcode, Log, TEXT("--- N1 UE6 Compatibility Status ---"));
	UE_LOG(LogN1Netcode, Log, TEXT("Engine Version: %s"), *DetectedEngineVersion);
	UE_LOG(LogN1Netcode, Log, TEXT("UE6+ Runtime: %s"), bIsUE6Runtime ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogN1Netcode, Log, TEXT("NetworkPrediction Plugin: %s"), bNetworkPredictionAvailable ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogN1Netcode, Log, TEXT("QUIC Transport: %s"), UE6Config.bUseQUICTransport ? TEXT("Enabled") : TEXT("Disabled"));
	UE_LOG(LogN1Netcode, Log, TEXT("Snapshots V2: %s"), UE6Config.bNetworkSnapshotsV2 ? TEXT("Enabled") : TEXT("Disabled"));
	UE_LOG(LogN1Netcode, Log, TEXT("------------------------------------"));
}
