// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/IPickupable.h"
#include "DrawDebugHelpers.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "UObject/ScriptInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Gameplay/Inventory/ECRInventoryManagerComponent.h"

UPickupableStatics::UPickupableStatics()
	: Super(FObjectInitializer::Get())
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TScriptInterface<IPickupable> UPickupableStatics::GetIPickupableFromActorInfo(UGameplayAbility* Ability)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UPickupableStatics::AddPickupInventory(UECRInventoryManagerComponent* InventoryComponent, TScriptInterface<IPickupable> Pickupable)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}
