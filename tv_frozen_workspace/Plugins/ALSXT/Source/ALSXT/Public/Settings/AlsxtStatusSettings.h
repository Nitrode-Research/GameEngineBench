#pragma once

#include "Utility/AlsxtStructs.h"
#include "Utility/AlsxtGameplayTags.h"
#include "NativeGameplayTags.h"
#include "AlsxtStatusSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtStaminaThresholdSettings
{
	GENERATED_BODY()

	/* Stamina above this number is connverted  to Gameplay Tag as *Optimal*. If Character has Stamina above this value, the Character will not make a Breath sound  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.51, ClampMax = 0.99, UIClampMin = 0.51, UIClampMax = 0.99))
	float StaminaOptimalThreshold{ 75.0f };

	/* Stamina above this number is connverted to Gameplay Tag as *Low*. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.01, ClampMax = 0.49, UIClampMin = 0.01, UIClampMax = 0.49))
	float StaminaLowThreshold{ 25.0f };

};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtStatusSettings
{
	GENERATED_BODY()

	/* These values determine the default Stamina Gameplay Tags. These values can be overidden by the Server  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.51, ClampMax = 0.99, UIClampMin = 0.51, UIClampMax = 0.99))
	FAlsxtStaminaThresholdSettings StaminaThresholdSettings;
};
