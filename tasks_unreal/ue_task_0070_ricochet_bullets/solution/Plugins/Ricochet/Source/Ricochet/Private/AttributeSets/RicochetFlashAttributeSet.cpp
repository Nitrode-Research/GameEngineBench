// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetFlashAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetFlashAttributeSet.cpp
* @brief Attribute Set for Flash
* @attribute Flash - 
*/
URicochetFlashAttributeSet::URicochetFlashAttributeSet()
{
}

void URicochetFlashAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetFlashAttributeSet, Flash, Params);
	
}

void URicochetFlashAttributeSet::OnRep_Flash(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetFlashAttributeSet, Flash, OldValue);
}
