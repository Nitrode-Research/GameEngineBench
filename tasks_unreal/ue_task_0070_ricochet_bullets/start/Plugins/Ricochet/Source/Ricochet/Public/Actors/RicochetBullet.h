// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h" 
#include "Components/SphereComponent.h"
#include "Data/RicochetEnums.h"
#include "Physics/NetworkPhysicsComponent.h"
#include "RicochetBullet.generated.h"

class UGameplayEffect;

/**
* @file RicochetBullet.h
* @brief Projectile Actor that performs Server-side Deterministic Simulation with GAS attributes Calculations and Effects
*/
UCLASS()
class RICOCHET_API ARicochetBullet : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARicochetBullet();

	virtual void Tick(float DeltaTime) override;

	// WORLD WIDE SETTINGS
	
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	// UEBMaterialResponseMap* MaterialResponseMap;

	// A curve table or data asset is best for data-driven design
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	class UCurveFloat* MachDragCurve;

	float RoundWeight = 1.0;

	virtual float GetRoundWeight();

	// ATTRIBUTES

	// Initial velocity property, potentially modified by GAS
	UPROPERTY(ReplicatedUsing=OnRep_InitialVelocity)
	FVector InitialVelocity;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shotgun") 
	bool Shotgun=false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shotgun", meta = (EditCondition = "Shotgun")) 
	float ShotSpread=0.01;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shotgun", meta = (EditCondition = "Shotgun")) 
	float ShotVelocitySpread = 0.01;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float Mass = 0.005;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float Diameter = 0.556;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float FormFactor = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float MuzzleVelocityMin = 100000.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float MuzzleVelocityMax = 100000.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight") 
	float Spread = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float GrazingAngleExponent = 2.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float MinPenetration = 10.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float MaxPenetration = 20.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float PenetrationNormalization = 0.5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float PenetrationNormalizationGrazing = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float PenetrationEntryAngleSpread = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float PenetrationExitAngleSpread = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float RicochetProbability = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float RicochetProbabilityGrazing = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float RicochetRestitution = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float RicochetFriction = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float RicochetSpread = 0.1;

	// Properties

	// CLASS SETTINGS

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	bool SpeedControlsRicochetProbability = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	bool AddImpulse = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	float ImpulseMultiplier = 1.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	ERicochetPenetrationTraceMode DefaultPenTraceType = ERicochetPenetrationTraceMode::PT_BackTrace;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	bool MaterialDensityControlsPenetrationDepth = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact") 
	bool MaterialRestitutionControlsRicochet = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Replication") 
	bool ReliableReplication = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (ToolTip = "Allow components to collide, intended for use with trigger volumes. Do not use for actual collisions.")) 
	bool AllowComponentCollisions = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision") 
	TEnumAsByte<ECollisionChannel> TraceChannel;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision") 
	bool TraceComplex;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision") 
	float CollisionMargin = 1.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (ToolTip = "Bullets with lower velocity will automatically despawn on impact, never despawn if set to zero or negative")) 
	float DespawnVelocity = 100.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision") 
	TArray<AActor*> IgnoredActors;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation") 
	bool DoFirstStepImmediately = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation", meta = (EditCondition = "DoFirstStepImmediately")) 
	bool RandomFirstStepDelta = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation") 
	bool FixedStep = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation", meta = (EditCondition = "FixedStep")) 
	float FixedStepSeconds = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation") 
	int MaxTracesPerStep = 8;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Retrace") 
	bool Retrace = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Retrace") 
	bool RetraceOnAnotherChannel = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Retrace", meta=(EditCondition="RetraceOnAnotherChannel")) 
	TEnumAsByte<ECollisionChannel> RetraceChannel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rotation") 
	bool RotateActor = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rotation") 
	bool RotateRandomRoll = true;	

	// Functions

	// Function to apply GAS effects and set initial velocity
	void ApplyInitialVelocityEffects(UAbilitySystemComponent* InstigatorASC);

	void SetupSubclassCollisionIgnoring();

	// Override Functions

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// Replicated Functions

	// Server function to activate the bullet from the pool
	UFUNCTION(BlueprintCallable, Category = "BulletPool")
	void ActivateBullet(const FVector& SpawnLocation, const FRotator& SpawnRotation);
    
	// Replicated callback to notify clients that the bullet is active
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ActivateBullet(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	// Server-side: Spawn and initialize the bullet
	void InitializeBullet(FVector StartLocation, FRotator StartRotation, float InitialSpeed);

	// Function to deactivate the bullet and return it to the pool
	void DeactivateBullet();

	// A replicated variable to track the bullet's active state
	UPROPERTY(ReplicatedUsing=OnRep_IsActive)
	bool bIsActive;

	// RepNotify handler for bIsActive
	UFUNCTION()
	void OnRep_IsActive();

protected:
	// FUNCTIONS

	// Override Functions
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// REPLICATED FUNCTIONS

	UFUNCTION()
	void OnRep_InitialVelocity();

	// Server-side function to apply rollback if necessary
	void Server_CorrectClientPrediction(FVector ClientPredictedLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network Physics")
	UNetworkPhysicsComponent* NetworkPhysicsComponent;

	// Properties

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	USphereComponent* CollisionSphere;

	// Ability System Component for the bullet
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// Gameplay Effect to apply to the bullet's initial velocity
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> InitialVelocityGE;

	UPROPERTY(ReplicatedUsing = OnRep_InitialVelocity)
	FVector_NetQuantize InitialVelocityReplicated;

	// Client-side prediction state for rollback
	struct FBulletPredictionState
	{
		FVector Location;
		FVector Velocity;
		// Add other relevant state like rotation, etc.
	};

	TArray<FBulletPredictionState> PredictionHistory;

private:
	// Example for deriving initial velocity from a GAS attribute
	// In a real scenario, this would likely be passed from the spawning ability
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayAttribute InitialSpeedAttribute; 
	
};
