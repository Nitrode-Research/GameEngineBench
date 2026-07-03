// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "AttributeSets/ClimaTemperatureAttributeSet.h"
#include "Net/UnrealNetwork.h"

/**
* @file ClimaTemperatureAttributeSet.cpp
* @brief Attribute Set for Temperature
* @attribute Temperature - 
*/
UClimaTemperatureAttributeSet::UClimaTemperatureAttributeSet()
{
}

void UClimaTemperatureAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(UClimaTemperatureAttributeSet, Temperature, Params);
	
}

void UClimaTemperatureAttributeSet::OnRep_Temperature(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClimaTemperatureAttributeSet, Temperature, OldValue);
}
