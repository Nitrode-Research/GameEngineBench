#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterHeroLifecycleTests
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
	GASShooterHeroLifecycleTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHeroAbilityLifecycleAndInputSetupTest,
	"GASShooter.HeroLifecycle.ability_lifecycle_and_input_setup_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHeroAbilityLifecycleAndInputSetupTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterHeroLifecycleTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2): possession and OnRep_PlayerState both initialize ASC actor info and input binding once state is ready.
	const TCHAR* RequiredTokens[] = {
		TEXT("PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this)"),
		TEXT("InitializeAttributes()"),
		TEXT("AddStartupEffects()"),
		TEXT("AddCharacterAbilities()"),
		TEXT("AbilitySystemComponent->BindAbilityActivationToInputComponent"),
		TEXT("bASCInputBound = true")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSHeroCharacter ability lifecycle"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHeroInventoryWeaponReplicationPathsTest,
	"GASShooter.HeroLifecycle.inventory_weapon_replication_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHeroInventoryWeaponReplicationPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterHeroLifecycleTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R3/R4/R5): inventory must avoid duplicates, grant/remove weapon abilities, and support predicted weapon switching.
	const TCHAR* RequiredTokens[] = {
		TEXT("DoesWeaponExistInInventory(NewWeapon)"),
		TEXT("Inventory.Weapons.Add(NewWeapon)"),
		TEXT("NewWeapon->SetOwningCharacter(this)"),
		TEXT("NewWeapon->AddAbilities()"),
		TEXT("WeaponToRemove->RemoveAbilities()"),
		TEXT("ServerEquipWeapon(NewWeapon)"),
		TEXT("ClientSyncCurrentWeapon(CurrentWeapon)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSHeroCharacter inventory and weapon paths"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHeroPerspectiveKnockdownRevivePathsTest,
	"GASShooter.HeroLifecycle.perspective_knockdown_revive_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHeroPerspectiveKnockdownRevivePathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterHeroLifecycleTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Heroes/GSHeroCharacter.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R6/R7/R8/R9): perspective, knockdown, revive, and death cleanup must remain integrated with gameplay state.
	const TCHAR* RequiredTokens[] = {
		TEXT("SetPerspective(bWasInFirstPersonPerspectiveWhenKnockedDown)"),
		// R6: switching perspective must re-apply that perspective's default field of view via the cached Default1PFOV member.
		TEXT("FirstPersonCamera->SetFieldOfView(Default1PFOV)"),
		TEXT("AbilitySystemComponent->CancelAllAbilities()"),
		TEXT("ExecuteGameplayCueLocal(FGameplayTag::RequestGameplayTag(\"GameplayCue.Hero.KnockedDown\")"),
		TEXT("ExecuteGameplayCueLocal(FGameplayTag::RequestGameplayTag(\"GameplayCue.Hero.Revived\")"),
		TEXT("Execute_InteractableCancelInteraction(this, GetThirdPersonMesh())"),
		TEXT("RemoveAllWeaponsFromInventory()"),
		TEXT("OnCharacterDied.Broadcast(this)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSHeroCharacter perspective and death paths"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
