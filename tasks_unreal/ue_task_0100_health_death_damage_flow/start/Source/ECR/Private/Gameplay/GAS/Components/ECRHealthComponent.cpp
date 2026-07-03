// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Components/ECRHealthComponent.h"
#include "System/ECRLogChannels.h"
#include "System/ECRAssetManager.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayPrediction.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "System/Messages/ECRVerbMessage.h"
#include "System/Messages/ECRVerbMessageHelpers.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Elimination_Message, "ECR.Elimination.Message");


UECRHealthComponent::UECRHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::OnUnregister()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRHealthComponent::GetNormalizedAttributeValue(const float Value, const float MaxValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UECRHealthComponent::InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::UninitializeFromAbilitySystem()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::ClearGameplayTags()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRHealthComponent::GetHealth() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRHealthComponent::GetMaxHealth() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRHealthComponent::GetHealthNormalized() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


AActor* UECRHealthComponent::GetInstigatorFromAttrChangeData(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::HandleReadyToDie(AActor* DamageInstigator, AActor* DamageCauser,
                                           const FGameplayEffectSpec& DamageEffectSpec, float DamageMagnitude)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::OnRep_DeathState(EECRDeathState OldDeathState)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::StartDeath()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::FinishDeath()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRHealthComponent::GetDamageToKill()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

