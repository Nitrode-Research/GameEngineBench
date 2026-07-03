// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AsyncTaskAttributeChanged.h"

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	UAsyncTaskAttributeChanged* Task = NewObject<UAsyncTaskAttributeChanged>();
	Task->ASC = AbilitySystemComponent;
	Task->AttributeToListenFor = Attribute;
	return Task;
}

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributesChange(UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes)
{
	UAsyncTaskAttributeChanged* Task = NewObject<UAsyncTaskAttributeChanged>();
	Task->ASC = AbilitySystemComponent;
	Task->AttributesToListenFor = Attributes;
	return Task;
}

void UAsyncTaskAttributeChanged::EndTask()
{
	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTaskAttributeChanged::AttributeChanged(const FOnAttributeChangeData& Data)
{
}
