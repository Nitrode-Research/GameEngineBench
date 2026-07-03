#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterAttributesDamageTests
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
	GASShooterAttributesDamageTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAttributeClampAndDamageHooksTest,
	"GASShooter.AttributesDamage.attribute_clamp_and_damage_hooks_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAttributeClampAndDamageHooksTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/AttributeSets/GSAttributeSetBase.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/GSDamageExecutionCalc.cpp")
	};
	if (!GASShooterAttributesDamageTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2/R3/R4/R9): attributes clamp through GAS hooks, damage consumes shield before health, and context is preserved.
	const TCHAR* RequiredTokens[] = {
		TEXT("AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute())"),
		TEXT("NewValue = FMath::Clamp<float>(NewValue, 150, 1000)"),
		TEXT("GetOriginalInstigatorAbilitySystemComponent()"),
		TEXT("Context.GetEffectCauser()"),
		TEXT("const float LocalDamageDone = GetDamage()"),
		TEXT("float DamageAfterShield = LocalDamageDone - OldShield"),
		TEXT("SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()))"),
		TEXT("!FMath::IsNearlyEqual(CurrentHealth, ClampedHealth)"),
		TEXT("PC->ShowDamageNumber(LocalDamageDone, TargetCharacter, DamageNumberTags)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter attribute and damage hooks"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAttributeAsyncListenerCleanupPathsTest,
	"GASShooter.AttributesDamage.async_listener_cleanup_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAttributeAsyncListenerCleanupPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/AsyncTaskAttributeChanged.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AsyncTaskGameplayTagAddedRemoved.cpp")
	};
	if (!GASShooterAttributesDamageTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R6/R7): async listeners must bind real delegates, broadcast useful payloads, and unregister on EndTask.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject"),
		TEXT("OnAttributeChanged.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue)"),
		TEXT("ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this)"),
		TEXT("RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject"),
		TEXT("OnTagAdded.Broadcast(Tag)"),
		TEXT("OnTagRemoved.Broadcast(Tag)"),
		TEXT("RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter async attribute/tag listeners"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmmoAndReplicationNotificationPathsTest,
	"GASShooter.AttributesDamage.ammo_and_replication_notification_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAmmoAndReplicationNotificationPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/AttributeSets/GSAttributeSetBase.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AttributeSets/GSAmmoAttributeSet.cpp")
	};
	if (!GASShooterAttributesDamageTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R5/R8): replicated attributes and ammo reserves must use GAS replication notification paths.
	const TCHAR* RequiredTokens[] = {
		TEXT("GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSetBase, Health, OldHealth)"),
		TEXT("GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSetBase, Shield, OldShield)"),
		TEXT("GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSetBase, XP, OldXP)"),
		TEXT("GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, RifleReserveAmmo, OldRifleReserveAmmo)"),
		TEXT("GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, MaxRifleReserveAmmo, OldMaxRifleReserveAmmo)"),
		TEXT("UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter attribute replication and ammo paths"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
