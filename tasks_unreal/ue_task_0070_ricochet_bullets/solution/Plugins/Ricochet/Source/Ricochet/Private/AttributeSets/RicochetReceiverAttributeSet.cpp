// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetReceiverAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetReceiverAttributeSet.cpp
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
URicochetReceiverAttributeSet::URicochetReceiverAttributeSet()
{
}

void URicochetReceiverAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, MuzzleVelocityMin, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, MuzzleVelocityMax, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, InheritVelocity, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMin, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMax, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, AdditionalVelocityX, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, AdditionalVelocityY, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, AdditionalVelocityZ, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, FireRateMin, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, FireRateMax, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, FailureToExtractProbability, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, FailureToEjectProbability, Params);

	// Owner Only
	// Params.Condition = COND_OwnerOnly;
	// DOREPLIFETIME_WITH_PARAMS_FAST(URicochetReceiverAttributeSet, StaminaRegeneration, Params);
}

void URicochetReceiverAttributeSet::OnRep_MuzzleVelocityMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, MuzzleVelocityMin, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_MuzzleVelocityMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, MuzzleVelocityMax, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_InheritVelocity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, InheritVelocity, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_MuzzleVelocityMultiplierMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMin, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_MuzzleVelocityMultiplierMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMax, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_AdditionalVelocityX(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, AdditionalVelocityX, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_AdditionalVelocityY(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, AdditionalVelocityY, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_AdditionalVelocityZ(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, AdditionalVelocityZ, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_FireRateMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, FireRateMin, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_FireRateMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, FireRateMax, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_FailureToExtractProbability(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, FailureToExtractProbability, OldValue);
}

void URicochetReceiverAttributeSet::OnRep_FailureToEjectProbability(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetReceiverAttributeSet, FailureToEjectProbability, OldValue);
}
