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
	if (!CollisionSphere) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARicochetBullet::StaticClass(), FoundActors); // Replace AMySubclassActor with your actual subclass

	for (AActor* SubclassActor : FoundActors)
	{
		if (SubclassActor && SubclassActor->GetOwner() == this) // Check if owned by this actor
		{
			CollisionSphere->IgnoreActorWhenMoving(SubclassActor, true);
			// If the subclass actor also has a component that needs to ignore this owner,
			// you might need to call IgnoreActorWhenMoving on that component as well.
			// if (UPrimitiveComponent* SubclassComp = SubclassActor->FindComponentByClass<UPrimitiveComponent>())
			// {
			//     SubclassComp->IgnoreActorWhenMoving(this, true);
			// }
		}
	}
}

UAbilitySystemComponent* ARicochetBullet::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARicochetBullet::ActivateBullet(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!HasAuthority()) return;

	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	bIsActive = true;
	// Seed the replicated launch velocity along the spawn direction at this project's
	// configured muzzle launch speed, so clients can predict the round's flight on activation.
	InitialVelocity = SpawnRotation.Vector() * 75000.0f;
	ProjectileMovement->Activate();

	Multicast_ActivateBullet(SpawnLocation, SpawnRotation);
}

void ARicochetBullet::Multicast_ActivateBullet_Implementation(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (HasAuthority()) return; // Server already handled this.

	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	bIsActive = true;
	ProjectileMovement->Activate();
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
	SetActorHiddenInGame(!bIsActive);
	SetActorEnableCollision(bIsActive);

	if (bIsActive)
	{
		ProjectileMovement->Activate();
	}
	else
	{
		ProjectileMovement->Deactivate();
	}
}

void ARicochetBullet::ApplyInitialVelocityEffects(UAbilitySystemComponent* InstigatorASC)
{
    if (InstigatorASC && InitialVelocityGE)
    {
        FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
        EffectContext.AddInstigator(InstigatorASC->GetOwner(), InstigatorASC->GetOwner());

        FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(InitialVelocityGE, 1.0f, EffectContext);
    	
        
        // Ideal: Use a custom execution calculation or custom attribute for velocity or a custom execution calculation
        // that modifies a temporary attribute or directly sets a value in the context.
        
        // Using Gameplay Effect: InitialVelocityGE applies a modifier to a custom "BulletInitialSpeed" attribute. Then, read this attribute to set InitialVelocity.
        // FGameplayAttribute BulletSpeedAttribute = UYourAttributeSet::GetBulletInitialSpeedAttribute();
        // float ModifiedSpeed = InstigatorASC->GetNumericAttributeBase(BulletSpeedAttribute);
        // InitialVelocity = GetActorForwardVector() * ModifiedSpeed;

        // Direct: GE directly provides a velocity override (ideally with GameplayEffectExecutionCalculation)
        // For simplicity, we'll just apply the GE and then read a value if it were an attribute.
        // GameplayEffect will modify an attribute on the bullet,
        // and that attribute will be used to calculate InitialVelocity.
        // Simplified direct application for demonstration:

        InitialVelocity = GetActorForwardVector() * 1000.0f; // Default for now
        
        // Apply the GE to the bullet's ASC
        if (AbilitySystemComponent)
        {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            // After applying, you'd read the attribute that defines the bullet's speed
            // and use it to set InitialVelocity.
        }
    }
}

void ARicochetBullet::OnRep_InitialVelocity()
{
    // Client receives replicated InitialVelocity and can start/adjust its predicted movement
    // (Update client-side physics based on the new InitialVelocity)
	// Client receives initial velocity from server
	if (!HasAuthority())
	{
		// Start client-side prediction using this initial velocity
		ProjectileMovement->Velocity = InitialVelocityReplicated;
		ProjectileMovement->InitialSpeed = InitialVelocityReplicated.Size();
		ProjectileMovement->MaxSpeed = InitialVelocityReplicated.Size();
		ProjectileMovement->SetUpdatedComponent(RootComponent);
		ProjectileMovement->Activate();
	}
}

void ARicochetBullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARicochetBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority()) // Server-side
	{
		// Simulate movement and potentially record state for rollback
		FVector CurrentLocation = GetActorLocation();
		FVector CurrentVelocity = ProjectileMovement->Velocity;

		// ... (Simulate movement, collision, etc.) ...

		// Store state for potential rollback if clients mispredict
		FBulletPredictionState ServerState;
		ServerState.Location = CurrentLocation;
		ServerState.Velocity = CurrentVelocity;
		PredictionHistory.Add(ServerState);
	}
	else // Client-side
	{
		// Client-side prediction
		if (!InitialVelocityReplicated.IsZero()) // Only predict after initial velocity is received
		{
			// Predict movement based on local state and initial velocity

			// Store client prediction for potential server correction
			FBulletPredictionState ClientPredictedState;
			ClientPredictedState.Location = GetActorLocation(); // Current predicted location
			// (Store other predicted state
			PredictionHistory.Add(ClientPredictedState);
		}
	}

}

void ARicochetBullet::InitializeBullet(FVector StartLocation, FRotator StartRotation, float InitialSpeed)
{
	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);

	// Calculate initial velocity based on rotation and speed
	InitialVelocityReplicated = StartRotation.Vector() * InitialSpeed;
	ProjectileMovement->Velocity = InitialVelocityReplicated; // Server's authoritative velocity
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed;
	ProjectileMovement->SetUpdatedComponent(RootComponent); // Assuming a root component for movement
	ProjectileMovement->Activate();
}

void ARicochetBullet::Server_CorrectClientPrediction(FVector ClientPredictedLocation)
{
	/** @todo This function would be called by the server, likely from an RPC, when a client's prediction deviates too much.
	* The server would then perform rollback based on PredictionHistory and send a correction to the client.
	*/
}
