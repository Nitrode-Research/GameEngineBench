#pragma once

#include "Engine/EngineTypes.h"
#include "Utility/AlsxtStructs.h"
#include "Settings/AlsxtVaultingSettings.h"
#include "AlsxtVaultingState.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtVaultingState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FAlsxtVaultingParameters VaultingParameters;
};

