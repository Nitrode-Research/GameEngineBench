// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"

//////////////////////////////////////////////////////////////////////
// UECRInventoryItemDefinition

UECRInventoryItemDefinition::UECRInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


const UECRInventoryItemFragment* UECRInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<UECRInventoryItemFragment> FragmentClass) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


//////////////////////////////////////////////////////////////////////
// UECRInventoryItemDefinition

const UECRInventoryItemFragment* UECRInventoryFunctionLibrary::FindItemDefinitionFragment(
	TSubclassOf<UECRInventoryItemDefinition> ItemDef, TSubclassOf<UECRInventoryItemFragment> FragmentClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}

