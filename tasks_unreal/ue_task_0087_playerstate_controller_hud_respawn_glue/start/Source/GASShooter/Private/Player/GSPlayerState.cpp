// Copyright 2020 Dan Kestranek.

#include "Player/GSPlayerState.h"

#include "Characters/Abilities/AttributeSets/GSAmmoAttributeSet.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"

AGSPlayerState::AGSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSetBase = CreateDefaultSubobject<UGSAttributeSetBase>(TEXT("AttributeSetBase"));
	AmmoAttributeSet = CreateDefaultSubobject<UGSAmmoAttributeSet>(TEXT("AmmoAttributeSet"));

	SetNetUpdateFrequency(100.0f);

	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	KnockedDownTag = FGameplayTag::RequestGameplayTag("State.KnockedDown");
}

UAbilitySystemComponent* AGSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UGSAttributeSetBase* AGSPlayerState::GetAttributeSetBase() const
{
	return AttributeSetBase;
}

UGSAmmoAttributeSet* AGSPlayerState::GetAmmoAttributeSet() const
{
	return AmmoAttributeSet;
}

bool AGSPlayerState::IsAlive() const
{
	return false;
}

void AGSPlayerState::ShowAbilityConfirmPrompt(bool bShowPrompt)
{
}

void AGSPlayerState::ShowInteractionPrompt(float InteractionDuration)
{
}

void AGSPlayerState::HideInteractionPrompt()
{
}

void AGSPlayerState::StartInteractionTimer(float InteractionDuration)
{
}

void AGSPlayerState::StopInteractionTimer()
{
}

float AGSPlayerState::GetHealth() const
{
	return 0.0f;
}

float AGSPlayerState::GetMaxHealth() const
{
	return 0.0f;
}

float AGSPlayerState::GetHealthRegenRate() const
{
	return 0.0f;
}

float AGSPlayerState::GetMana() const
{
	return 0.0f;
}

float AGSPlayerState::GetMaxMana() const
{
	return 0.0f;
}

float AGSPlayerState::GetManaRegenRate() const
{
	return 0.0f;
}

float AGSPlayerState::GetStamina() const
{
	return 0.0f;
}

float AGSPlayerState::GetMaxStamina() const
{
	return 0.0f;
}

float AGSPlayerState::GetStaminaRegenRate() const
{
	return 0.0f;
}

float AGSPlayerState::GetShield() const
{
	return 0.0f;
}

float AGSPlayerState::GetMaxShield() const
{
	return 0.0f;
}

float AGSPlayerState::GetShieldRegenRate() const
{
	return 0.0f;
}

float AGSPlayerState::GetArmor() const
{
	return 0.0f;
}

float AGSPlayerState::GetMoveSpeed() const
{
	return 0.0f;
}

int32 AGSPlayerState::GetCharacterLevel() const
{
	return 0;
}

int32 AGSPlayerState::GetXP() const
{
	return 0;
}

int32 AGSPlayerState::GetXPBounty() const
{
	return 0;
}

int32 AGSPlayerState::GetGold() const
{
	return 0;
}

int32 AGSPlayerState::GetGoldBounty() const
{
	return 0;
}

int32 AGSPlayerState::GetPrimaryClipAmmo() const
{
	return 0;
}

int32 AGSPlayerState::GetPrimaryReserveAmmo() const
{
	return 0;
}

void AGSPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void AGSPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
}

void AGSPlayerState::KnockDownTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
}
