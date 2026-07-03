// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetReceiverGatlingAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetReceiverGatlingAttributeSet.cpp
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
URicochetReceiverGatlingAttributeSet::URicochetReceiverGatlingAttributeSet()
{
}

void URicochetReceiverGatlingAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverGatlingAttributeSet, GatlingRps, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverGatlingAttributeSet, GatlingSpoolUpTime, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverGatlingAttributeSet, GatlingSpoolDownTime, Params);

	// Owner Only
	// Params.Condition = COND_OwnerOnly;
	// DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverGatlingAttributeSet, StaminaRegeneration, Params);
}

void URicochetReceiverGatlingAttributeSet::OnRep_GatlingRps(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverGatlingAttributeSet, GatlingRps, OldValue);
}

void URicochetReceiverGatlingAttributeSet::OnRep_GatlingSpoolUpTime(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverGatlingAttributeSet, GatlingSpoolUpTime, OldValue);
}

void URicochetReceiverGatlingAttributeSet::OnRep_GatlingSpoolDownTime(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverGatlingAttributeSet, GatlingSpoolDownTime, OldValue);
}
