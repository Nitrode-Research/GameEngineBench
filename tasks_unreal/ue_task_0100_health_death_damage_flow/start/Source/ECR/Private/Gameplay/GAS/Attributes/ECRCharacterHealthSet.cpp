// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Net/UnrealNetwork.h"

UECRCharacterHealthSet::UECRCharacterHealthSet()
	: Shield(100.0f),
	  MaxShield(100.0f),
	  ShieldRegenDelay(5.0f),
	  ShieldRegenRate(20.0f),
	  BleedingHealth(100.0f),
	  MaxBleedingHealth(100.0f)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UECRCharacterHealthSet::GetIsReadyToBecomeWounded() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRCharacterHealthSet::GetIsReadyToDie() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UECRCharacterHealthSet::CheckIfReadyToBecomeWounded(const FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::OnRep_Shield(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthSet::OnRep_ShieldRegenDelay(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthSet::OnRep_ShieldRegenRate(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::OnRep_BleedingHealth(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthSet::OnRep_MaxBleedingHealth(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

