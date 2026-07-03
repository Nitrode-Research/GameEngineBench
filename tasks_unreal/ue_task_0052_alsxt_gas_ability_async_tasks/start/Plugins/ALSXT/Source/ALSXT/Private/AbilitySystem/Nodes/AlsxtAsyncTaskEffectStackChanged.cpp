// MIT

#include "AbilitySystem/Nodes/AlsxtAsyncTaskEffectStackChanged.h"

UAlsxtAsyncTaskEffectStackChanged* UAlsxtAsyncTaskEffectStackChanged::ListenForGameplayEffectStackChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayTag EffectGameplayTag)
{
	return nullptr;
}

void UAlsxtAsyncTaskEffectStackChanged::EndTask()
{
	SetReadyToDestroy();
}

void UAlsxtAsyncTaskEffectStackChanged::OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
}

void UAlsxtAsyncTaskEffectStackChanged::OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved)
{
}

void UAlsxtAsyncTaskEffectStackChanged::GameplayEffectStackChanged(FActiveGameplayEffectHandle EffectHandle, int32 NewStackCount, int32 PreviousStackCount)
{
}
