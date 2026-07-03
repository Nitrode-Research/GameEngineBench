#pragma once

#include "AlsxtFPEyeFocusSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtFPEyeFocusSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0))
	float FirstPersonEyeFocusFOV{ 80.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0))
	float FirstPersonEyeFocusVignette{ 0.33f };
};
