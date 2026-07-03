// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAction_ProjectileAttack.h"
#include "ActionRoguelike.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "Performance/RogueActorPoolingSubsystem.h"
#include "Player/RoguePlayerCharacter.h"
#include "Projectiles/RogueProjectilesSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueAction_ProjectileAttack)




void URogueAction_ProjectileAttack::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
}


void URogueAction_ProjectileAttack::AttackDelay_Elapsed(ARoguePlayerCharacter* InstigatorCharacter) {}
