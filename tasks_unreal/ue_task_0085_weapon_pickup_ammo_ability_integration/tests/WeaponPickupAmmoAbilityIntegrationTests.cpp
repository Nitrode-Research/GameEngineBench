#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterWeaponIntegrationTests
{
static bool LoadProjectFile(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutSource)
{
	const FString AbsolutePath = FPaths::ProjectDir() / RelativePath;
	if (!FFileHelper::LoadFileToString(OutSource, *AbsolutePath))
	{
		Test.AddError(FString::Printf(TEXT("Could not read %s"), RelativePath));
		return false;
	}
	return true;
}

static bool LoadCombined(FAutomationTestBase& Test, const TCHAR* const* RelativePaths, int32 Count, FString& OutCombined)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FString Source;
		if (!LoadProjectFile(Test, RelativePaths[Index], Source))
		{
			return false;
		}
		OutCombined += Source;
		OutCombined += TEXT("\n");
	}
	return true;
}

static bool RequireTokens(FAutomationTestBase& Test, const TCHAR* Label, const FString& Source, const TCHAR* const* Tokens, int32 Count)
{
	bool bAllPresent = true;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const TCHAR* Token = Tokens[Index];
		const bool bPresent = Source.Contains(Token);
		bAllPresent &= Test.TestTrue(FString::Printf(TEXT("%s contains '%s'"), Label, Token), bPresent);
	}
	return bAllPresent;
}
}

#define GS_REQUIRE_TOKENS(Test, Label, Source, Tokens) \
	GASShooterWeaponIntegrationTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponPickupInventoryStatePathsTest,
	"GASShooter.WeaponIntegration.pickup_inventory_state_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeaponPickupInventoryStatePathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"),
		TEXT("Source/GASShooter/Private/Items/Pickups/GSPickup.cpp"),
		TEXT("Source/GASShooter/Private/Weapons/GSWeapon.cpp")
	};
	if (!GASShooterWeaponIntegrationTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2/R7/R8): pickup/inventory transitions must reject invalid states and avoid duplicate ownership.
	const TCHAR* RequiredTokens[] = {
		TEXT("DoesWeaponExistInInventory(NewWeapon)"),
		TEXT("Inventory.Weapons.Add(NewWeapon)"),
		TEXT("NewWeapon->SetOwningCharacter(this)"),
		TEXT("Weapon->OnDropped(GetActorLocation()"),
		TEXT("RestrictedPickupTags"),
		TEXT("IsAlive()"),
		TEXT("SetOwner(nullptr)"),
		TEXT("CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter weapon pickup/inventory state"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponAbilityGrantAndTargetActorPathsTest,
	"GASShooter.WeaponIntegration.ability_grant_and_target_actor_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeaponAbilityGrantAndTargetActorPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"),
		TEXT("Source/GASShooter/Private/Weapons/GSWeapon.cpp")
	};
	if (!GASShooterWeaponIntegrationTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R4/R5/R9): equipped weapons grant/revoke server-side ability specs and lazily create reusable target actors.
	const TCHAR* RequiredTokens[] = {
		TEXT("NewWeapon->AddAbilities()"),
		TEXT("WeaponToRemove->RemoveAbilities()"),
		TEXT("AbilitySpecHandles.Add(ASC->GiveAbility"),
		TEXT("ASC->ClearAbility(SpecHandle)"),
		TEXT("for (FGameplayAbilitySpecHandle& SpecHandle : AbilitySpecHandles)"),
		TEXT("AbilitySpecHandles.Empty()"),
		TEXT("GetWorld()->SpawnActor<AGSGATA_LineTrace>()"),
		TEXT("GetWorld()->SpawnActor<AGSGATA_SphereTrace>()")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter weapon ability grants and target actors"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponAmmoReplicationAndMeshPresentationPathsTest,
	"GASShooter.WeaponIntegration.ammo_replication_and_mesh_presentation_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeaponAmmoReplicationAndMeshPresentationPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"),
		TEXT("Source/GASShooter/Private/Weapons/GSWeapon.cpp")
	};
	if (!GASShooterWeaponIntegrationTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R3/R6): clip ammo delegates/owner-only replication and 1P/3P attachments must drive HUD and presentation.
	const TCHAR* RequiredTokens[] = {
		TEXT("DOREPLIFETIME_CONDITION(AGSWeapon, PrimaryClipAmmo, COND_OwnerOnly)"),
		TEXT("OnPrimaryClipAmmoChanged.Broadcast(OldPrimaryClipAmmo, PrimaryClipAmmo)"),
		TEXT("OnPrimaryClipAmmoChanged.AddDynamic(this, &AGSHeroCharacter::CurrentWeaponPrimaryClipAmmoChanged)"),
		TEXT("PrimaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate"),
		TEXT("AttachToComponent(OwningCharacter->GetFirstPersonMesh()"),
		TEXT("AttachToComponent(OwningCharacter->GetThirdPersonMesh()"),
		TEXT("WeaponMesh1P->SetVisibility(true, true)"),
		TEXT("WeaponMesh3P->SetVisibility(false, true)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter weapon ammo and mesh presentation"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
