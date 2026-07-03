// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueActionEffect_Thorns.h"

#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Core/RogueGameplayFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueActionEffect_Thorns)


void URogueActionEffect_Thorns::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
}


void URogueActionEffect_Thorns::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
}


void URogueActionEffect_Thorns::OnHealthChanged(float NewValue, const FAttributeModification& AttributeModification) {}
