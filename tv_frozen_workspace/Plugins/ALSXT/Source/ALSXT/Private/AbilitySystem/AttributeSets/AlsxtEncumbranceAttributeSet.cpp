// MIT


#include "AbilitySystem/AttributeSets/AlsxtEncumbranceAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

float UAlsxtEncumbranceAttributeSet::Encumbrance_BASE = 100.f; 
float UAlsxtEncumbranceAttributeSet::Encumbrance_MAX = 200.f; // 3 times the base armor.
float UAlsxtEncumbranceAttributeSet::Encumbrance_MIN = -75.f; // Up to 1/4 of the base armor.

UAlsxtEncumbranceAttributeSet::UAlsxtEncumbranceAttributeSet()
{
	Encumbrance = 0.0f;
}

void UAlsxtEncumbranceAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UAlsxtEncumbranceAttributeSet::ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttributes(Attribute, NewValue);
	if (Attribute == GetEncumbranceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, Encumbrance_MIN, Encumbrance_MAX);
		return;
	}
}

void UAlsxtEncumbranceAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtEncumbranceAttributeSet, Encumbrance, Params);
}

void UAlsxtEncumbranceAttributeSet::OnRep_Encumbrance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtEncumbranceAttributeSet, Encumbrance, OldValue);
}