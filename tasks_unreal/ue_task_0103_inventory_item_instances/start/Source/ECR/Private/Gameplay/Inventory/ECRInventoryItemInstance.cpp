// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"

UECRInventoryItemInstance::UECRInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UECRInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRInventoryItemInstance::SetItemDef(TSubclassOf<UECRInventoryItemDefinition> InDef)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


const UECRInventoryItemFragment* UECRInventoryItemInstance::FindFragmentByClass(
	TSubclassOf<UECRInventoryItemFragment> FragmentClass) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}

