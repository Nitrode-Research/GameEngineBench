#include "AbilitySystem/Calculations/AlsxtGeecHoldingBreathCost.h"
#include "AbilitySystem/AttributeSets/AlsxtStaminaAttributeSet.h"
#include "AbilitySystem/AttributeSets/AlsxtHoldBreathAttributeSet.h"
#include "AbilitySystem/AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

static const FALSXTExecutionCalculationStaminaAndHoldBreathStatics& GetStaminaAndHoldBreathStatics()
{
    static FALSXTExecutionCalculationStaminaAndHoldBreathStatics Statics;
    return Statics;
}

// Define the captured attributes
struct FHoldingBreathCostCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentStamina);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentHoldBreathDuration);

	FHoldingBreathCostCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtStaminaAttributeSet, CurrentStamina, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAlsxtHoldBreathAttributeSet, CurrentHoldBreathDuration, Target, true);
	}
};

static const FHoldingBreathCostCapture& GetHoldingBreathCostCapture()
{
	static FHoldingBreathCostCapture HoldingBreathCostCapture;
	return HoldingBreathCostCapture;
}

UAlsxtGeecHoldingBreathCost::UAlsxtGeecHoldingBreathCost()
{
	// Capture the target's Stamina attribute
	FGameplayEffectAttributeCaptureDefinition StaminaDef;
	StaminaDef.AttributeToCapture = UAlsxtStaminaAttributeSet::GetCurrentStaminaAttribute();
	StaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaDef.bSnapshot = false; // We don't need a snapshot
	RelevantAttributesToCapture.Add(StaminaDef);

	// Capture the target's Hold Breath attribute
	FGameplayEffectAttributeCaptureDefinition HoldBreathDef;
	HoldBreathDef.AttributeToCapture = UAlsxtHoldBreathAttributeSet::GetCurrentHoldBreathDurationAttribute();
	HoldBreathDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	HoldBreathDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(HoldBreathDef);
}

void UAlsxtGeecHoldingBreathCost::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Get the target and effect data
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer SourceTags = Spec.CapturedSourceTags.GetActorTags();
	const FGameplayTagContainer TargetTags = Spec.CapturedTargetTags.GetActorTags();

	// Get the duration of the Gameplay Effect
	const float Duration = Spec.GetDuration();

	// The effect should have a duration to calculate over time.
	if (Duration <= 0.0f)
	{
		return;
	}

	// Capture the current Stamina and Hold Breath values
	float Stamina = 0.0f;
	FGameplayEffectAttributeCaptureDefinition StaminaDef;
	StaminaDef.AttributeToCapture = StaminaAttribute;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(StaminaDef, FAggregatorEvaluateParameters(), Stamina);

	float HoldBreath = 0.0f;
	FGameplayEffectAttributeCaptureDefinition HoldBreathDef;
	StaminaDef.AttributeToCapture = HoldBreathAttribute;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HoldBreathDef, FAggregatorEvaluateParameters(), HoldBreath);

	// Get any set by caller magnitudes. For example, a "cost per second"
	float StaminaCostPerSecond = 0.0f;
	//Spec.GetSetByCallerMagnitudeByName(FName("StaminaCostPerSecond"), false, StaminaCostPerSecond);
	Spec.GetSetByCallerMagnitude(FName("StaminaCostPerSecond"), false, StaminaCostPerSecond);
    
	float BreathCostPerSecond = 0.0f;
	Spec.GetSetByCallerMagnitude(FName("BreathCostPerSecond"), false, BreathCostPerSecond);

	// Calculate the total costs based on the effect's duration
	const float TotalStaminaCost = StaminaCostPerSecond * Duration;
	const float TotalBreathCost = BreathCostPerSecond * Duration;

	// Apply the costs to the respective attributes
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UAlsxtStaminaAttributeSet::GetCurrentStaminaAttribute(), EGameplayModOp::Additive, -TotalStaminaCost));
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UAlsxtHoldBreathAttributeSet::GetCurrentHoldBreathDurationAttribute(), EGameplayModOp::Additive, -TotalBreathCost));
}
