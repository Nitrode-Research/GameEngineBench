// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Components/RicochetMagazineBase.h"
#include "AbilitySystemComponent.h"

/**
* @file RicochetMagazineBase.cpp
* @brief Brief description of what this header file provides.
*/

URicochetMagazineBase::URicochetMagazineBase()
{
	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
 
	// Set Replication Mode to Mixed for NPCs.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void URicochetMagazineBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
}

UAbilitySystemComponent* URicochetMagazineBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
