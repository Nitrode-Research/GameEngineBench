// MIT

#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskWaitEnhancedInputEvent.h"

UAlsxtAbilityTaskWaitEnhancedInputEvent* UAlsxtAbilityTaskWaitEnhancedInputEvent::WaitEnhancedInputEvent(UGameplayAbility* OwningAbility, const FName TaskInstanceName, UInputAction* InInputAction, const ETriggerEvent TriggerEventType, const bool bShouldOnlyTriggerOnce)
{
	return NewAbilityTask<UAlsxtAbilityTaskWaitEnhancedInputEvent>(OwningAbility, TaskInstanceName);
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::Activate()
{
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::EventReceived(const FInputActionValue& Value)
{
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::OnDestroy(const bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}
