// MIT

#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskWaitGameplayTagChanged.h"

void UAlsxtAbilityTaskWaitGameplayTagChanged::Activate()
{
}

UAlsxtAbilityTaskWaitGameplayTagChanged* UAlsxtAbilityTaskWaitGameplayTagChanged::WaitGameplayTagChanged(UGameplayAbility* OwningAbility, FGameplayTag Tag, AActor* InOptionalExternalTarget, bool bOnlyTriggerOnce, bool bTriggerAtActivation)
{
	return NewAbilityTask<UAlsxtAbilityTaskWaitGameplayTagChanged>(OwningAbility);
}

void UAlsxtAbilityTaskWaitGameplayTagChanged::GameplayTagCallback(const FGameplayTag InTag, int32 NewCount)
{
}
