#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AlsxtGeecHoldingBreathDuration.generated.h"

UCLASS()
class ALSXT_API UAlsxtGeecHoldingBreathDuration : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	UAlsxtGeecHoldingBreathDuration();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
