// MIT


#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskOnTickEvent.h"

UAlsxtAbilityTaskOnTickEvent::UAlsxtAbilityTaskOnTickEvent()
{
	bTickingTask = true;
	bSimulatedTask = true;
}

UAlsxtAbilityTaskOnTickEvent* UAlsxtAbilityTaskOnTickEvent::OnTickEvent(UGameplayAbility* OwningAbility, const FName TaskInstanceName)
{
	return NewAbilityTask<UAlsxtAbilityTaskOnTickEvent>(OwningAbility, TaskInstanceName);
}

void UAlsxtAbilityTaskOnTickEvent::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);

	EventReceived(DeltaTime);
}

void UAlsxtAbilityTaskOnTickEvent::EventReceived(const float DeltaTime) const
{
	if (TickEventReceived.IsBound())
	{
		TickEventReceived.Broadcast(DeltaTime);
	}
}

void UAlsxtAbilityTaskOnTickEvent::OnDestroy(const bool bInOwnerFinished)
{
	bTickingTask = false;
	
	Super::OnDestroy(bInOwnerFinished);
}


