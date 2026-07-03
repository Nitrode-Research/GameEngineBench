// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Physics/PhysicalMaterialWithTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Weapon_SteadyAimingCamera, "ECR.Weapon.SteadyAimingCamera");

UECRRangedWeaponInstance::UECRRangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::PostLoad()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if WITH_EDITOR
void UECRRangedWeaponInstance::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::UpdateDebugVisualization()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif

void UECRRangedWeaponInstance::OnEquipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::OnUnequipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::Tick(float DeltaSeconds)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::ComputeSpreadRange(float& MinSpread, float& MaxSpread)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::AddSpread()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::OverrideHeat(float NewHeat)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRRangedWeaponInstance::RemoveHeat(float DeltaHeat)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


float UECRRangedWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
                                                       const FGameplayTagContainer* TargetTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


float UECRRangedWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial,
                                                               const FGameplayTagContainer* SourceTags,
                                                               const FGameplayTagContainer* TargetTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRRangedWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRRangedWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRRangedWeaponInstance::UpdateMultipliers(float DeltaSeconds)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

