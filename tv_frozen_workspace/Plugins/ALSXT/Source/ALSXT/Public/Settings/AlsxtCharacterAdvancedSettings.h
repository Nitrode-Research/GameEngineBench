// MIT

#pragma once

#include "CoreMinimal.h"
#include "Settings/AlsxtCombatSettings.h"
#include "Settings/AlsxtFirearmSettings.h"
#include "AlsxtCharacterAdvancedSettings.generated.h"

UCLASS()
class ALSXT_API UAlsxtCharacterAdvancedSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FAlsxtGeneralCombatSettings Combat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sights")
	FAlsxtGeneralSightsSettings Sights;

	UAlsxtCharacterAdvancedSettings();
	
};
