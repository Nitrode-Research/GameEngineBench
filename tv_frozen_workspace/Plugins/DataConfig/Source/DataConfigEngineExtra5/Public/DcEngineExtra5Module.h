#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FDcEngineExtra5Module : public IModuleInterface
{
public:
	FORCEINLINE static FDcEngineExtra5Module& Get()
	{
		return FModuleManager::LoadModuleChecked<FDcEngineExtra5Module>("DataConfigEngineExtra5");
	}

	FORCEINLINE static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DataConfigEngineExtra5");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
