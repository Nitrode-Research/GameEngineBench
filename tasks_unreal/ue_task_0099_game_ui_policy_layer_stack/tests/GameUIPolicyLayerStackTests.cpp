#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRGameUiPolicyLayerStackTests
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
	FGameUiPolicyLayerStackSourceContractTokensTest,
	"ECR.GameUIPolicy.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGameUiPolicyLayerStackSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Plugins/CommonGame/Source/Private/GameUIPolicy.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/GameUIManagerSubsystem.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/PrimaryGameLayout.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/CommonUIExtensions.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/Actions/AsyncAction_CreateWidgetAsync.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/Actions/AsyncAction_PushContentToLayerForPlayer.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/Actions/AsyncAction_ShowConfirmation.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/Messaging/CommonMessagingSubsystem.cpp"),
		TEXT("Plugins/CommonGame/Source/Private/Messaging/CommonGameDialog.cpp")

	};
	if (!ECRGameUiPolicyLayerStackTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetWorldFromContextObject"),
		TEXT("GetGameInstance"),
		TEXT("GetCurrentUIPolicy"),
		TEXT("GetOuter"),
		TEXT("FindByKey"),
		TEXT("AddWeakLambda"),
		TEXT("IsPrimaryPlayer"),
		TEXT("RemoveAll"),
		TEXT("IndexOfByKey"),
		TEXT("RemoveAt"),
		TEXT("GetName"),
		TEXT("GetNameSafe")

	};
	return ECRGameUiPolicyLayerStackTests::RequireTokens(*this, TEXT("game_ui_policy_layer_stack"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
