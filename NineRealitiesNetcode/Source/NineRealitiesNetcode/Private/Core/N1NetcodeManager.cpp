// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Core/N1NetcodeManager.h"
#include "Core/N1NetworkClock.h"
#include "Core/N1PredictionBuffer.h"
#include "Pipeline/N1ReconciliationEngine.h"
#include "Pipeline/N1ClientPrediction.h"
#include "Pipeline/N1ServerAuthority.h"
#include "Pipeline/N1RollbackEngine.h"
#include "Pipeline/N1BlendInterpolator.h"
#include "UE6/N1UE6Compatibility.h"
#include "Engine/World.h"

// Static singleton storage
static TWeakObjectPtr<UN1NetcodeManager> GNetcodeManager;

UN1NetcodeManager::UN1NetcodeManager()
{
	CurrentMode = EN1NetcodeMode::Standalone;
	CurrentPhase = EN1NetcodePhase::Initializing;
}

void UN1NetcodeManager::Initialize(const FN1NetcodeConfig& InConfig, EN1NetcodeMode InMode)
{
	Config = InConfig;
	CurrentMode = InMode;

	UE_LOG(LogN1Netcode, Log, TEXT("Initializing N1 Netcode Manager [Mode: %s]"),
		*UEnum::GetValueAsString(InMode));

	// Create subsystems
	NetworkClock = NewObject<UN1NetworkClock>(this);
	NetworkClock->Initialize(InMode == EN1NetcodeMode::Server || InMode == EN1NetcodeMode::ListenServer);

	if (InMode == EN1NetcodeMode::Client || InMode == EN1NetcodeMode::ListenServer)
	{
		auto* Buffer = NewObject<UN1PredictionBuffer>(this);
		int32 BufferFrames = FMath::CeilToInt((Config.MaxAcceptableLatencyMs / 1000.0f) * Config.ClientTickRate) + Config.MaxRollbackFrames;
		Buffer->Initialize(BufferFrames, Config.ClientTickRate);

		PredictionSystem = NewObject<UN1ClientPrediction>(this);
		PredictionSystem->Initialize(Buffer, NetworkClock, Config.ClientTickRate);

		RollbackEngine = NewObject<UN1RollbackEngine>(this);
		RollbackEngine->Initialize(Config.MaxRollbackFrames);

		BlendInterpolator = NewObject<UN1BlendInterpolator>(this);
		BlendInterpolator->Initialize(Config.BlendFrames, EN1BlendCurve::SmoothStep);

		ReconciliationEngine = NewObject<UN1ReconciliationEngine>(this);
		ReconciliationEngine->Initialize(Config.RollbackThreshold, Config.MaxRollbackFrames);
	}

	if (InMode == EN1NetcodeMode::Server || InMode == EN1NetcodeMode::ListenServer)
	{
		ServerAuthority = NewObject<UN1ServerAuthority>(this);
		ServerAuthority->Initialize(Config.ServerTickRate, Config.InputBufferMs);
	}

	// UE6 compatibility check
	auto* UE6Compat = NewObject<UN1UE6Compatibility>(this);
	UE6Compat->Initialize();
	UE6Compat->LogCompatibilityStatus();

	bInitialized = true;
	SetPhase(EN1NetcodePhase::Active);

	UE_LOG(LogN1Netcode, Log, TEXT("N1 Netcode Manager initialized successfully"));
}

void UN1NetcodeManager::Shutdown()
{
	SetPhase(EN1NetcodePhase::ShuttingDown);

	PredictionSystem = nullptr;
	ReconciliationEngine = nullptr;
	RollbackEngine = nullptr;
	BlendInterpolator = nullptr;
	ServerAuthority = nullptr;
	NetworkClock = nullptr;

	bInitialized = false;
	GNetcodeManager.Reset();

	UE_LOG(LogN1Netcode, Log, TEXT("N1 Netcode Manager shutdown complete"));
}

void UN1NetcodeManager::Tick(float DeltaTime)
{
	if (!bInitialized) return;

	// Update network clock
	if (NetworkClock)
	{
		NetworkClock->Tick(DeltaTime);
	}

	// Tick client systems
	if (PredictionSystem)
	{
		PredictionSystem->Tick(DeltaTime);
	}

	if (BlendInterpolator)
	{
		BlendInterpolator->Tick(DeltaTime);
	}

	// Server tick
	if (ServerAuthority)
	{
		ServerAuthority->TickServer(DeltaTime);
	}

	// Periodic forced reconciliation
	if (Config.ForcedReconciliationIntervalSec > 0)
	{
		PerformForcedReconciliation(DeltaTime);
	}
}

UN1NetcodeManager* UN1NetcodeManager::GetN1Manager(UWorld* World)
{
	if (GNetcodeManager.IsValid())
	{
		return GNetcodeManager.Get();
	}

	if (World)
	{
		UN1NetcodeManager* Manager = NewObject<UN1NetcodeManager>(World);
		GNetcodeManager = Manager;
		return Manager;
	}

	return nullptr;
}

void UN1NetcodeManager::SetPhase(EN1NetcodePhase NewPhase)
{
	if (CurrentPhase != NewPhase)
	{
		EN1NetcodePhase OldPhase = CurrentPhase;
		CurrentPhase = NewPhase;
		OnPhaseChanged.Broadcast(NewPhase);
		UE_LOG(LogN1Netcode, Verbose, TEXT("Phase transition: %s -> %s"),
			*UEnum::GetValueAsString(OldPhase),
			*UEnum::GetValueAsString(NewPhase));
	}
}

void UN1NetcodeManager::PerformForcedReconciliation(float DeltaTime)
{
	ForcedReconciliationTimer += DeltaTime;
	if (ForcedReconciliationTimer >= Config.ForcedReconciliationIntervalSec)
	{
		ForcedReconciliationTimer = 0.0f;
		UE_LOG(LogN1Netcode, Verbose, TEXT("Performing forced periodic reconciliation"));
		// This would trigger a full-state resync in production
	}
}
