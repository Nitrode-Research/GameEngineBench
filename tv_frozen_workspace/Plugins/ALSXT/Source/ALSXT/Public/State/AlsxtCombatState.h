#pragma once

#include "Settings/AlsxtCombatSettings.h"
#include "AlsxtCombatState.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCombatState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FAlsxtCombatParameters CombatParameters;
};

