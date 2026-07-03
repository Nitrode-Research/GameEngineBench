// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetBarrelLengthAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetBarrelLengthAttributeSet.cpp
* @brief Attribute Set for BarrelLength
* @attribute BarrelLength - 
*/
URicochetBarrelLengthAttributeSet::URicochetBarrelLengthAttributeSet()
{
}

void URicochetBarrelLengthAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetBarrelLengthAttributeSet, BarrelLength, Params);
	
}

void URicochetBarrelLengthAttributeSet::OnRep_BarrelLength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetBarrelLengthAttributeSet, BarrelLength, OldValue);
}
