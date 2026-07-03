#include "Gameplay/GAS/Components/ECRCharacterHealthComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/ECRGameplayTags.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRMovementSet.h"
#include "System/Messages/ECRVerbMessage.h"
#include "System/Messages/ECRVerbMessageHelpers.h"


UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Wound_Message, "ECR.Wound.Message");


void UECRCharacterHealthComponent::InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::UninitializeFromAbilitySystem()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRCharacterHealthComponent::GetShield() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetMaxShield() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetShieldNormalized() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetBleedingHealth() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetMaxBleedingHealth() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetBleedingHealthNormalized() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetStamina() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetMaxStamina() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetStaminaNormalized() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetEvasionStamina() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetMaxEvasionStamina() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRCharacterHealthComponent::GetEvasionStaminaNormalized() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRCharacterHealthComponent::ClearGameplayTags()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthComponent::ChangeCharacterRootMotionScale(float NewScale)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthComponent::ChangeCharacterSpeed(const float NewSpeed)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleRootMotionScaleChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleWalkSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleMaxShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleBleedingHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleMaxBleedingHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleReadyToBecomeWounded(AActor* DamageInstigator, AActor* DamageCauser,
                                                              const FGameplayEffectSpec& DamageEffectSpec,
                                                              float DamageMagnitude)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthComponent::HandleReadyToBecomeUnwounded(AActor* EffectInstigator, AActor* EffectCauser,
                                                                const FGameplayEffectSpec& EffectSpec,
                                                                float EffectMagnitude)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCharacterHealthComponent::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCharacterHealthComponent::HandleMaxEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRCharacterHealthComponent::GetDamageToKill()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

