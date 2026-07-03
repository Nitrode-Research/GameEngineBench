// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetRangeAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetRangeAttributeSet.cpp
* @brief Attribute Set for Range
* @attribute Range - 
*/
URicochetRangeAttributeSet::URicochetRangeAttributeSet()
{
}

void URicochetRangeAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRangeAttributeSet, Range, Params);
	
}

void URicochetRangeAttributeSet::OnRep_Range(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRangeAttributeSet, Range, OldValue);
}
