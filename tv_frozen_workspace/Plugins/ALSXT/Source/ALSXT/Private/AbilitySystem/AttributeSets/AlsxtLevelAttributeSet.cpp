// MIT


#include "AbilitySystem/AttributeSets/AlsxtLevelAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UAlsxtLevelAttributeSet::UAlsxtLevelAttributeSet()
{
	InitMaximumLevel(0.f);
	InitCurrentLevel(0.f);
}

void UAlsxtLevelAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);

	if (Attribute != GetCurrentLevelAttribute())
	{
		return;
	}

	if (UAlsxtAbilitySystemComponent* const ASC = Cast<UAlsxtAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
	{
		ASC->ChangeLevel(NewValue);
	}
}

void UAlsxtLevelAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtLevelAttributeSet, CurrentLevel, Params);
	
	// Only Owner
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtLevelAttributeSet, MaximumLevel, Params);
}

void UAlsxtLevelAttributeSet::OnRep_CurrentLevel(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtLevelAttributeSet, CurrentLevel, OldValue);
}

void UAlsxtLevelAttributeSet::OnRep_MaximumLevel(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtLevelAttributeSet, MaximumLevel, OldValue);
}