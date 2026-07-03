// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAttributeSet.h"

#include "RogueActionComponent.h"
#include "SharedGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


URogueMonsterAttributeSet::URogueMonsterAttributeSet() {}

void URogueHealthAttributeSet::OnRep_Health(FRogueAttribute OldValue) {}


void URogueHealthAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URogueHealthAttributeSet, Health);
}

void URoguePawnAttributeSet::ApplyMovementSpeed() {}
