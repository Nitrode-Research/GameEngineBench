#pragma once

#include "Utility/AlsxtStructs.h"
#include "AlsxtForegripPositionSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtForegripPositionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FForegripPositions ForegripPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Debug {false};
};
