// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - N+1 Concurrent Simulation Framework
// Unreal Engine 5.5+ / UE6 Forward Compatible

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @class FNineRealitiesNetcodeModule
 * @brief Runtime module initializer for the N+1 netcode framework.
 *
 * The Nine Realities Netcode plugin implements a production-ready N+1 concurrent
 * simulation model for competitive multiplayer games. It provides:
 *
 * - Server-authoritative architecture with client-side prediction
 * - Rollback-based state reconciliation
 * - Adaptive interpolation and tolerance-based blending
 * - Deterministic physics synchronization
 * - UE6 forward-compatible networking primitives
 *
 * @version 3.0.0
 * @see https://powder-ranger.github.io/nine-realities-netcode/
 */
class FNineRealitiesNetcodeModule : public IModuleInterface
{
public:
	/** Called when the module is loaded into memory */
	virtual void StartupModule() override;

	/** Called when the module is unloaded from memory */
	virtual void ShutdownModule() override;

	/** Check if the module supports dynamic reloading */
	virtual bool IsGameModule() const override { return true; }

	/** @return The singleton module instance */
	static FNineRealitiesNetcodeModule& Get();

private:
	static FNineRealitiesNetcodeModule* Singleton;
};

/** Plugin-wide logging category */
DECLARE_LOG_CATEGORY_EXTERN(LogN1Netcode, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogN1Prediction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogN1Rollback, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogN1UE6Compat, Log, All);