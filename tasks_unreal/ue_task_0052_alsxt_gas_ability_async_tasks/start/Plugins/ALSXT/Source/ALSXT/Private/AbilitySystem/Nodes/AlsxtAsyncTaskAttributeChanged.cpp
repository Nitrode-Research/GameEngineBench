// MIT

#include "AbilitySystem/Nodes/AlsxtAsyncTaskAttributeChanged.h"

UAlsxtAsyncTaskAttributeChanged* UAlsxtAsyncTaskAttributeChanged::ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	return nullptr;
}

UAlsxtAsyncTaskAttributeChanged* UAlsxtAsyncTaskAttributeChanged::ListenForAttributesChange(UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes)
{
	return nullptr;
}

void UAlsxtAsyncTaskAttributeChanged::EndTask()
{
	SetReadyToDestroy();
}

void UAlsxtAsyncTaskAttributeChanged::AttributeChanged(const FOnAttributeChangeData& Data)
{
}
