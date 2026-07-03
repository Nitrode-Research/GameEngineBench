// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupActor_HealthPotion.h"

#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "Player/RoguePlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RoguePickupActor_HealthPotion)


#define LOCTEXT_NAMESPACE "InteractableActors"


void ARoguePickupActor_HealthPotion::Interact_Implementation(AController* InstigatorController)
{
}


FText ARoguePickupActor_HealthPotion::GetInteractText_Implementation(AController* InstigatorController)
{
	return FText::GetEmpty();
}


#undef LOCTEXT_NAMESPACE
