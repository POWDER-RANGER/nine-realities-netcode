// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.
// Nine Realities Netcode - Editor Module

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FN1NetcodeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
