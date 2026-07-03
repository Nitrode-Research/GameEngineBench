#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRCustomizationMaterialNamespacesTests
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
	FCustomizationMaterialNamespacesSourceContractTokensTest,
	"ECR.CustomizationMaterials.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCustomizationMaterialNamespacesSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECRCommon/Private/Customization/CustomizationMaterialAsset.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationMaterialLoaderComponent.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationMaterialNameSpace.cpp"),
		TEXT("Source/ECRCommon/Private/Customization/CustomizationSavingNameSpace.cpp")

	};
	if (!ECRCustomizationMaterialNamespacesTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("IsEmpty"),
		TEXT("GetNameSafe"),
		TEXT("GetOwner"),
		TEXT("GetChildrenComponents"),
		TEXT("GetAttachmentMaterialCustomizationNamespace"),
		TEXT("IsNone"),
		TEXT("GetDisplayNameEnd"),
		TEXT("IsGarbageCollecting"),
		TEXT("GetCustomizationNamespaceOverride"),
		TEXT("GetSlotNamesNamespacesOverride"),
		TEXT("GetMaterialSlotNames"),
		TEXT("GetParameterValue")

	};
	return ECRCustomizationMaterialNamespacesTests::RequireTokens(*this, TEXT("customization_material_namespaces"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
