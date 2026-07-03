// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueProjectile_Magic.h"
#include "ActionRoguelike.h"

#include "Components/SphereComponent.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Projectiles/RogueProjectileMovementComponent.h"
#include "ActionSystem/RogueActionEffect.h"
#include "Core/RogueDeferredTaskSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueProjectile_Magic)


ARogueProjectile_Magic::ARogueProjectile_Magic()
{
}


void ARogueProjectile_Magic::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


void ARogueProjectile_Magic::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}
