// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "NineRealitiesNetcode.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogN1Netcode);
DEFINE_LOG_CATEGORY(LogN1Prediction);
DEFINE_LOG_CATEGORY(LogN1Rollback);
DEFINE_LOG_CATEGORY(LogN1UE6Compat);

FNineRealitiesNetcodeModule* FNineRealitiesNetcodeModule::Singleton = nullptr;

void FNineRealitiesNetcodeModule::StartupModule()
{
	Singleton = this;
	UE_LOG(LogN1Netcode, Log, TEXT("============================================================"));
	UE_LOG(LogN1Netcode, Log, TEXT("Nine Realities Netcode v3.0.0 - N+1 Concurrent Simulation"));
	UE_LOG(LogN1Netcode, Log, TEXT("UE5.5+ Runtime | UE6 Forward Compatible"));
	UE_LOG(LogN1Netcode, Log, TEXT("https://powder-ranger.github.io/nine-realities-netcode/"));
	UE_LOG(LogN1Netcode, Log, TEXT("============================================================"));

#if N1_UE6_READY
	UE_LOG(LogN1UE6Compat, Log, TEXT("UE6 Forward Compatibility: ENABLED"));
#endif
}

void FNineRealitiesNetcodeModule::ShutdownModule()
{
	UE_LOG(LogN1Netcode, Log, TEXT("Nine Realities Netcode module shutting down"));
	Singleton = nullptr;
}

FNineRealitiesNetcodeModule& FNineRealitiesNetcodeModule::Get()
{
	checkf(Singleton, TEXT("NineRealitiesNetcode module not yet loaded!"));
	return *Singleton;
}

IMPLEMENT_MODULE(FNineRealitiesNetcodeModule, NineRealitiesNetcode)
