// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetDexterityAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetDexterityAttributeSet.cpp
* @brief Attribute Set for Dexterity
* @attribute Dexterity - 
*/
URicochetDexterityAttributeSet::URicochetDexterityAttributeSet()
{
}

void URicochetDexterityAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetDexterityAttributeSet, Dexterity, Params);
	
}

void URicochetDexterityAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetDexterityAttributeSet, Dexterity, OldValue);
}
