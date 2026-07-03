#include "AbilitySystem/Calculations/AlsxtGeecHoldingBreathDuration.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets//AlsxtHoldBreathAttributeSet.h"
#include "AbilitySystem/AttributeSets//AlsxtStaminaAttributeSet.h"

// Set up attribute capture definitions. These are used to specify which attributes the execution calculation needs.
struct FAlsxtHoldingBreathDurationStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentStamina);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxStamina);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentHoldBreathDuration);

	FAlsxtHoldingBreathDurationStatics()
	{
		// Capture the Target's Stamina attribute.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtStaminaAttributeSet, CurrentStamina, Target, false);
		// Capture the Target's MaxStamina attribute.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtStaminaAttributeSet, MaxStamina, Target, false);
		// Capture the Target's BreathDuration attribute.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtHoldBreathAttributeSet, CurrentHoldBreathDuration, Target, false);
	}
};

static const FAlsxtHoldingBreathDurationStatics& HoldingBreathDurationStatics()
{
	static FAlsxtHoldingBreathDurationStatics Statics;
	return Statics;
}

UAlsxtGeecHoldingBreathDuration::UAlsxtGeecHoldingBreathDuration()
{
	// Add the captured attributes to the RelevantAttributesToCapture array.
	RelevantAttributesToCapture.Add(HoldingBreathDurationStatics().CurrentStaminaDef);
	RelevantAttributesToCapture.Add(HoldingBreathDurationStatics().MaxStaminaDef);
	RelevantAttributesToCapture.Add(HoldingBreathDurationStatics().CurrentHoldBreathDurationDef);
}

void UAlsxtGeecHoldingBreathDuration::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetAbilitySystemComponent)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FAggregatorEvaluateParameters EvaluationParameters;
	
	// Get the captured attribute values.
	float CurrentStamina = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HoldingBreathDurationStatics().CurrentStaminaDef, EvaluationParameters, CurrentStamina);

	float MaxStamina = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HoldingBreathDurationStatics().MaxStaminaDef, EvaluationParameters, MaxStamina);

	float BaseBreathDuration = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HoldingBreathDurationStatics().CurrentHoldBreathDurationDef, EvaluationParameters, BaseBreathDuration);

	// Get any SetByCaller magnitude. The Gameplay Tag "HoldingBreath.StaminaCost" would be used in the GameplayEffect to specify the cost.
	float StaminaCostMagnitude = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("HoldingBreath.StaminaCost")));

	// Perform the calculation.
	// Example: Breath Duration scales with Stamina, but is also affected by a stamina cost.
	float BreathDuration = FMath::Max(0.0f, BaseBreathDuration * (CurrentStamina / MaxStamina) - StaminaCostMagnitude);
	
	// Add the output modifier to apply the calculated value.
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		HoldingBreathDurationStatics().CurrentHoldBreathDurationProperty,
		EGameplayModOp::Override,
		BreathDuration
	));
}
