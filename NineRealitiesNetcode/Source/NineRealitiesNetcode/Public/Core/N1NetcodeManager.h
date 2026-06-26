// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "N1SimulationState.h"
#include "N1NetworkClock.h"
#include "N1NetcodeManager.generated.h"

class UN1ReconciliationEngine;
class UN1ClientPrediction;
class UN1ServerAuthority;
class UN1RollbackEngine;
class UN1BlendInterpolator;

/**
 * @enum EN1NetcodeMode
 * @brief Operating mode for the N+1 netcode manager
 */
UENUM(BlueprintType)
enum class EN1NetcodeMode : uint8
{
	/** Standalone / local simulation */
	Standalone	UMETA(DisplayName = "Standalone"),
	/** Client mode with prediction enabled */
	Client		UMETA(DisplayName = "Client"),
	/** Dedicated server with full authority */
	Server		UMETA(DisplayName = "Server"),
	/** Listen server (host) - acts as both client and server */
	ListenServer UMETA(DisplayName = "Listen Server")
};

/**
 * @enum EN1NetcodePhase
 * @brief Current operational phase of the netcode system
 */
UENUM(BlueprintType)
enum class EN1NetcodePhase : uint8
{
	/** System is initializing */
	Initializing,
	/** Handshake and time sync in progress */
	Handshaking,
	/** Time sync complete, awaiting first snapshot */
	Syncing,
	/** Fully operational - predicting and reconciling */
	Active,
	/** High packet loss detected, running in degraded mode */
	Degraded,
	/** Connection lost, attempting recovery */
	Recovering,
	/** Shutting down */
	ShuttingDown
};

/**
 * @struct FN1NetcodeConfig
 * @brief Configuration parameters for the N+1 simulation
 */
USTRUCT(BlueprintType)
struct NINEREALITIESNETCODE_API FN1NetcodeConfig
{
	GENERATED_BODY()

	/** Target client tick rate (Hz) - typical range 60-144 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Tick")
	int32 ClientTickRate = 120;

	/** Server tick rate (Hz) - typical range 20-128 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Tick")
	int32 ServerTickRate = 120;

	/** Snapshot send rate from server (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Tick")
	int32 SnapshotRate = 60;

	/** Input buffer window in milliseconds for lag compensation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Timing")
	float InputBufferMs = 100.0f;

	/** Interpolation delay in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Timing")
	float InterpolationDelayMs = 33.0f;

	/** Maximum rollback depth in frames before forced resync */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Rollback")
	int32 MaxRollbackFrames = 16;

	/** Divergence threshold for triggering rollback (units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Rollback")
	float RollbackThreshold = 2.5f;

	/** Number of frames to blend corrections over */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Blend")
	int32 BlendFrames = 5;

	/** Maximum acceptable latency in ms before degradation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Network")
	float MaxAcceptableLatencyMs = 150.0f;

	/** Forced reconciliation interval in seconds (0 = disabled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Network")
	float ForcedReconciliationIntervalSec = 15.0f;

	/** Enable adaptive snapshot rates based on per-client RTT */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Adaptive")
	bool bAdaptiveSnapshotRate = true;

	/** Enable prediction smoothing for high-latency clients */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|Adaptive")
	bool bAdaptivePrediction = true;

	/** UE6: Enable network serialization v2 format (forward compatible) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "N1 Netcode|UE6")
	bool bUE6NetworkFormat = false;
};

/**
 * @class UN1NetcodeManager
 * @brief Central orchestrator for the N+1 concurrent simulation model.
 *
 * Manages N client-local predicted simulations plus one server-authoritative
 * simulation. Each client predicts the future while the server reconstructs
 * the past — the "truth" emerges through continuous reconciliation.
 *
 * @note Singleton pattern — use GetN1Manager() for access
 */
UCLASS(ClassGroup = (N1Netcode), meta = (DisplayName = "N1 Netcode Manager"))
class NINEREALITIESNETCODE_API UN1NetcodeManager : public UObject
{
	GENERATED_BODY()

public:
	UN1NetcodeManager();

	/** Initialize the netcode manager with configuration */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode")
	void Initialize(const FN1NetcodeConfig& InConfig, EN1NetcodeMode InMode);

	/** Shutdown and cleanup all netcode systems */
	UFUNCTION(BlueprintCallable, Category = "N1 Netcode")
	void Shutdown();

	/** Called every frame to update the netcode pipeline */
	void Tick(float DeltaTime);

	/** @return Current netcode operating mode */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	EN1NetcodeMode GetNetcodeMode() const { return CurrentMode; }

	/** @return Current operational phase */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	EN1NetcodePhase GetPhase() const { return CurrentPhase; }

	/** @return Current configuration (read-only) */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	const FN1NetcodeConfig& GetConfig() const { return Config; }

	/** @return The reconciliation engine */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	UN1ReconciliationEngine* GetReconciliationEngine() const { return ReconciliationEngine; }

	/** @return The prediction system */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	UN1ClientPrediction* GetPrediction() const { return PredictionSystem; }

	/** @return The rollback engine */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	UN1RollbackEngine* GetRollbackEngine() const { return RollbackEngine; }

	/** @return The blend interpolator */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	UN1BlendInterpolator* GetBlendInterpolator() const { return BlendInterpolator; }

	/** @return The shared network clock */
	UFUNCTION(BlueprintPure, Category = "N1 Netcode")
	UN1NetworkClock* GetNetworkClock() const { return NetworkClock; }

	/** Get the singleton instance */
	static UN1NetcodeManager* GetN1Manager(UWorld* World);

	/** Event dispatcher for phase changes */
	UPROPERTY(BlueprintAssignable, Category = "N1 Netcode|Events")
	FOnN1PhaseChanged OnPhaseChanged;

	/** Event dispatcher for divergence detected */
	UPROPERTY(BlueprintAssignable, Category = "N1 Netcode|Events")
	FOnN1DivergenceDetected OnDivergenceDetected;

private:
	/** Transition to a new operational phase */
	void SetPhase(EN1NetcodePhase NewPhase);

	/** Perform forced periodic reconciliation */
	void PerformForcedReconciliation(float DeltaTime);

private:
	UPROPERTY()
	FN1NetcodeConfig Config;

	UPROPERTY()
	EN1NetcodeMode CurrentMode;

	UPROPERTY()
	EN1NetcodePhase CurrentPhase;

	UPROPERTY()
	TObjectPtr<UN1ReconciliationEngine> ReconciliationEngine;

	UPROPERTY()
	TObjectPtr<UN1ClientPrediction> PredictionSystem;

	UPROPERTY()
	TObjectPtr<UN1ServerAuthority> ServerAuthority;

	UPROPERTY()
	TObjectPtr<UN1RollbackEngine> RollbackEngine;

	UPROPERTY()
	TObjectPtr<UN1BlendInterpolator> BlendInterpolator;

	UPROPERTY()
	TObjectPtr<UN1NetworkClock> NetworkClock;

	float ForcedReconciliationTimer = 0.0f;
	bool bInitialized = false;
};

/** Global accessor delegate */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnN1PhaseChanged, EN1NetcodePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnN1DivergenceDetected, float, DivergenceMagnitude, int32, ClientId, FString, EntityName);
