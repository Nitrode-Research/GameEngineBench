#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRCustomizationAttachmentModulesTests
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
	FCustomizationAttachmentModulesSourceContractTokensTest,
	"ECR.CustomizationAttachments.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCustomizationAttachmentModulesSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECRCommon/Private/Customization/CustomizationAttachmentAsset.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationElementaryAsset.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationElementaryModule.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationLoaderAsset.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationLoaderComponent.cpp")

	};
	if (!ECRCustomizationAttachmentModulesTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("IsGarbageCollecting"),
		TEXT("GetDisplayNameEnd"),
		TEXT("FindRef"),
		TEXT("ApplyMaterialChanges"),
		TEXT("IsNone"),
		TEXT("Contains"),
		TEXT("SetLeaderPoseComponent"),
		TEXT("GetChildrenComponents"),
		TEXT("GetSkeletalMeshAsset"),
		TEXT("GetDisplayName"),
		TEXT("IsEmpty"),
		TEXT("ToString"),
		TEXT("MergeMeshes")

	};
	return ECRCustomizationAttachmentModulesTests::RequireTokens(*this, TEXT("customization_attachment_modules"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
