#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRRangedWeaponFireControlTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRangedWeaponFireControlSourceContractTokensTest,
	"ECR.RangedWeapon.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRangedWeaponFireControlSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECR/Private/Gameplay/Weapons/ECRGameplayAbility_RangedWeapon.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Weapons/ECRRangedWeaponInstance.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Weapons/ECRWeaponInstance.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Weapons/ECRWeaponStateComponent.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Weapons/ECRDamageLogDebuggerComponent.cpp"),
		TEXT("Source/ECR/Private/GUI/Weapons/ECRWeaponUserInterface.cpp"),
		TEXT("Source/ECR/Private/GUI/Weapons/HitMarkerConfirmationWidget.cpp")

	};
	if (!ECRRangedWeaponFireControlTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("AddTag"),
		TEXT("GetAssociatedEquipment"),
		TEXT("FindAbilitySpecFromHandle"),
		TEXT("weapon"),
		TEXT("GetPathName"),
		TEXT("GetPathNameSafe"),
		TEXT("StaticClass"),
		TEXT("GetName"),
		TEXT("DoesRepresentClass"),
		TEXT("FetchActor"),
		TEXT("GetAttachParentActor"),
		TEXT("GetAvatarActorFromActorInfo")

	};
	return ECRRangedWeaponFireControlTests::RequireTokens(*this, TEXT("ranged_weapon_fire_control"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
