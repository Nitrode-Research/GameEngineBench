#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRInputMappingUserSettingsTests
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
	FInputMappingUserSettingsSourceContractTokensTest,
	"ECR.InputSettings.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInputMappingUserSettingsSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECR/Private/Input/ECRInputComponent.cpp"),
		TEXT("Source/ECR/Private/Input/ECRInputConfig.cpp"),
		TEXT("Source/ECR/Private/Input/ECRAimSensitivityData.cpp"),
		TEXT("Source/ECR/Private/Input/ECRInputModifiers.cpp"),
		TEXT("Source/ECR/Private/Input/ECRMappableConfigPair.cpp"),
		TEXT("Source/ECR/Private/Settings/ECRSettingsLocal.cpp"),
		TEXT("Source/ECR/Private/Settings/ECRSettingsShared.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/CommonPlayerInputKey.cpp")

	};
	if (!ECRInputMappingUserSettingsTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetCustomPlayerInputConfig"),
		TEXT("IsValid"),
		TEXT("GetUserSettings"),
		TEXT("MapPlayerKey"),
		TEXT("GetAllRegisteredInputConfigs"),
		TEXT("GetMappingContexts"),
		TEXT("RemoveMappingContext"),
		TEXT("UnregisterInputMappingContext"),
		TEXT("UnMapPlayerKey"),
		TEXT("RemoveBindingByHandle"),
		TEXT("ToString"),
		TEXT("GetNameSafe")

	};
	return ECRInputMappingUserSettingsTests::RequireTokens(*this, TEXT("input_mapping_user_settings"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
