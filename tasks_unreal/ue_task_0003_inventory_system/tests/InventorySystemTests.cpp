#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"

#define protected public
#include "Character/HordeBaseCharacter.h"
#include "Gameplay/GameplayStructures.h"
#include "Inventory/InventoryBaseItem.h"
#include "Inventory/InventoryComponent.h"
#undef protected

namespace InventoryTests
{
	static constexpr EAutomationTestFlags Flags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	static UWorld* CreateTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		if (World && GEngine)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
		}
		return World;
	}

	static void DestroyTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	static bool IsPropertyReplicated(UClass* InClass, const FName& PropertyName)
	{
		if (!InClass)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			if (FProperty* Property = *It; Property && Property->GetFName() == PropertyName)
			{
				return (Property->GetPropertyFlags() & CPF_Net) != 0;
			}
		}
		return false;
	}

	static bool HasFunctionFlag(UClass* InClass, const TCHAR* FunctionName, EFunctionFlags Flag)
	{
		if (!InClass)
		{
			return false;
		}

		if (UFunction* Function = InClass->FindFunctionByName(FName(FunctionName)))
		{
			return Function->HasAnyFunctionFlags(Flag);
		}
		return false;
	}

	static FItem MakeWeaponItem(const FName ItemID, const EActiveType Type, const FName AmmoType, const int32 LoadedAmmo)
	{
		FItem Item;
		Item.ItemID = ItemID;
		Item.Type = Type;
		Item.AmmoType = AmmoType;
		Item.DefaultLoadedAmmo = LoadedAmmo;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryReplicationSurfaceTest,
	"HordeTemplate.Inventory.replication_surface",
	InventoryTests::Flags)

bool FInventoryReplicationSurfaceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Active item index should replicate"), InventoryTests::IsPropertyReplicated(UInventoryComponent::StaticClass(), TEXT("ActiveItemIndex")));
	TestTrue(TEXT("Available ammo for current weapon should replicate"), InventoryTests::IsPropertyReplicated(UInventoryComponent::StaticClass(), TEXT("AvailableAmmoForCurrentWeapon")));

	TestTrue(TEXT("ServerAddItem should be a server RPC"), InventoryTests::HasFunctionFlag(UInventoryComponent::StaticClass(), TEXT("ServerAddItem"), FUNC_NetServer));
	TestTrue(TEXT("ServerDropItem should be a server RPC"), InventoryTests::HasFunctionFlag(UInventoryComponent::StaticClass(), TEXT("ServerDropItem"), FUNC_NetServer));
	TestTrue(TEXT("ServerDropItemAtIndex should be a server RPC"), InventoryTests::HasFunctionFlag(UInventoryComponent::StaticClass(), TEXT("ServerDropItemAtIndex"), FUNC_NetServer));
	TestTrue(TEXT("SwitchWeapon should be a server RPC"), InventoryTests::HasFunctionFlag(UInventoryComponent::StaticClass(), TEXT("SwitchWeapon"), FUNC_NetServer));
	TestTrue(TEXT("ScrollItems should be a server RPC"), InventoryTests::HasFunctionFlag(UInventoryComponent::StaticClass(), TEXT("ScrollItems"), FUNC_NetServer));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryAmmoAccountingTest,
	"HordeTemplate.Inventory.ammo_accounting",
	InventoryTests::Flags)

bool FInventoryAmmoAccountingTest::RunTest(const FString& Parameters)
{
	UWorld* World = InventoryTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	TestNotNull(TEXT("Character should own an inventory component"), Character ? Character->Inventory : nullptr);
	if (!Character || !Character->Inventory)
	{
		InventoryTests::DestroyTestWorld(World);
		return false;
	}

	UInventoryComponent* Inventory = Character->Inventory;
	Inventory->Inventory.Empty();
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Rifle"), EActiveType::EActiveRifle, TEXT("Ammo_556"), 12));
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Ammo_556"), EActiveType::EActiveAmmo, TEXT("Ammo_556"), 30));
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Ammo_556"), EActiveType::EActiveAmmo, TEXT("Ammo_556"), 20));
	Inventory->ActiveItemIndex = 0;

	int32 LastAmmoIndex = INDEX_NONE;
	TestEqual(TEXT("CountAmmo should total matching ammo stacks"), Inventory->CountAmmo(TEXT("Ammo_556"), LastAmmoIndex), 50);
	TestEqual(TEXT("CountAmmo should report the last matching ammo stack"), LastAmmoIndex, 2);

	TestTrue(TEXT("RemoveAmmoByType should succeed when enough total ammo exists"), Inventory->RemoveAmmoByType(TEXT("Ammo_556"), 35));
	TestEqual(TEXT("Available ammo should refresh after removing ammo"), Inventory->AvailableAmmoForCurrentWeapon, 15);

	LastAmmoIndex = INDEX_NONE;
	TestEqual(TEXT("Remaining matching ammo should be conserved across stacks"), Inventory->CountAmmo(TEXT("Ammo_556"), LastAmmoIndex), 15);
	TestFalse(TEXT("RemoveAmmoByType should fail when insufficient ammo exists"), Inventory->RemoveAmmoByType(TEXT("Ammo_556"), 99));

	InventoryTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventorySwitchGuardsTest,
	"HordeTemplate.Inventory.switch_guards",
	InventoryTests::Flags)

