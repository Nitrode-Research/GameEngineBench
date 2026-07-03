// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRWeaponStateComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/Pawn.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"
#include "NativeGameplayTags.h"
#include "Physics/PhysicalMaterialWithTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Zone, "Gameplay.Zone");

UECRWeaponStateComponent::UECRWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponStateComponent::ClientConfirmTargetData_Implementation(uint16 UniqueId, bool bSuccess,
                                                                      const TArray<uint8>& HitReplaces)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponStateComponent::AddUnconfirmedServerSideHitMarkers(const UObject* SourceObject,
                                                                  const FGameplayAbilityTargetDataHandle& InTargetData,
                                                                  const TArray<FHitResult>& FoundHits)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponStateComponent::ClientDrawServerHit_Implementation(FVector WorldHitLocation, EHitSuccess HitSuccess,
	FGameplayTag HitZone)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponStateComponent::UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRWeaponStateComponent::ShouldUpdateDamageInstigatedTime_Implementation(
	const FGameplayEffectContextHandle& EffectContext) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRWeaponStateComponent::ActuallyUpdateDamageInstigatedTime()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


double UECRWeaponStateComponent::GetTimeSinceLastHitNotification() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


EHitSuccess UECRWeaponStateComponent::ShouldShowHitAsSuccess_Implementation(const UObject* SourceObject, const FHitResult& Hit) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

