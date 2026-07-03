// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GASDemoCharacterMovementComponent.generated.h"


class AGASDemoCharacterBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSXT_API UGASDemoCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend AGASDemoCharacterBase;
	
public:
	// Sets default values for this component's properties
	UGASDemoCharacterMovementComponent();

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void BeginPlay() override;

protected:
	float MovementSpeedMultiplier = 1.f;
};