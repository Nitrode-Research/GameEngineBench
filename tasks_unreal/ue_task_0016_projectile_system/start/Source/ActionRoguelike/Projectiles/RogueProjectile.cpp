// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueProjectile.h"
#include "Components/SphereComponent.h"
#include "Projectiles/RogueProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "Performance/RogueActorPoolingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueProjectile)

TRACE_DECLARE_INT_COUNTER(COUNTER_GAME_ActiveProjectiles, TEXT("Game/ActiveProjectiles"));

ARogueProjectile::ARogueProjectile()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetCollisionProfileName("Projectile");
	SphereComp->SetCanEverAffectNavigation(false);
	RootComponent = SphereComp;

	NiagaraLoopComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComp"));
	NiagaraLoopComp->SetupAttachment(RootComponent);

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);

	MoveComp = CreateDefaultSubobject<URogueProjectileMovementComponent>(TEXT("ProjectileMoveComp"));
	MoveComp->bRotationFollowsVelocity = true;
	MoveComp->bInitialVelocityInLocalSpace = true;
	MoveComp->ProjectileGravityScale = 0.0f;
	MoveComp->InitialSpeed = 8000;

	bReplicates = true;
}


void ARogueProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


void ARogueProjectile::BeginPlay()
{
	Super::BeginPlay();
	TRACE_COUNTER_INCREMENT(COUNTER_GAME_ActiveProjectiles);
}

void ARogueProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	TRACE_COUNTER_DECREMENT(COUNTER_GAME_ActiveProjectiles);
}


void ARogueProjectile::PoolBeginPlay_Implementation()
{
}


void ARogueProjectile::PoolEndPlay_Implementation()
{
}

float ARogueProjectile::GetDefaultSpeed() const
{
	return MoveComp->InitialSpeed;
}

float ARogueProjectile::GetGravityScale() const
{
	return MoveComp->ProjectileGravityScale;
}


void ARogueProjectile::LifeSpanExpired()
{
}


void ARogueProjectile::OnActorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}


void ARogueProjectile::Explode_Implementation()
{
}
