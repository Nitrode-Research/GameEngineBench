// MIT


#include "AbilitySystem/AbilityTasks/AlsxtAbilityTaskWaitEnhancedInputEvent.h"

UAlsxtAbilityTaskWaitEnhancedInputEvent* UAlsxtAbilityTaskWaitEnhancedInputEvent::WaitEnhancedInputEvent(UGameplayAbility* OwningAbility, const FName TaskInstanceName, UInputAction* InputAction, const ETriggerEvent TriggerEventType, const bool bShouldOnlyTriggerOnce)
{
	UAlsxtAbilityTaskWaitEnhancedInputEvent* AbilityTask = NewAbilityTask<UAlsxtAbilityTaskWaitEnhancedInputEvent>(OwningAbility, TaskInstanceName);
	
	AbilityTask->InputAction = InputAction;
	AbilityTask->EventType = TriggerEventType;
	AbilityTask->bTriggerOnce = bShouldOnlyTriggerOnce;
	
	return AbilityTask;
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::Activate()
{
	Super::Activate();

	if (!AbilitySystemComponent.Get() || !Ability || !InputAction.IsValid())
	{
		return;
	}

	const APawn* const AvatarPawn = Cast<APawn>(Ability->GetAvatarActorFromActorInfo());
	const APlayerController* const PlayerController = AvatarPawn ? Cast<APlayerController>(AvatarPawn->GetController()) : nullptr;
	
	if (!PlayerController)
	{
		return;
	}

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
			
	if (IsValid(EnhancedInputComponent.Get()))
	{
		EnhancedInputComponent->BindAction(InputAction.Get(), EventType, this, &UAlsxtAbilityTaskWaitEnhancedInputEvent::EventReceived);
	}
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::EventReceived(const FInputActionValue& Value)
{
	if (bTriggerOnce && bHasBeenTriggered)
	{
		return;
	}

	bHasBeenTriggered = true;
	
	InputEventReceived.Broadcast(Value);
}

void UAlsxtAbilityTaskWaitEnhancedInputEvent::OnDestroy(const bool bInOwnerFinished)
{
	if (IsValid(EnhancedInputComponent.Get()))
	{
		EnhancedInputComponent->ClearBindingsForObject(this);
	}
	
	Super::OnDestroy(bInOwnerFinished);
}


