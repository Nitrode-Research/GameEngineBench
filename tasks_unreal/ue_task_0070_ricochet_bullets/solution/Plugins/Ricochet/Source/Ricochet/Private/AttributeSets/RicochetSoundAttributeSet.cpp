// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetSoundAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetSoundAttributeSet.cpp
* @brief Attribute Set for Sound
* @attribute Sound - 
*/
URicochetSoundAttributeSet::URicochetSoundAttributeSet()
{
}

void URicochetSoundAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetSoundAttributeSet, Sound, Params);
	
}

void URicochetSoundAttributeSet::OnRep_Sound(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetSoundAttributeSet, Sound, OldValue);
}
