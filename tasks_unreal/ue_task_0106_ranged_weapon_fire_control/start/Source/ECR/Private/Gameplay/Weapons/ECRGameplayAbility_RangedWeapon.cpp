// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRGameplayAbility_RangedWeapon.h"
#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"
#include "Physics/ECRCollisionChannels.h"
#include "System/ECRLogChannels.h"
#include "AIController.h"
#include "System/Messages/ECRVerbMessage.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Gameplay/Weapons/ECRWeaponStateComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRGameplayAbilityTargetData_SingleTargetHit.h"
#include "DrawDebugHelpers.h"
#include "Gameplay/ECRGameplayBlueprintLibrary.h"

namespace ECRConsoleVariables
{
	static float DrawBulletTracesDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletTraceDuraton(
		TEXT("ECR.Weapon.DrawBulletTraceDuration"),
		DrawBulletTracesDuration,
		TEXT("Should we do debug drawing for bullet traces (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHits(
		TEXT("ECR.Weapon.DrawBulletHitDuration"),
		DrawBulletHitDuration,
		TEXT("Should we do debug drawing for bullet impacts (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitRadius = 3.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHitRadius(
		TEXT("ECR.Weapon.DrawBulletHitRadius"),
		DrawBulletHitRadius,
		TEXT(
			"When bullet hit debug drawing is enabled (see DrawBulletHitDuration), how big should the hit radius be? (in uu)"),
		ECVF_Default);
}

// Weapon fire will be blocked/canceled if the player has this tag
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked, "Ability.Weapon.NoFiring");

//////////////////////////////////////////////////////////////////////

UECRGameplayAbility_RangedWeapon::UECRGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRRangedWeaponInstance* UECRGameplayAbility_RangedWeapon::GetWeaponInstance(UObject* SourceObject) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


bool UECRGameplayAbility_RangedWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                          const FGameplayAbilityActorInfo* ActorInfo,
                                                          const FGameplayTagContainer* SourceTags,
                                                          const FGameplayTagContainer* TargetTags,
                                                          FGameplayTagContainer* OptionalRelevantTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UECRGameplayAbility_RangedWeapon::FindFirstPawnHitResult(const TArray<FHitResult>& HitResults)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRGameplayAbility_RangedWeapon::AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


ECollisionChannel UECRGameplayAbility_RangedWeapon::DetermineTraceChannel(
	FCollisionQueryParams& TraceParams, bool bIsSimulated) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FHitResult UECRGameplayAbility_RangedWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace,
                                                         float SweepRadius, bool bIsSimulated,
                                                         OUT TArray<FHitResult>& OutHitResults) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FVector UECRGameplayAbility_RangedWeapon::GetWeaponTargetingSourceLocation() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FTransform UECRGameplayAbility_RangedWeapon::GetTargetingTransform(APawn* SourcePawn,
                                                                   EECRAbilityTargetingSource Source) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FHitResult UECRGameplayAbility_RangedWeapon::DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace,
                                                                 float SweepRadius, bool bIsSimulated,
                                                                 OUT TArray<FHitResult>& OutHits,
                                                                 bool bSuppressDebugDraw) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FVector UECRGameplayAbility_RangedWeapon::GetSingleCameraTraceHitLocation(APawn* const AvatarPawn,
                                                                          UECRRangedWeaponInstance* WeaponData) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRGameplayAbility_RangedWeapon::PerformLocalTargeting(OUT TArray<FHitResult>& OutHits)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_RangedWeapon::TraceBulletsInCartridge(const FRangedWeaponFiringInput& InputData,
                                                               OUT TArray<FHitResult>& OutHits)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_RangedWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                       const FGameplayAbilityActorInfo* ActorInfo,
                                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                                       const FGameplayEventData* TriggerEventData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_RangedWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo,
                                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                                  bool bReplicateEndAbility, bool bWasCancelled)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_RangedWeapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData,
                                                                 FGameplayTag ApplicationTag)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameplayAbility_RangedWeapon::StartRangedWeaponTargeting()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FVector UECRGameplayAbility_RangedWeapon::GetBotFocusTargetOffset_Implementation() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

