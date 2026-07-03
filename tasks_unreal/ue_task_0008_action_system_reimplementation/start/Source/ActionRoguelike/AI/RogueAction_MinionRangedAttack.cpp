// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RogueAction_MinionRangedAttack.h"

#include "ActionRoguelike.h"
#include "ActionSystem/RogueActionComponent.h"
#include "AI/RogueAICharacter.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Projectiles/RogueProjectile.h"
#include "Projectiles/RogueProjectilesSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueAction_MinionRangedAttack)


void URogueAction_MinionRangedAttack::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
}

bool URogueAction_MinionRangedAttack::CanStart_Implementation(AActor* Instigator)
{
	return false;
}
