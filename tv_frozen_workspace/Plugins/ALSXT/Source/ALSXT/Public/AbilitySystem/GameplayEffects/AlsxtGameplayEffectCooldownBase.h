// MIT

#pragma once

#include "CoreMinimal.h"
#include "AlsxtGameplayEffectBase.h"
#include "GameplayEffect.h"
#include "AlsxtGameplayEffectCooldownBase.generated.h"

/**
 * 
 */
UCLASS()
class ALSXT_API UAlsxtGameplayEffectCooldownBase : public UAlsxtGameplayEffectBase
{
	GENERATED_BODY()
	
public:
	UAlsxtGameplayEffectCooldownBase();
	
protected:
	UPROPERTY()
	FGameplayTag CooldownTag;
	
};
