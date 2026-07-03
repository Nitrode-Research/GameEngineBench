// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "System/Messages/ECRVerbMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Damage, "Gameplay.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageImmunity, "Gameplay.DamageImmunity");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageSelfDestruct, "Gameplay.Damage.Reason.SelfDestruct");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_FellOutOfWorld, "Gameplay.Damage.Reason.FellOutOfWorld");
UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Damage_Message, "ECR.Damage.Message");
UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Healing_Message, "ECR.Healing.Message");

UECRHealthSet::UECRHealthSet()
	: Health(100.0f),
	  MaxHealth(100.0f)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UECRHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UECRHealthSet::SendDamageMessage(const FGameplayEffectModCallbackData& DamageData) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthSet::SendHealingMessage(const FGameplayEffectModCallbackData& HealingData) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UECRHealthSet::GetIsReadyToDie() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRHealthSet::CheckIfReadyToDie(const FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

