#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FDcEngineExtraModule : public IModuleInterface
{
public:
	FORCEINLINE static FDcEngineExtraModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FDcEngineExtraModule>("DataConfigEngineExtra");
	}

	FORCEINLINE static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DataConfigEngineExtra");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
