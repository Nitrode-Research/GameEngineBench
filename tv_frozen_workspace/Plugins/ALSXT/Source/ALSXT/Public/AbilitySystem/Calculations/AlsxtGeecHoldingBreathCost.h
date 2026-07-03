// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AbilitySystem/AttributeSets/AlsxtHoldBreathAttributeSet.h"
#include "AbilitySystem/AttributeSets/AlsxtStaminaAttributeSet.h"
#include "AlsxtGeecHoldingBreathCost.generated.h"

// Struct to hold the captured attribute definitions
struct FALSXTExecutionCalculationStaminaAndHoldBreathStatics
{
	// Define the capture for the stamina attribute on the target
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentStamina);

	// Define the capture for the hold breath attribute on the target
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentHoldBreathDuration);

	FALSXTExecutionCalculationStaminaAndHoldBreathStatics()
	{
		// Capture Stamina attribute from the Target (character being affected)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtStaminaAttributeSet, CurrentStamina, Target, false);

		// Capture HoldBreath attribute from the Target
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtHoldBreathAttributeSet, CurrentHoldBreathDuration, Target, false);
	}
};

/**
 * Custom execution calculation to apply stamina and hold breath costs.
 */
UCLASS()
class ALSXT_API UAlsxtGeecHoldingBreathCost : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UAlsxtGeecHoldingBreathCost();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// Captures the Stamina attribute from the target
	FGameplayAttribute StaminaAttribute;
    
	// Captures the Hold Breath attribute from the target
	FGameplayAttribute HoldBreathAttribute;
};