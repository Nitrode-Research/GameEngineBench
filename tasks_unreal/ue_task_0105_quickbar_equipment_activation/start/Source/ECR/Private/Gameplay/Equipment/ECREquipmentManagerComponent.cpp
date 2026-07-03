// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Character.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Equipment/ECREquipmentInstance_EquipmentMod.h"
#include "System/ECRLogChannels.h"

//////////////////////////////////////////////////////////////////////
// FECRAppliedEquipmentEntry

FString FECRAppliedEquipmentEntry::GetDebugString() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// FECREquipmentList

void FECREquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECREquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECREquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRAbilitySystemComponent* FECREquipmentList::GetAbilitySystemComponent() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECREquipmentInstance* FECREquipmentList::AddEntry(TSubclassOf<UECREquipmentDefinition> EquipmentDefinition)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void FECREquipmentList::RemoveEntry(UECREquipmentInstance* Instance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


//////////////////////////////////////////////////////////////////////
// UECREquipmentManagerComponent

UECREquipmentManagerComponent::UECREquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , EquipmentList(this)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECREquipmentInstance* UECREquipmentManagerComponent::EquipItem(TSubclassOf<UECREquipmentDefinition> EquipmentClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECREquipmentManagerComponent::SetItemVisible(UECREquipmentInstance* ItemInstance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentManagerComponent::SetItemsInvisible(TArray<UECREquipmentInstance*> Items)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentManagerComponent::UnequipItem(UECREquipmentInstance* ItemInstance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECREquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch,
                                                        FReplicationFlags* RepFlags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECREquipmentManagerComponent::InitializeComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECREquipmentManagerComponent::UninitializeComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECREquipmentInstance* UECREquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<UECREquipmentInstance> InstanceType)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


TArray<UECREquipmentInstance*> UECREquipmentManagerComponent::GetEquipmentInstancesOfType(
	TSubclassOf<UECREquipmentInstance> InstanceType) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


TArray<UECREquipmentInstance_EquipmentMod*> UECREquipmentManagerComponent::GetEquipmentModifiersWithTags(
	FGameplayTagContainer ModifierTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECREquipmentInstance* UECREquipmentManagerComponent::GetFirstInstanceVisibleInChannel(const FName VisibilityChannel)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}

