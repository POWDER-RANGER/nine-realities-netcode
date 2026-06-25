// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 6 Forward Compatibility Layer

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1UE6Compatibility.generated.h"

/**
 * @file N1UE6Compatibility.h
 * @brief Forward-compatibility layer for Unreal Engine 6 migration.
 *
 * This module provides abstraction wrappers for UE5.5+ features that
 * will change in UE6, allowing the N1 netcode to compile against both
 * engine versions with minimal changes.
 *
 * When UE6 is released, toggle N1_UE6_BUILD in your build configuration
 * to switch to native UE6 APIs.
 */

#ifndef N1_UE6_BUILD
#define N1_UE6_BUILD 0
#endif

/**
 * @struct FN1UE6NetworkConfig
 * @brief UE6-native network configuration (prepared for future engine)
 *
 * UE6 introduces a new NetworkTransport API with pluggable congestion
 * control and built-in QoS tagging. This struct maps to the planned
 * UE6 FNetworkTransportConfig while maintaining UE5.5 compatibility.
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1UE6NetworkConfig
{
	GENERATED_BODY()

	/** Enable QUIC transport (UE6 feature — falls back to UDP on UE5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	bool bUseQUICTransport = false;

	/** Enable predictive packet pacing (UE6 feature) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	bool bPredictivePacing = false;

	/** Enable network state snapshots v2 (UE6 serialization) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	bool bNetworkSnapshotsV2 = false;

	/** Pluggable congestion control algorithm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	FName CongestionControlAlgorithm = FName("Cubic");

	/** QoS priority tags for different packet types */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	TMap<FName, uint8> QosPriorityTags;

	/** Enable the UE6 NetworkPrediction plugin integration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	bool bNetworkPredictionPlugin = false;
};

/**
 * @class UN1UE6Compatibility
 * @brief Runtime compatibility manager for UE5.5/UE6 dual targeting.
 *
 * Detects the runtime engine version and adapts behavior accordingly.
 * When running on UE6, uses native APIs. On UE5.5, uses polyfills.
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 UE6 Compatibility"))
class NINEREALITIESNETCODE_API UN1UE6Compatibility : public UObject
{
	GENERATED_BODY()

public:
	UN1UE6Compatibility();

	/** Initialize the compatibility layer */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|UE6")
	void Initialize();

	/** @return true if running on UE6 or later */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|UE6")
	bool IsUE6OrLater() const;

	/** @return true if the UE6 NetworkPrediction plugin is available */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|UE6")
	bool HasNetworkPredictionPlugin() const;

	/** @return The detected engine version string */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|UE6")
	FString GetEngineVersionString() const;

	/** Get UE6 network config (or best available fallback) */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode|UE6")
	const FN1UE6NetworkConfig& GetUE6Config() const { return UE6Config; }

	/** Apply UE6 config — validates compatibility */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|UE6")
	void ApplyUE6Config(const FN1UE6NetworkConfig& Config);

	/** Log compatibility status */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode|UE6")
	void LogCompatibilityStatus() const;

private:
	UPROPERTY()
	FN1UE6NetworkConfig UE6Config;

	bool bIsUE6Runtime = false;
	bool bNetworkPredictionAvailable = false;

	/** Engine version at compile time */
	FString DetectedEngineVersion;
};

/**
 * @def N1 UE6_SERIALIZATION_V2
 * @brief Conditional macro for UE6 network serialization format.
 *
 * When N1_UE6_BUILD is enabled, uses UE6's FNetworkBitWriterV2.
 * Otherwise, uses UE5.5's FBitWriter with custom N1 extensions.
 */
#if N1_UE6_BUILD
	#define N1_UE6_SERIALIZATION_V2 1
	#define N1_USE_NATIVE_UE6_CLOCK 1
#else
	#define N1_UE6_SERIALIZATION_V2 0
	#define N1_USE_NATIVE_UE6_CLOCK 0
#endif
