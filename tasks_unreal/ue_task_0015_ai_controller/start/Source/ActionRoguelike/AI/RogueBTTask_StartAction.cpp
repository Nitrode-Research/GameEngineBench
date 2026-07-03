// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RogueBTTask_StartAction.h"
#include "AIController.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Core/RogueGameplayFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueBTTask_StartAction)


EBTNodeResult::Type URogueBTTask_StartAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Failed;
}
