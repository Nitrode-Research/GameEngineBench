// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetRecoilAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetRecoilAttributeSet.cpp
* @brief Attribute Set for Weight
* @attribute Weight - 
*/
URicochetRecoilAttributeSet::URicochetRecoilAttributeSet()
{
}

void URicochetRecoilAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRecoilAttributeSet, Recoil, Params);
	
}

void URicochetRecoilAttributeSet::OnRep_Recoil(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRecoilAttributeSet, Recoil, OldValue);
}
