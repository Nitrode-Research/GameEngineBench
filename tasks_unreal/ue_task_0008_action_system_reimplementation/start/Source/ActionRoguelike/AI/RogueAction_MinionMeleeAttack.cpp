// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAction_MinionMeleeAttack.h"

#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Animation/RogueAnimInstance.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"


URogueAction_MinionMeleeAttack::URogueAction_MinionMeleeAttack() {}


void URogueAction_MinionMeleeAttack::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
}

void URogueAction_MinionMeleeAttack::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
}

void URogueAction_MinionMeleeAttack::OnMeleeOverlaps(const TArray<FOverlapResult>& Overlaps) {}
