// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetStatusAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetStatusAttributeSet.cpp
* @brief Attribute Set for Rounds
* @attribute Health - 
* @attribute Dirtiness - 
* @attribute Wetness - Heavy Rain 0.5, Submerged 1.0, Affects Accuracy and could cause Catastrophic Failure in certain firearms like Shotguns
*/
URicochetStatusAttributeSet::URicochetStatusAttributeSet()
{
}

void URicochetStatusAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetStatusAttributeSet, Health, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetStatusAttributeSet, Dirtiness, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetStatusAttributeSet, Wetness, Params);
}

void URicochetStatusAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetStatusAttributeSet, Health, OldValue);
}

void URicochetStatusAttributeSet::OnRep_Dirtiness(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetStatusAttributeSet, Dirtiness, OldValue);
}

void URicochetStatusAttributeSet::OnRep_Wetness(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetStatusAttributeSet, Wetness, OldValue);
}
