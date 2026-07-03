// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueActionEffect.h"
#include "ActionSystem/RogueActionComponent.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueActionEffect)




URogueActionEffect::URogueActionEffect() {}


void URogueActionEffect::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
}


void URogueActionEffect::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
}


float URogueActionEffect::GetTimeRemaining() const
{
	return 0.0f;
}


void URogueActionEffect::ExecutePeriodicEffect_Implementation(AActor* Instigator) {}
