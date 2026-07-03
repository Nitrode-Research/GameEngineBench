// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RogueAIController.h"

#include "ActionRoguelike.h"
#include "RogueAICharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Player/RoguePlayerCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueAIController)


ARogueAIController::ARogueAIController()
{
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
}


void ARogueAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ARogueAIController::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();
}


EBlackboardNotificationResult ARogueAIController::OnTargetActorChanged(const UBlackboardComponent& Comp, FBlackboard::FKey KeyID)
{
	return EBlackboardNotificationResult::ContinueObserving;
}
