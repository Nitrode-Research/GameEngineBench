// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AsyncTaskGameplayTagAddedRemoved.h"

UAsyncTaskGameplayTagAddedRemoved* UAsyncTaskGameplayTagAddedRemoved::ListenForGameplayTagAddedOrRemoved(UAbilitySystemComponent* AbilitySystemComponent, FGameplayTagContainer InTags)
{
	UAsyncTaskGameplayTagAddedRemoved* Task = NewObject<UAsyncTaskGameplayTagAddedRemoved>();
	Task->ASC = AbilitySystemComponent;
	Task->Tags = InTags;
	return Task;
}

void UAsyncTaskGameplayTagAddedRemoved::EndTask()
{
	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTaskGameplayTagAddedRemoved::TagChanged(const FGameplayTag Tag, int32 NewCount)
{
}
