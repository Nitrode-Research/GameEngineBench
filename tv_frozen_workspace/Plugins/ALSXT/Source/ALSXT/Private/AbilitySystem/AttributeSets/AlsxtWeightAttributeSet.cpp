// MIT


#include "AbilitySystem/AttributeSets/AlsxtWeightAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

float UAlsxtWeightAttributeSet::Weight_BASE = 100.f; 
float UAlsxtWeightAttributeSet::Weight_MAX = 200.f; // 3 times the base armor.
float UAlsxtWeightAttributeSet::Weight_MIN = -75.f; // Up to 1/4 of the base armor.

UAlsxtWeightAttributeSet::UAlsxtWeightAttributeSet()
{
	Weight = 0.0f;
}

void UAlsxtWeightAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UAlsxtWeightAttributeSet::ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttributes(Attribute, NewValue);
	if (Attribute == GetWeightAttribute())
	{
		NewValue = FMath::Clamp(NewValue, Weight_MIN, Weight_MAX);
		return;
	}
}

void UAlsxtWeightAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtWeightAttributeSet, Weight, Params);
}

void UAlsxtWeightAttributeSet::OnRep_Weight(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtWeightAttributeSet, Weight, OldValue);
}