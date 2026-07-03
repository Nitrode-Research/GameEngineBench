// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRWeaponInstance.h"

#include "Cosmetics/ECRCosmeticStatics.h"
#include "Cosmetics/ECRPawnComponent_CharacterParts.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "System/ECRLogChannels.h"

UECRWeaponInstance::UECRWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponInstance::LinkAnimLayer()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
                                                 const FGameplayTagContainer* TargetTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial,
                                                         const FGameplayTagContainer* SourceTags,
                                                         const FGameplayTagContainer* TargetTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRWeaponInstance::GetArmorPenetration() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRWeaponInstance::GetIsDamageMelee() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRWeaponInstance::OnEquipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponInstance::OnUnequipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponInstance::UpdateFiringTime()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSubclassOf<UAnimInstance> UECRWeaponInstance::PickBestAnimLayer(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UAnimMontage* UECRWeaponInstance::GetSwitchToExecutionMontage() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UAnimMontage* UECRWeaponInstance::GetKillerExecutionMontage() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UAnimMontage* UECRWeaponInstance::GetVictimExecutionMontage(AActor* TargetActor) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UAnimMontage* UECRWeaponInstance::GetMontageFromSet(const FECRAnimMontageSelectionSet& SelectionSet,
                                                    AActor* TargetActor) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRWeaponInstance::LoadMontages()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRWeaponInstance::OnCharacterPartsChanged(UECRPawnComponent_CharacterParts* ComponentWithChangedParts)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

