// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Abilities/GSGATA_Trace.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "WorldCollision.h"
#include "GSGATA_SphereTrace.generated.h"

UCLASS()
class GASSHOOTER_API AGSGATA_SphereTrace : public AGSGATA_Trace
{
	GENERATED_BODY()
	
public:
	AGSGATA_SphereTrace();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = "Trace")
	float TraceSphereRadius;

	UFUNCTION(BlueprintCallable)
	void Configure(
		UPARAM(DisplayName = "Start Location") const FGameplayAbilityTargetingLocationInfo& InStartLocation,
		UPARAM(DisplayName = "Aiming Tag") FGameplayTag InAimingTag,
		UPARAM(DisplayName = "Aiming Removal Tag") FGameplayTag InAimingRemovalTag,
		UPARAM(DisplayName = "Trace Profile") FCollisionProfileName InTraceProfile,
		UPARAM(DisplayName = "Filter") FGameplayTargetDataFilterHandle InFilter,
		UPARAM(DisplayName = "Reticle Class") TSubclassOf<AGameplayAbilityWorldReticle> InReticleClass,
		UPARAM(DisplayName = "Reticle Params") FWorldReticleParameters InReticleParams,
		UPARAM(DisplayName = "Ignore Blocking Hits") bool bInIgnoreBlockingHits = false,
		UPARAM(DisplayName = "Should Produce Target Data on Server") bool bInShouldProduceTargetDataOnServer = false,
		UPARAM(DisplayName = "Use Persistent Hit Results") bool bInUsePersistentHitResults = false,
		UPARAM(DisplayName = "Debug") bool bInDebug = false,
		UPARAM(DisplayName = "Trace Affects Aim Pitch") bool bInTraceAffectsAimPitch = true,
		UPARAM(DisplayName = "Trace From Player ViewPoint") bool bInTraceFromPlayerViewPoint = false,
		UPARAM(DisplayName = "Use Aiming Spread Mod") bool bInUseAimingSpreadMod = false,
		UPARAM(DisplayName = "Max Range") float InMaxRange = 999999.0f,
		UPARAM(DisplayName = "Trace Sphere Radius") float InTraceSphereRadius = 100.0f,
		UPARAM(DisplayName = "Base Targeting Spread") float InBaseSpread = 0.0f,
		UPARAM(DisplayName = "Aiming Spread Mod") float InAimingSpreadMod = 0.0f,
		UPARAM(DisplayName = "Targeting Spread Increment") float InTargetingSpreadIncrement = 0.0f,
		UPARAM(DisplayName = "Targeting Spread Max") float InTargetingSpreadMax = 0.0f,
		UPARAM(DisplayName = "Max Hit Results Per Trace") int32 InMaxHitResultsPerTrace = 1,
		UPARAM(DisplayName = "Number of Traces") int32 InNumberOfTraces = 1
	);

	virtual void SphereTraceWithFilter(TArray<FHitResult>& OutHitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, float Radius, FName ProfileName, const FCollisionQueryParams Params);

protected:
	virtual void DoTrace(TArray<FHitResult>& HitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params) override;
	virtual void ShowDebugTrace(TArray<FHitResult>& HitResults, EDrawDebugTrace::Type DrawDebugType, float Duration = 2.0f) override;

#if ENABLE_DRAW_DEBUG
	void DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0);
	void DrawDebugSphereTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
#endif // ENABLE_DRAW_DEBUG
};
