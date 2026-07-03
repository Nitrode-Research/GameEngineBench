// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueProjectile_Dash.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/RoguePlayerController.h"
#include "Projectiles/RogueProjectileMovementComponent.h"
#include "Sound/SoundCue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueProjectile_Dash)




ARogueProjectile_Dash::ARogueProjectile_Dash()
{
}


void ARogueProjectile_Dash::BeginPlay()
{
	Super::BeginPlay();
}


void ARogueProjectile_Dash::Explode_Implementation()
{
}


void ARogueProjectile_Dash::TeleportInstigator()
{
}
