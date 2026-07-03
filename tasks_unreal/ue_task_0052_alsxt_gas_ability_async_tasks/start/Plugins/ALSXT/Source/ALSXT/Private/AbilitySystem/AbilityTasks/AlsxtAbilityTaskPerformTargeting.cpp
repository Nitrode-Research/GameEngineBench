// MIT

#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskPerformTargeting.h"

UAlsxtAbilityTaskPerformTargeting::UAlsxtAbilityTaskPerformTargeting(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAlsxtAbilityTaskPerformTargeting* UAlsxtAbilityTaskPerformTargeting::CustomTargetDataTask(UGameplayAbility* OwningAbility, FName TaskName)
{
	return NewAbilityTask<UAlsxtAbilityTaskPerformTargeting>(OwningAbility, TaskName);
}

void UAlsxtAbilityTaskPerformTargeting::Activate()
{
}

void UAlsxtAbilityTaskPerformTargeting::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

void UAlsxtAbilityTaskPerformTargeting::PerformTargeting()
{
}

FGameplayAbilityTargetDataHandle UAlsxtAbilityTaskPerformTargeting::ProduceTargetData()
{
	return FGameplayAbilityTargetDataHandle();
}
