// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetAccuracyAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetAccuracyAttributeSet.cpp
* @brief Attribute Set for Accuracy
* @attribute Accuracy - 
*/
URicochetAccuracyAttributeSet::URicochetAccuracyAttributeSet()
{
}

void URicochetAccuracyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetAccuracyAttributeSet, Accuracy, Params);
	
}

void URicochetAccuracyAttributeSet::OnRep_Accuracy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetAccuracyAttributeSet, Accuracy, OldValue);
}
