// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Equipment/ECREquipmentInstance_EquipmentMod.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/Weapons/ECRWeaponInstance.h"
#include "Net/UnrealNetwork.h"

UECREquipmentInstance::UECREquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UWorld* UECREquipmentInstance::GetWorld() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECREquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


APawn* UECREquipmentInstance::GetPawn() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


APawn* UECREquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECREquipmentInstance::SpawnEquipmentActors(const TArray<FECREquipmentActorToSpawn>& ActorsToSpawn)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::DestroyEquipmentActors()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::OnEquipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::OnUnequipped()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::SetVisibility(const bool bNewVisible)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::OnRep_bVisible()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentInstance::OnRep_Instigator()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

