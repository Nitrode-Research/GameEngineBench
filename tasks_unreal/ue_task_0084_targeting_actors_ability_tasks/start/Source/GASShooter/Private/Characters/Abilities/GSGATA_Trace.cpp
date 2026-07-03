// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/GSGATA_Trace.h"
#include "Abilities/GameplayAbility.h"

AGSGATA_Trace::AGSGATA_Trace()
{
	bDestroyOnConfirmation = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	MaxHitResultsPerTrace = 1;
	NumberOfTraces = 1;
	bIgnoreBlockingHits = false;
	bTraceAffectsAimPitch = true;
	bTraceFromPlayerViewPoint = false;
	MaxRange = 999999.0f;
	bUseAimingSpreadMod = false;
	BaseSpread = 0.0f;
	AimingSpreadMod = 0.0f;
	TargetingSpreadIncrement = 0.0f;
	TargetingSpreadMax = 0.0f;
	CurrentTargetingSpread = 0.0f;
	bUsePersistentHitResults = false;
}

void AGSGATA_Trace::ResetSpread()
{
}

float AGSGATA_Trace::GetCurrentSpread() const
{
	return 0.0f;
}

void AGSGATA_Trace::SetStartLocation(const FGameplayAbilityTargetingLocationInfo& InStartLocation)
{
	StartLocation = InStartLocation;
}

void AGSGATA_Trace::SetShouldProduceTargetDataOnServer(bool bInShouldProduceTargetDataOnServer)
{
	ShouldProduceTargetDataOnServer = bInShouldProduceTargetDataOnServer;
}

void AGSGATA_Trace::SetDestroyOnConfirmation(bool bInDestroyOnConfirmation)
{
	bDestroyOnConfirmation = bInDestroyOnConfirmation;
}

void AGSGATA_Trace::StartTargeting(UGameplayAbility* Ability)
{
	OwningAbility = Ability;
	SourceActor = Ability ? Ability->GetCurrentActorInfo()->AvatarActor.Get() : nullptr;
}

void AGSGATA_Trace::ConfirmTargetingAndContinue()
{
	TargetDataReadyDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
}

void AGSGATA_Trace::CancelTargeting()
{
	CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
}

void AGSGATA_Trace::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
}

void AGSGATA_Trace::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AGSGATA_Trace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AGSGATA_Trace::LineTraceWithFilter(TArray<FHitResult>& OutHitResults, const UWorld* World, const FGameplayTargetDataFilterHandle FilterHandle, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params)
{
	OutHitResults.Reset();
}

void AGSGATA_Trace::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch)
{
	OutTraceEnd = TraceStart;
}

bool AGSGATA_Trace::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition)
{
	ClippedPosition = CameraLocation;
	return false;
}

void AGSGATA_Trace::StopTargeting()
{
	SetActorTickEnabled(false);
}

FGameplayAbilityTargetDataHandle AGSGATA_Trace::MakeTargetData(const TArray<FHitResult>& HitResults) const
{
	return FGameplayAbilityTargetDataHandle();
}

TArray<FHitResult> AGSGATA_Trace::PerformTrace(AActor* InSourceActor)
{
	return TArray<FHitResult>();
}

AGameplayAbilityWorldReticle* AGSGATA_Trace::SpawnReticleActor(FVector Location, FRotator Rotation)
{
	return nullptr;
}

void AGSGATA_Trace::DestroyReticleActors()
{
	ReticleActors.Reset();
}