bool FInventorySwitchGuardsTest::RunTest(const FString& Parameters)
{
	UWorld* World = InventoryTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	TestNotNull(TEXT("Character should own an inventory component"), Character ? Character->Inventory : nullptr);
	if (!Character || !Character->Inventory)
	{
		InventoryTests::DestroyTestWorld(World);
		return false;
	}

	UInventoryComponent* Inventory = Character->Inventory;
	Inventory->Inventory.Empty();
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Rifle"), EActiveType::EActiveRifle, TEXT("Ammo_556"), 12));
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Pistol"), EActiveType::EActivePistol, TEXT("Ammo_9mm"), 8));
	Inventory->ActiveItemIndex = 0;

	Character->Reloading = true;
	Inventory->SwitchWeapon_Implementation(EActiveType::EActivePistol);
	TestEqual(TEXT("SwitchWeapon should be blocked while reloading"), Inventory->ActiveItemIndex, 0);

	Character->Reloading = false;
	Character->IsDead = true;
	Inventory->SwitchWeapon_Implementation(EActiveType::EActivePistol);
	TestEqual(TEXT("SwitchWeapon should be blocked while dead"), Inventory->ActiveItemIndex, 0);

	Character->IsDead = false;
	Inventory->SwitchWeapon_Implementation(EActiveType::EActivePistol);
	TestEqual(TEXT("SwitchWeapon should activate the first matching category when allowed"), Inventory->ActiveItemIndex, 1);

	InventoryTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryPickupStoredStateTest,
	"HordeTemplate.Inventory.pickup_preserves_stored_state",
	InventoryTests::Flags)

bool FInventoryPickupStoredStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = InventoryTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	AInventoryBaseItem* Pickup = World->SpawnActor<AInventoryBaseItem>();
	TestNotNull(TEXT("Character should spawn"), Character);
	TestNotNull(TEXT("Pickup actor should spawn"), Pickup);
	TestNotNull(TEXT("Character should own an inventory component"), Character ? Character->Inventory : nullptr);
	if (!Character || !Pickup || !Character->Inventory)
	{
		InventoryTests::DestroyTestWorld(World);
		return false;
	}

	Character->Inventory->Inventory.Empty();

	FItem StoredItem = InventoryTests::MakeWeaponItem(TEXT("Item_HM5"), EActiveType::EActiveRifle, TEXT("Ammo_556"), 17);
	Pickup->ItemID = StoredItem.ItemID;
	Pickup->ItemInfo = StoredItem;
	Pickup->Spawned = true;

	Pickup->Interact_Implementation(Character);

	int32 FoundIndex = INDEX_NONE;
	EActiveType FoundType = EActiveType::EActiveMisc;
	const bool bFound = Character->Inventory->InventoryItemExists(TEXT("Item_HM5"), FoundIndex, FoundType);
	TestTrue(TEXT("Picking up a spawned item should insert that item into inventory"), bFound);
	TestEqual(TEXT("Picked up item should preserve its stored loaded ammo"), Character->Inventory->Inventory.IsValidIndex(FoundIndex) ? Character->Inventory->Inventory[FoundIndex].DefaultLoadedAmmo : -1, 17);
	TestEqual(TEXT("Picked up item should preserve its category"), FoundType, EActiveType::EActiveRifle);

	InventoryTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryDropItemTest,
	"HordeTemplate.Inventory.drop_item_removes_from_inventory",
	InventoryTests::Flags)

bool FInventoryDropItemTest::RunTest(const FString& Parameters)
{
	UWorld* World = InventoryTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	TestNotNull(TEXT("Character should own an inventory component"), Character ? Character->Inventory : nullptr);
	if (!Character || !Character->Inventory)
	{
		InventoryTests::DestroyTestWorld(World);
		return false;
	}

	UInventoryComponent* Inventory = Character->Inventory;
	Inventory->Inventory.Empty();
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Rifle"), EActiveType::EActiveRifle, TEXT("Ammo_556"), 10));
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Pistol"), EActiveType::EActivePistol, TEXT("Ammo_9mm"), 8));
	Inventory->ActiveItemIndex = 0;

	// Dropping a valid item at index 1 must remove it from the array.
	const int32 CountBefore = Inventory->Inventory.Num();
	Inventory->ServerDropItemAtIndex_Implementation(1);
	TestEqual(TEXT("Dropping a valid item should remove it from the inventory"), Inventory->Inventory.Num(), CountBefore - 1);

	// The "Hands" sentinel may never be dropped — attempting to drop it must be blocked.
	Inventory->Inventory.Empty();
	Inventory->Inventory.Add(InventoryTests::MakeWeaponItem(TEXT("Item_Hands"), EActiveType::EActiveMisc, TEXT(""), 0));
	Inventory->ServerDropItemAtIndex_Implementation(0);
	TestEqual(TEXT("Dropping the Hands sentinel must be blocked and leave inventory unchanged"), Inventory->Inventory.Num(), 1);

	InventoryTests::DestroyTestWorld(World);
	return true;
}
