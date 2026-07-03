// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/Abilities/ECRGameplayAbility_Interact.h"
#include "AbilitySystemComponent.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Interaction/InteractionStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NativeGameplayTags.h"
#include "GUI/IndicatorSystem/ECRIndicatorManagerComponent.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "GUI/IndicatorSystem/IndicatorDescriptor.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Interaction_Activate, "Ability.Interaction.Activate");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Indicator_Category_Interaction, "GUI.Indicators.Category.Interaction");

UECRGameplayAbility_Interact::UECRGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_Interact::UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_Interact::TriggerInteraction()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

