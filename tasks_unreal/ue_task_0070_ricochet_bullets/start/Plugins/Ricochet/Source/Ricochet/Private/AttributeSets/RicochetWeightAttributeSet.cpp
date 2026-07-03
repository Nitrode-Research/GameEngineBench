// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetWeightAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetWeightAttributeSet.cpp
* @brief Attribute Set for Weight
* @attribute Weight - 
*/
URicochetWeightAttributeSet::URicochetWeightAttributeSet()
{
}

void URicochetWeightAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetWeightAttributeSet, Weight, Params);
	
}

void URicochetWeightAttributeSet::OnRep_Weight(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetWeightAttributeSet, Weight, OldValue);
}
