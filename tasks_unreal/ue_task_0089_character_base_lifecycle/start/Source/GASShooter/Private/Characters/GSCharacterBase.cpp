// Copyright 2020 Dan Kestranek.

#include "Characters/GSCharacterBase.h"

AGSCharacterBase::AGSCharacterBase(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

UAbilitySystemComponent* AGSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AGSCharacterBase::IsAlive() const
{
	return false;
}

int32 AGSCharacterBase::GetAbilityLevel(EGSAbilityInputID AbilityID) const
{
	return 1;
}

void AGSCharacterBase::RemoveCharacterAbilities()
{
}

void AGSCharacterBase::Die()
{
}

void AGSCharacterBase::FinishDying()
{
	Destroy();
}

void AGSCharacterBase::AddDamageNumber(float Damage, FGameplayTagContainer DamageNumberTags)
{
}

int32 AGSCharacterBase::GetCharacterLevel() const
{
	return 1;
}

float AGSCharacterBase::GetHealth() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMaxHealth() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMana() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMaxMana() const
{
	return 0.0f;
}

float AGSCharacterBase::GetStamina() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMaxStamina() const
{
	return 0.0f;
}

float AGSCharacterBase::GetShield() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMaxShield() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeed() const
{
	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeedBaseValue() const
{
	return 0.0f;
}

void AGSCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AGSCharacterBase::AddCharacterAbilities()
{
}

void AGSCharacterBase::InitializeAttributes()
{
}

void AGSCharacterBase::AddStartupEffects()
{
}

void AGSCharacterBase::ShowDamageNumber()
{
}

void AGSCharacterBase::SetHealth(float Health)
{
}

void AGSCharacterBase::SetMana(float Mana)
{
}

void AGSCharacterBase::SetStamina(float Stamina)
{
}

void AGSCharacterBase::SetShield(float Shield)
{
}
