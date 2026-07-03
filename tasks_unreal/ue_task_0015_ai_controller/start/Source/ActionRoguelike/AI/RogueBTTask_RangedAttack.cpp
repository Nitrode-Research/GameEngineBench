// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RogueBTTask_RangedAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/RogueGameplayFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueBTTask_RangedAttack)


UDEPRECATED_URogueBTTask_RangedAttack::UDEPRECATED_URogueBTTask_RangedAttack()
{
}


EBTNodeResult::Type UDEPRECATED_URogueBTTask_RangedAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Failed;
}
