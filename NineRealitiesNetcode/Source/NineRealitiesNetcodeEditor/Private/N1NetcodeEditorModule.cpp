// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "N1NetcodeEditorModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "N1NetcodeEditor"

void FN1NetcodeEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Nine Realities Netcode Editor module loaded"));
}

void FN1NetcodeEditorModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("Nine Realities Netcode Editor module unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FN1NetcodeEditorModule, NineRealitiesNetcodeEditor)
