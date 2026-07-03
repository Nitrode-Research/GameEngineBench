// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Executions/BmrDamageExecution.h"

// Bomber
#include "AbilitySystem/Attributes/BmrHealthAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrDamageExecution)

// Capture definitions
struct FBmrDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition OutcomingDamageDef;

	FBmrDamageStatics()
	{
		OutcomingDamageDef = FGameplayEffectAttributeCaptureDefinition(UBmrHealthAttributeSet::GetOutcomingDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FBmrDamageStatics& DamageStatics()
{
	static FBmrDamageStatics Statics;
	return Statics;
}

// Sets default capture
UBmrDamageExecution::UBmrDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().OutcomingDamageDef);
}

// Called whenever the owning gameplay effect is executed
void UBmrDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// Capture outcoming damage from source
	float OutcomingDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().OutcomingDamageDef, EvaluateParameters, OutcomingDamage);

	// Apply damage to target's IncomingDamage meta attribute
	const float DamageDone = FMath::Max(OutcomingDamage, 0.f);
	if (DamageDone > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBmrHealthAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, DamageDone));
	}
}