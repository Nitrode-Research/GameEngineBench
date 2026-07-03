// MIT

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "InputActionValue.h"
#include "AlsxtAbilityTaskWaitEnhancedInputEvent.generated.h"

class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnhancedInputEventDelegate, FInputActionValue, Value);

UCLASS()
class ALSXT_API UAlsxtAbilityTaskWaitEnhancedInputEvent : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FEnhancedInputEventDelegate InputEventReceived;
	
	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"), Category = "Ability Tasks")
	static UAlsxtAbilityTaskWaitEnhancedInputEvent* WaitEnhancedInputEvent(UGameplayAbility* OwningAbility, const FName TaskInstanceName, UInputAction* InputAction, const ETriggerEvent TriggerEventType, bool bShouldOnlyTriggerOnce = true);

private:

	TWeakObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;
	
	TWeakObjectPtr<UInputAction> InputAction = nullptr;
	
	ETriggerEvent EventType;
	
	bool bTriggerOnce;

	bool bHasBeenTriggered = false;

	virtual void Activate() override;

	void EventReceived(const FInputActionValue& Value);

	virtual void OnDestroy(const bool bInOwnerFinished) override;
	
	
};
