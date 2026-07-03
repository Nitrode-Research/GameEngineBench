// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECRQuickBarComponent.h"

#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "Net/UnrealNetwork.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "Gameplay/Inventory/InventoryFragment_EquippableItem.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_QuickBar_Message_ChannelChanged, "ECR.QuickBar.Message.ChannelChanged");


FECRQuickBarChannel::FECRQuickBarChannel()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void FECRQuickBar::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::BroadcastChangeMessage(FECRQuickBarChannel& Channel)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::UnequipItemInActiveSlot(FECRQuickBarChannel& Channel)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::EquipItemInActiveSlot(FECRQuickBarChannel& Channel)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRQuickBar::ClearChannels()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECREquipmentManagerComponent* FECRQuickBar::FindEquipmentManager() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


int32 FECRQuickBar::GetIndexOfChannelWithName(const FName Name) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 FECRQuickBar::GetIndexOfChannelWithNameOrCreate(const FName Name)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



UECRQuickBarComponent::UECRQuickBarComponent(const FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	ChannelData(this)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TArray<FName> UECRQuickBarComponent::GetChannels() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRQuickBarComponent::ClearChannels()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRQuickBarComponent::CycleActiveSlotForward(FName ChannelName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRQuickBarComponent::CycleActiveSlotBackward(FName ChannelName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TArray<UECRInventoryItemInstance*> UECRQuickBarComponent::GetSlots(FName ChannelName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


int32 UECRQuickBarComponent::GetActiveSlotIndex(FName ChannelName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UECRInventoryItemInstance* UECRQuickBarComponent::GetActiveSlotItem(FName ChannelName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


int32 UECRQuickBarComponent::GetNextFreeItemSlot(FName ChannelName, bool bReturnZeroIfChannelMissing) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UECRQuickBarComponent::GetItemAmountInChannel(FName ChannelName, bool bReturnZeroIfChannelMissing) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRQuickBarComponent::AddItemToSlot(int32 SlotIndex, UECRInventoryItemInstance* Item)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRInventoryItemInstance* UECRQuickBarComponent::RemoveItemFromSlot(FName ChannelName, int32 SlotIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}



void UECRQuickBarComponent::SetActiveSlotIndex_Implementation(FName ChannelName, int32 NewIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

