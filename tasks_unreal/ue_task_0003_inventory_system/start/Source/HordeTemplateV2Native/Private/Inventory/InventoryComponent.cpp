// InventoryComponent.cpp

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, ActiveItemIndex);
	DOREPLIFETIME(UInventoryComponent, AvailableAmmoForCurrentWeapon);
}

const TArray<FItem>& UInventoryComponent::GetInventory()
{
	return Inventory;
}

void UInventoryComponent::UpdateCurrentItemAmmo(int32 Ammo) {}

void UInventoryComponent::RefreshCurrentAmmoForItem() {}

void UInventoryComponent::ServerAddItem_Implementation(const FString& ItemID, bool Custom, FItem CustomItem) {}
bool UInventoryComponent::ServerAddItem_Validate(const FString& ItemID, bool Custom, FItem CustomItem) { return true; }

void UInventoryComponent::ServerDropItem_Implementation(ABaseFirearm* Firearm) {}
bool UInventoryComponent::ServerDropItem_Validate(ABaseFirearm* Firearm) { return true; }

void UInventoryComponent::ServerDropItemAtIndex_Implementation(int32 IndexToDrop) {}
bool UInventoryComponent::ServerDropItemAtIndex_Validate(int32 IndexToDrop) { return true; }

void UInventoryComponent::SwitchWeapon_Implementation(EActiveType ItemType) {}
bool UInventoryComponent::SwitchWeapon_Validate(EActiveType ItemType) { return true; }

void UInventoryComponent::ScrollItems_Implementation(bool ScrollUp) {}
bool UInventoryComponent::ScrollItems_Validate(bool ScrollUp) { return true; }

bool UInventoryComponent::InventoryItemExists(FString ItemID, int32& Index, EActiveType& ItemType) { return false; }

FTransform UInventoryComponent::CalculateDropLocation() { return FTransform{}; }

int32 UInventoryComponent::CountAmmo(FName AmmoType, int32& Index) { Index = -1; return 0; }

bool UInventoryComponent::RemoveAmmoByType(FName AmmoType, int32 AmountToRemove) { return false; }

void UInventoryComponent::FindItemByCategory(EActiveType IType, FItem& OutItem, int32& OutIndex) { OutIndex = -1; }

bool UInventoryComponent::GetItemByID(FString ItemID, FItem& Item) { return false; }

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInventoryComponent::OnRep_ActiveItemIndex() {}

void UInventoryComponent::OnRep_AvailableAmmo() {}

void UInventoryComponent::AddItem_Internal(const FItem& ItemToAdd, bool bSetActiveIfWeapon) {}
