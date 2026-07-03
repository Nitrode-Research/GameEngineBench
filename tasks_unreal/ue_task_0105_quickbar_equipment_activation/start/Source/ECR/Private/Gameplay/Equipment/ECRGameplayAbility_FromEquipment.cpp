// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECRGameplayAbility_FromEquipment.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Misc/DataValidation.h"

UECRGameplayAbility_FromEquipment::UECRGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECREquipmentInstance* UECRGameplayAbility_FromEquipment::GetAssociatedEquipment(UObject* SourceObject) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECRInventoryItemInstance* UECRGameplayAbility_FromEquipment::GetAssociatedItem(UObject* SourceObject) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}



#if WITH_EDITOR
EDataValidationResult UECRGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#endif
