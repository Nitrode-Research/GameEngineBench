// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/GSDamageExecutionCalc.h"

UGSDamageExecutionCalc::UGSDamageExecutionCalc()
{
	HeadShotMultiplier = 1.0f;
}

void UGSDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
}
