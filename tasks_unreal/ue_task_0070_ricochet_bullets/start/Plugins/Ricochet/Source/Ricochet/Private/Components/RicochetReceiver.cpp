// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).


#include "Components/RicochetReceiver.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetReceiver.cpp
* @brief Core firearm component that defines Calibers, Fire Modes, Fire Rate, Base Recoil, and has Barrel and Magazine Attachments
*/
URicochetReceiver::URicochetReceiver()
{

	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
 
	// Set Replication Mode to Minimal for Server Spawned Actors.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void URicochetReceiver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, FireMode, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ShootingBlocked, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Ammo, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CycleAmmoCount, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CycleAmmoPos, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ChamberedBullet, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Shooting, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Spooling, Parameters)
}

float URicochetReceiver::GetReceiverWeight() const
{
	return 0.0f;
}

float URicochetReceiver::GetTotalWeaponWeight()
{
	return 0.0f;
}

float URicochetReceiver::GetTotalCalculatedRecoil()
{
	return 0.0f;
}

UAbilitySystemComponent* URicochetReceiver::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}



