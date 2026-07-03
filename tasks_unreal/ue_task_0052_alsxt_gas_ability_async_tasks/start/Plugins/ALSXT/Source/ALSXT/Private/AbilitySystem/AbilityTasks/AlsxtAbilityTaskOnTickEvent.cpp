// MIT

#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskOnTickEvent.h"

UAlsxtAbilityTaskOnTickEvent::UAlsxtAbilityTaskOnTickEvent()
{
	bTickingTask = false;
}

UAlsxtAbilityTaskOnTickEvent* UAlsxtAbilityTaskOnTickEvent::OnTickEvent(UGameplayAbility* OwningAbility, const FName TaskInstanceName)
{
	return NewAbilityTask<UAlsxtAbilityTaskOnTickEvent>(OwningAbility, TaskInstanceName);
}

void UAlsxtAbilityTaskOnTickEvent::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UAlsxtAbilityTaskOnTickEvent::EventReceived(const float DeltaTime) const
{
}

void UAlsxtAbilityTaskOnTickEvent::OnDestroy(const bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}
