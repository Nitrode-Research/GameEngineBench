// MIT

#include "AbilitySystem/Nodes/AlsxtAsyncTaskCooldownChanged.h"

UAlsxtAsyncTaskCooldownChanged* UAlsxtAsyncTaskCooldownChanged::ListenForCooldownChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayTagContainer CooldownTags, bool UseServerCooldown)
{
	return nullptr;
}

void UAlsxtAsyncTaskCooldownChanged::EndTask()
{
	SetReadyToDestroy();
}

void UAlsxtAsyncTaskCooldownChanged::OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
}

void UAlsxtAsyncTaskCooldownChanged::CooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount)
{
}

bool UAlsxtAsyncTaskCooldownChanged::GetCooldownRemainingForTag(FGameplayTagContainer CooldownTags, float& TimeRemaining, float& CooldownDuration)
{
	TimeRemaining = 0.0f;
	CooldownDuration = 0.0f;
	return false;
}
