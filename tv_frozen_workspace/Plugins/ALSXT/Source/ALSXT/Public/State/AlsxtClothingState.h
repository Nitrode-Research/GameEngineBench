#pragma once

#include "Settings/AlsxtClothingSettings.h"
#include "AlsxtClothingState.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtClothingState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FAlsxtClothingParameters ClothingParameters;
};

