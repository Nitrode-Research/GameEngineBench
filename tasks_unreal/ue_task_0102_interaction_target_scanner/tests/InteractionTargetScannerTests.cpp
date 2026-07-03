#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRInteractionTargetScannerTests
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
	FInteractionTargetScannerSourceContractTokensTest,
	"ECR.InteractionScanner.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionTargetScannerSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECR/Private/Gameplay/Interaction/Abilities/ECRGameplayAbility_Interact.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Interaction/Abilities/GameplayAbilityTargetActor_Interact.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Interaction/InteractionStatics.cpp"),

	};
	if (!ECRInteractionTargetScannerTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetAbilitySystemComponentFromActorInfo"),
		TEXT("GetControllerFromActorInfo"),
		TEXT("IsLocalPlayerController"),
		TEXT("GetComponent"),
		TEXT("RemoveIndicator"),
		TEXT("IsNull"),
		TEXT("SetWantsNonDefaultHandling"),
		TEXT("SetCategory"),
		TEXT("SetDataObject"),
		TEXT("SetSceneComponent"),
		TEXT("GetRootComponent"),
		TEXT("SetIndicatorClass")

	};
	return ECRInteractionTargetScannerTests::RequireTokens(*this, TEXT("interaction_target_scanner"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
