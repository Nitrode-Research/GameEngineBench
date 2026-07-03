// MIT


#include "AbilitySystem/AttributeSets/AlsxtResistanceAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

float UAlsxtResistanceAttributeSet::RESISTANCE_BASE = 100.f; 
float UAlsxtResistanceAttributeSet::RESISTANCE_MAX = 200.f; // 3 times the base armor.
float UAlsxtResistanceAttributeSet::RESISTANCE_MIN = -75.f; // Up to 1/4 of the base armor.

UAlsxtResistanceAttributeSet::UAlsxtResistanceAttributeSet()
{
	Resistance = 0.0f;
}

void UAlsxtResistanceAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UAlsxtResistanceAttributeSet::ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttributes(Attribute, NewValue);
	if (Attribute == GetResistanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, RESISTANCE_MIN, RESISTANCE_MAX);
		return;
	}
}

void UAlsxtResistanceAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtResistanceAttributeSet, Resistance, Params);
}

void UAlsxtResistanceAttributeSet::OnRep_Resistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtResistanceAttributeSet, Resistance, OldValue);
}