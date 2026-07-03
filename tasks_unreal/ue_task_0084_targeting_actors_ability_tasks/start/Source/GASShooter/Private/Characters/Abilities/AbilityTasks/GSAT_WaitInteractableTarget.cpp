// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_WaitInteractableTarget.h"

UGSAT_WaitInteractableTarget::UGSAT_WaitInteractableTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxRange = 0.0f;
	TimerPeriod = 0.0f;
	bShowDebug = false;
	bTraceAffectsAimPitch = false;
}

UGSAT_WaitInteractableTarget* UGSAT_WaitInteractableTarget::WaitForInteractableTarget(UGameplayAbility* OwningAbility, FName TaskInstanceName, FCollisionProfileName InTraceProfile, float InMaxRange, float InTimerPeriod, bool bInShowDebug)
{
	return NewAbilityTask<UGSAT_WaitInteractableTarget>(OwningAbility, TaskInstanceName);
}

void UGSAT_WaitInteractableTarget::Activate()
{
	EndTask();
}

void UGSAT_WaitInteractableTarget::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

void UGSAT_WaitInteractableTarget::LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params, bool bLookForInteractableActor) const
{
	OutHitResult = FHitResult();
}

void UGSAT_WaitInteractableTarget::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch) const
{
	OutTraceEnd = TraceStart;
}

bool UGSAT_WaitInteractableTarget::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition) const
{
	ClippedPosition = CameraLocation;
	return false;
}

void UGSAT_WaitInteractableTarget::PerformTrace()
{
}

FGameplayAbilityTargetDataHandle UGSAT_WaitInteractableTarget::MakeTargetData(const FHitResult& HitResult) const
{
	return FGameplayAbilityTargetDataHandle();
}
