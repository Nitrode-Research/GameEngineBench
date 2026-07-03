// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/GSGATA_LineTrace.h"

AGSGATA_LineTrace::AGSGATA_LineTrace()
{
}

void AGSGATA_LineTrace::Configure(const FGameplayAbilityTargetingLocationInfo& InStartLocation, FGameplayTag InAimingTag, FGameplayTag InAimingRemovalTag, FCollisionProfileName InTraceProfile, FGameplayTargetDataFilterHandle InFilter, TSubclassOf<AGameplayAbilityWorldReticle> InReticleClass, FWorldReticleParameters InReticleParams, bool bInIgnoreBlockingHits, bool bInShouldProduceTargetDataOnServer, bool bInUsePersistentHitResults, bool bInDebug, bool bInTraceAffectsAimPitch, bool bInTraceFromPlayerViewPoint, bool bInUseAimingSpreadMod, float InMaxRange, float InBaseSpread, float InAimingSpreadMod, float InTargetingSpreadIncrement, float InTargetingSpreadMax, int32 InMaxHitResultsPerTrace, int32 InNumberOfTraces)
{
}

void AGSGATA_LineTrace::DoTrace(TArray<FHitResult>& HitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params)
{
	HitResults.Reset();
}

void AGSGATA_LineTrace::ShowDebugTrace(TArray<FHitResult>& HitResults, EDrawDebugTrace::Type DrawDebugType, float Duration)
{
}

#if ENABLE_DRAW_DEBUG
void AGSGATA_LineTrace::DrawDebugLineTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
}
#endif
