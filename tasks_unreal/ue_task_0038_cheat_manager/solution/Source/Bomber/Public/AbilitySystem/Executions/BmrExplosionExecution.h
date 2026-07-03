// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "BmrExplosionExecution.generated.h"

/**
 * Handles bomb explosion with chain reaction across multiple cells:
 * 1. Captures OutcomingDamage from source actor
 * 2. Calculates explosion cells with bomb chain reactions
 * 3. Applies damage to multiple target actors through IncomingDamage meta attribute
 */
UCLASS()
class BOMBER_API UBmrExplosionExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	/** Sets default capture. */
	UBmrExplosionExecution();

protected:
	/** Called whenever the owning gameplay effect is executed. */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
