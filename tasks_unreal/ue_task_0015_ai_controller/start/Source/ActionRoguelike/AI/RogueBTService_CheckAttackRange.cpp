// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RogueBTService_CheckAttackRange.h"

#include "ActionRoguelike.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Core/RogueDeferredTaskSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueBTService_CheckAttackRange)

#if !UE_BUILD_SHIPPING
namespace DevelopmentOnly
{
	static bool GDrawDebugAttackRange = false;
	static FAutoConsoleVariableRef CVarDrawDebug_AttackRangeService(
		TEXT("game.drawdebugattackrange"),
		GDrawDebugAttackRange,
		TEXT("Enable debug rendering of the attack range services.\n"),
		ECVF_Cheat
		);
}
#endif

URogueBTService_CheckAttackRange::URogueBTService_CheckAttackRange()
{
}


void URogueBTService_CheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
}
