// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/RicochetRoundAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file RicochetRoundAttributeSet.cpp
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
URicochetRoundAttributeSet::URicochetRoundAttributeSet()
{
}

void URicochetRoundAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, Mass, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, Diameter, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, FormFactor, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, EffectiveRange, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, BaseEffectiveRangeMultiplier, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MaxEffectiveRangeMultiplier, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, BaseImpactEnergyMultiplier, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MaxImpactEnergyMultiplier, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, InitialVelocityX, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, InitialVelocityY, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, InitialVelocityZ, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MuzzleVelocityMin, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MuzzleVelocityMax, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, Spread, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, GrazingAngleExponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MinPenetration, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, MaxPenetration, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, PenetrationNormalization, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, PenetrationNormalizationGrazing, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, PenetrationEntryAngleSpread, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, RicochetProbability, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, RicochetProbabilityGrazing, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, PenetrationExitAngleSpread, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, RicochetRestitution, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, RicochetFriction, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, RicochetSpread, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, ShotgunSpread, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, ShotgunVelocitySpread, Params);

	// Owner Only
	// Params.Condition = COND_OwnerOnly;
	// DOREPLIFETIME_WITH_PARAMS_FAST(URicochetRoundAttributeSet, StaminaRegeneration, Params);
}

void URicochetRoundAttributeSet::OnRep_Mass(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, Mass, OldValue);
}

void URicochetRoundAttributeSet::OnRep_Diameter(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, Diameter, OldValue);
}

void URicochetRoundAttributeSet::OnRep_FormFactor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, FormFactor, OldValue);
}

void URicochetRoundAttributeSet::OnRep_EffectiveRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, EffectiveRange, OldValue);
}

void URicochetRoundAttributeSet::OnRep_BaseEffectiveRangeMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, BaseEffectiveRangeMultiplier, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MaxEffectiveRangeMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MaxEffectiveRangeMultiplier, OldValue);
}

void URicochetRoundAttributeSet::OnRep_BaseImpactEnergyMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, BaseImpactEnergyMultiplier, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MaxImpactEnergyMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MaxImpactEnergyMultiplier, OldValue);
}

void URicochetRoundAttributeSet::OnRep_InitialVelocityX(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, InitialVelocityX, OldValue);
}

void URicochetRoundAttributeSet::OnRep_InitialVelocityY(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, InitialVelocityY, OldValue);
}

void URicochetRoundAttributeSet::OnRep_InitialVelocityZ(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, InitialVelocityZ, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MuzzleVelocityMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MuzzleVelocityMin, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MuzzleVelocityMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MuzzleVelocityMax, OldValue);
}

void URicochetRoundAttributeSet::OnRep_Spread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, Spread, OldValue);
}

void URicochetRoundAttributeSet::OnRep_GrazingAngleExponent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, GrazingAngleExponent, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MinPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MinPenetration, OldValue);
}

void URicochetRoundAttributeSet::OnRep_MaxPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, MaxPenetration, OldValue);
}

void URicochetRoundAttributeSet::OnRep_PenetrationNormalization(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, PenetrationNormalization, OldValue);
}

void URicochetRoundAttributeSet::OnRep_PenetrationNormalizationGrazing(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, PenetrationNormalizationGrazing, OldValue);
}

void URicochetRoundAttributeSet::OnRep_PenetrationEntryAngleSpread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, PenetrationEntryAngleSpread, OldValue);
}

void URicochetRoundAttributeSet::OnRep_RicochetProbability(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetProbability, OldValue);
}

void URicochetRoundAttributeSet::OnRep_RicochetProbabilityGrazing(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetProbabilityGrazing, OldValue);
}

void URicochetRoundAttributeSet::OnRep_PenetrationExitAngleSpread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, PenetrationExitAngleSpread, OldValue);
}

void URicochetRoundAttributeSet::OnRep_RicochetRestitution(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetRestitution, OldValue);
}

void URicochetRoundAttributeSet::OnRep_RicochetFriction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetFriction, OldValue);
}

void URicochetRoundAttributeSet::OnRep_RicochetSpread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetSpread, OldValue);
}

void URicochetRoundAttributeSet::OnRep_ShotgunSpread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, RicochetFriction, OldValue);
}

void URicochetRoundAttributeSet::OnRep_ShotgunVelocitySpread(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URicochetRoundAttributeSet, ShotgunVelocitySpread, OldValue);
}
