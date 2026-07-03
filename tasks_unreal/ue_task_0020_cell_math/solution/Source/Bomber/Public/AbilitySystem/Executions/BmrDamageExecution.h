// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "BmrDamageExecution.generated.h"

/**
 * Captures OutcomingDamage from source actor → Applies to target's IncomingDamage meta attribute.
 * Source damage can be modified by effects before capture.
 */
UCLASS()
class BOMBER_API UBmrDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	/** Sets default capture. */
	UBmrDamageExecution();

protected:
	/** Called whenever the owning gameplay effect is executed. */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
