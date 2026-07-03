// Copyright 2021 Joseph "Narxim" Thigpen.


#include "AbilitySystem/FunctionLibrary/AlsxtAbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/AttributeSets/AlsxtHealthAttributeSet.h"
#include "AbilitySystem/AttributeSets/AlsxtResistanceAttributeSet.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"


AActor* UCustomAbilitySystemBlueprintLibrary::GetInstigatorFromGameplayEffectSpec(const FGameplayEffectSpec& Spec)
{
	return Spec.GetEffectContext().GetInstigator();
}

float UCustomAbilitySystemBlueprintLibrary::GetAttributeValueFromActor(const AActor* const Actor, const FGameplayAttribute Attribute, const EAlsxtAttributeSearchType SearchType)
{
	const UAbilitySystemComponent* const AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	
	if (!AbilitySystemComponent)
	{
		return -1.f;
	}

	float ReturnValue = -1.0f;
	GetAttributeValue(AbilitySystemComponent, Attribute, SearchType, ReturnValue);
	
	return ReturnValue;
}

float UCustomAbilitySystemBlueprintLibrary::GetAttributeValueFromAbilitySystem(const UAbilitySystemComponent* const AbilitySystemComponent, const FGameplayAttribute Attribute, const EAlsxtAttributeSearchType SearchType)
{
	float ReturnValue = -1.0f;

	GetAttributeValue(AbilitySystemComponent, Attribute, SearchType, ReturnValue);

	return ReturnValue;
}

float UCustomAbilitySystemBlueprintLibrary::CalculateEffectiveResistance(const float CurrentArmor)
{
	const float CurrentArmorTmp = FMath::Clamp(CurrentArmor, UAlsxtResistanceAttributeSet::RESISTANCE_MIN, UAlsxtResistanceAttributeSet::RESISTANCE_MAX);
	
	const float ArmorDenominator = UAlsxtResistanceAttributeSet::RESISTANCE_BASE + CurrentArmorTmp;
	return UAlsxtResistanceAttributeSet::RESISTANCE_BASE > 0.f && ArmorDenominator > 0.f ? UAlsxtResistanceAttributeSet::RESISTANCE_BASE / ArmorDenominator : 1.f;
}

float UCustomAbilitySystemBlueprintLibrary::GetValueAtLevel(const FScalableFloat& ScalableFloat, const float Level /* = 0.f */,	const FString& ContextString /* = "" */)
{
	return ScalableFloat.GetValueAtLevel(Level, &ContextString);
}

void UCustomAbilitySystemBlueprintLibrary::SetTargetOnGameplayEffectContext(FGameplayEffectContextHandle& ContextHandle,	const AActor* TargetActor)
{
	if (FAlsxtCustomGameplayEffectContext* const EffectContext = static_cast<FAlsxtCustomGameplayEffectContext*>(ContextHandle.Get()))
	{
		EffectContext->SetTargetActor(TargetActor);
	}
}

void UCustomAbilitySystemBlueprintLibrary::SetTargetOnGameplayEffectContextFromSpec(FGameplayEffectSpec& EffectSpec,	const AActor* TargetActor)
{
	SetTargetOnGameplayEffectContext(const_cast<FGameplayEffectContextHandle&>(EffectSpec.GetEffectContext()), TargetActor);
}

const AActor* UCustomAbilitySystemBlueprintLibrary::GetTargetActorFromGameplayEffectContext(const FGameplayEffectContextHandle& ContextHandle)
{
	if (const FAlsxtCustomGameplayEffectContext* const EffectContext = static_cast<const FAlsxtCustomGameplayEffectContext*>(ContextHandle.Get()))
	{
		return EffectContext->GetTargetActor();
	}
	return nullptr;
}

const AActor* UCustomAbilitySystemBlueprintLibrary::GetTargetActorFromGameplayEffectSpec(const FGameplayEffectSpec& EffectSpec)
{
	return GetTargetActorFromGameplayEffectContext(EffectSpec.GetEffectContext());
}

void UCustomAbilitySystemBlueprintLibrary::GetAttributeValue(const UAbilitySystemComponent* const AbilitySystemComponent, const FGameplayAttribute& Attribute, const EAlsxtAttributeSearchType SearchType, OUT float& ReturnValue)
{
	ReturnValue = -1.0f;
	
	if (!AbilitySystemComponent || !AbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		return;
	}

	switch (SearchType)
	{
	case EAlsxtAttributeSearchType::FinalValue:
		{
			ReturnValue = AbilitySystemComponent->GetNumericAttribute(Attribute);

			return;
		}

	case EAlsxtAttributeSearchType::BaseValue:
		{
			ReturnValue = AbilitySystemComponent->GetNumericAttributeBase(Attribute);

			return;
		}

	case EAlsxtAttributeSearchType::BonusValue:
		{
			ReturnValue = AbilitySystemComponent->GetNumericAttribute(Attribute) - AbilitySystemComponent->
				GetNumericAttributeBase(Attribute);
		}
	}
}