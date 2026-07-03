// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/GSGATA_SphereTrace.h"

AGSGATA_SphereTrace::AGSGATA_SphereTrace()
{
	TraceSphereRadius = 0.0f;
}

void AGSGATA_SphereTrace::Configure(const FGameplayAbilityTargetingLocationInfo& InStartLocation, FGameplayTag InAimingTag, FGameplayTag InAimingRemovalTag, FCollisionProfileName InTraceProfile, FGameplayTargetDataFilterHandle InFilter, TSubclassOf<AGameplayAbilityWorldReticle> InReticleClass, FWorldReticleParameters InReticleParams, bool bInIgnoreBlockingHits, bool bInShouldProduceTargetDataOnServer, bool bInUsePersistentHitResults, bool bInDebug, bool bInTraceAffectsAimPitch, bool bInTraceFromPlayerViewPoint, bool bInUseAimingSpreadMod, float InMaxRange, float InTraceSphereRadius, float InBaseSpread, float InAimingSpreadMod, float InTargetingSpreadIncrement, float InTargetingSpreadMax, int32 InMaxHitResultsPerTrace, int32 InNumberOfTraces)
{
	TraceSphereRadius = InTraceSphereRadius;
}

void AGSGATA_SphereTrace::SphereTraceWithFilter(TArray<FHitResult>& OutHitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, float InRadius, FName ProfileName, const FCollisionQueryParams Params)
{
	OutHitResults.Reset();
}

void AGSGATA_SphereTrace::DoTrace(TArray<FHitResult>& HitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params)
{
	HitResults.Reset();
}

void AGSGATA_SphereTrace::ShowDebugTrace(TArray<FHitResult>& HitResults, EDrawDebugTrace::Type DrawDebugType, float Duration)
{
}

#if ENABLE_DRAW_DEBUG
void AGSGATA_SphereTrace::DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float InRadius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
}

void AGSGATA_SphereTrace::DrawDebugSphereTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float InRadius, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
}
#endif
