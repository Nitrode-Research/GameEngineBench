// MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AlsxtAssetManagerBase.generated.h"

/**
 * 
 */
UCLASS()
class ALSXT_API UAlsxtAssetManagerBase : public UAssetManager
{
	GENERATED_BODY()
	
protected:
	virtual void StartInitialLoading() override;
	
	
};
