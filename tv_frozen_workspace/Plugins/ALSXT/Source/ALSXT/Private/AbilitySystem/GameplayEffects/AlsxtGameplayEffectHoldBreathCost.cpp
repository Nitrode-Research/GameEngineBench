#include "AbilitySystem/GameplayEffects/AlsxtGameplayEffectHoldBreathCost.h"

#include "AbilitySystem/AttributeSets/AlsxtHoldBreathAttributeSet.h"
#include "AbilitySystem/Calculations/AlsxtGeecHoldingBreathCost.h"

UAlsxtGameplayEffectHoldBreathCost::UAlsxtGameplayEffectHoldBreathCost()
{
	FGameplayEffectAttributeCaptureDefinition CaptureDef;
	CaptureDef.AttributeToCapture = UAlsxtHoldBreathAttributeSet::GetCurrentHoldBreathDurationAttribute();

	FGameplayEffectExecutionDefinition ExecutionDefinition;
	// Set the execution calculation class to UAlsxtGeecHoldingBreathCost
	ExecutionDefinition.CalculationClass = UAlsxtGeecHoldingBreathCost::StaticClass();
	Executions.Add(ExecutionDefinition);

	// Add any modifiers or other properties needed for this effect
	// Example: Add a simple instant modifier to the BreathingRate attribute
	// Note: For complex calculations like breathing rate, you will primarily
	// rely on the execution calculation.
	/*FGameplayModifierInfo BreathingRateModifier;
	BreathingRateModifier.Attribute = UAlsxtAttributeSet::GetBreathingRateAttribute();
	BreathingRateModifier.ModifierOp = EGameplayModOp::Override;
	BreathingRateModifier.Magnitude.SetValue(1.0f);
	Modifiers.Add(BreathingRateModifier);*/

	DurationPolicy = EGameplayEffectDurationType::HasDuration;
}
