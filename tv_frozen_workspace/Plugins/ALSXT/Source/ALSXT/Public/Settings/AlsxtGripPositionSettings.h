#pragma once

#include "Utility/ALSXTStructs.h"
#include "AlsxtGripPositionSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGripPositionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FForegripPositions ForegripPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Debug {false};
};
