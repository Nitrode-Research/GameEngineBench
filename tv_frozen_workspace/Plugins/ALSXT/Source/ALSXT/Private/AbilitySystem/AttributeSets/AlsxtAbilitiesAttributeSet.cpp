#include "AbilitySystem/AttributeSets/AlsxtAbilitiesAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

float UAlsxtAbilitiesAttributeSet::COOLDOWN_MULTIPLIER_MIN = 0.1f;
float UAlsxtAbilitiesAttributeSet::COOLDOWN_MULTIPLIER_MAX = 2.f;

float UAlsxtAbilitiesAttributeSet::COST_MULTIPLIER_MIN = 0.1f;
float UAlsxtAbilitiesAttributeSet::COST_MULTIPLIER_MAX = 2.f;

UAlsxtAbilitiesAttributeSet::UAlsxtAbilitiesAttributeSet()
{
	InitCooldownMultiplier(1.f);
	InitCostMultiplier(1.f);
}

void UAlsxtAbilitiesAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void UAlsxtAbilitiesAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UAlsxtAbilitiesAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UAlsxtAbilitiesAttributeSet::ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetCostMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, COST_MULTIPLIER_MIN, COST_MULTIPLIER_MAX);
		return;
	}

	if (Attribute == GetCooldownMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, COOLDOWN_MULTIPLIER_MIN, COOLDOWN_MULTIPLIER_MAX);
		return;
	}
}

void UAlsxtAbilitiesAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_OwnerOnly;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtAbilitiesAttributeSet, CooldownMultiplier, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAlsxtAbilitiesAttributeSet, CostMultiplier, Params);
}

void UAlsxtAbilitiesAttributeSet::OnRep_CooldownMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtAbilitiesAttributeSet, CooldownMultiplier, OldValue);
}

void UAlsxtAbilitiesAttributeSet::OnRep_CostMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlsxtAbilitiesAttributeSet, CostMultiplier, OldValue);
}