// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).


#include "Actors/RicochetBullet.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Kismet/GameplayStatics.h"
#include "Physics/NetworkPhysicsComponent.h"


/**
* @file RicochetBullet.cpp
* @brief Base or Internal Magazine class, Health would be stored in Receiver
*/
ARicochetBullet::ARicochetBullet()
{
 	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent); // Attach to the root component
	CollisionSphere->SetSphereRadius(50.0f); // Set the desired radius
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // Or your desired collision profile

	// Optional: Enable overlap events if needed
	CollisionSphere->SetGenerateOverlapEvents(true);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	NetworkPhysicsComponent = CreateDefaultSubobject<UNetworkPhysicsComponent>(TEXT("NetworkPhysicsComponent"));
	NetworkPhysicsComponent->SetIsReplicated(true);
	SetPhysicsReplicationMode(EPhysicsReplicationMode::Resimulation);
	/* @todo Lower SetNetUpdateFrequency to optimize */
	SetNetUpdateFrequency(60.f); 

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetIsReplicated(false); // We'll manage replication
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 0.f;
}

float ARicochetBullet::GetRoundWeight()
{
	return RoundWeight;
}

void ARicochetBullet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARicochetBullet, InitialVelocity);
}

void ARicochetBullet::SetupSubclassCollisionIgnoring()
{
}

UAbilitySystemComponent* ARicochetBullet::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARicochetBullet::ActivateBullet(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);
	DeactivateBullet();
}

void ARicochetBullet::Multicast_ActivateBullet_Implementation(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
}

void ARicochetBullet::DeactivateBullet()
{
	if (!HasAuthority()) return;

	bIsActive = false;
	ProjectileMovement->Deactivate();
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ARicochetBullet::OnRep_IsActive()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	ProjectileMovement->Deactivate();
}

void ARicochetBullet::ApplyInitialVelocityEffects(UAbilitySystemComponent* InstigatorASC)
{
	InitialVelocity = FVector::ZeroVector;
}

void ARicochetBullet::OnRep_InitialVelocity()
{
	ProjectileMovement->Deactivate();
	ProjectileMovement->Velocity = FVector::ZeroVector;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
}

void ARicochetBullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARicochetBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARicochetBullet::InitializeBullet(FVector StartLocation, FRotator StartRotation, float InitialSpeed)
{
	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);
	InitialVelocityReplicated = FVector::ZeroVector;
	ProjectileMovement->Velocity = FVector::ZeroVector;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->Deactivate();
}

void ARicochetBullet::Server_CorrectClientPrediction(FVector ClientPredictedLocation)
{
}
