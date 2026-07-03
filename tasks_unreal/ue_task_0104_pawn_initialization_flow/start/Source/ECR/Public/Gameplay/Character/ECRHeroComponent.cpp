#include "ECRHeroComponent.h"

#include "ECRCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Input/ECRInputComponent.h"
#include "Kismet/KismetMathLibrary.h"


UECRHeroComponent::UECRHeroComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHeroComponent::BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

