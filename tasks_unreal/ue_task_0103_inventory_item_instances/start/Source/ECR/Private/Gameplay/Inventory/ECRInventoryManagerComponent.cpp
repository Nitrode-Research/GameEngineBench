// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryManagerComponent.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Inventory_Message_StackChanged, "ECR.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FECRInventoryEntry

FString FECRInventoryEntry::GetDebugString() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// FECRInventoryList

void FECRInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRInventoryList::BroadcastChangeMessage(FECRInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRInventoryItemInstance* FECRInventoryList::AddEntry(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void FECRInventoryList::AddEntry(UECRInventoryItemInstance* Instance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRInventoryList::RemoveEntry(UECRInventoryItemInstance* Instance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TArray<UECRInventoryItemInstance*> FECRInventoryList::GetAllItems() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


//////////////////////////////////////////////////////////////////////
// UECRInventoryManagerComponent

UECRInventoryManagerComponent::UECRInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UECRInventoryItemInstance* UECRInventoryManagerComponent::AddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRInventoryManagerComponent::AddItemInstance(UECRInventoryItemInstance* ItemInstance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRInventoryManagerComponent::RemoveItemInstance(UECRInventoryItemInstance* ItemInstance)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TArray<UECRInventoryItemInstance*> UECRInventoryManagerComponent::GetAllItems() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECRInventoryItemInstance* UECRInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


int32 UECRInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
//

// UCLASS(Abstract)
// class UECRInventoryFilter : public UObject
// {
// public:
// 	virtual bool PassesFilter(UECRInventoryItemInstance* Instance) const { return true; }
// };

// UCLASS()
// class UECRInventoryFilter_HasTag : public UECRInventoryFilter
// {
// public:
// 	virtual bool PassesFilter(UECRInventoryItemInstance* Instance) const { return true; }
// };

