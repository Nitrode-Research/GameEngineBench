// MIT

#include "AbilitySystem/Character/GASDemoCharacterMovementComponent.h"

#include "AbilitySystem/FunctionLibrary/AlsxtAbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AlsxtMovementAttributeSet.h"

// Sets default values for this component's properties
UGASDemoCharacterMovementComponent::UGASDemoCharacterMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

float UGASDemoCharacterMovementComponent::GetMaxSpeed() const
{
	return Super::GetMaxSpeed() * MovementSpeedMultiplier;
}

float UGASDemoCharacterMovementComponent::GetMaxAcceleration() const
{
	return Super::GetMaxAcceleration();
}

void UGASDemoCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}