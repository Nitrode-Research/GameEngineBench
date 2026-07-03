#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ECRHealthDeathDamageFlowTests
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
	FHealthDeathDamageFlowSourceContractTokensTest,
	"ECR.HealthDamage.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthDeathDamageFlowSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECR/Private/Gameplay/GAS/Attributes/ECRHealthSet.cpp"),
		TEXT("Source/ECR/Private/Gameplay/GAS/Attributes/ECRCharacterHealthSet.cpp"),
		TEXT("Source/ECR/Private/Gameplay/GAS/Attributes/ECRVehicleHealthSet.cpp"),
		TEXT("Source/ECR/Public/Gameplay/GAS/Attributes/ECRSimpleVehicleHealthSet.cpp"),
		TEXT("Source/ECR/Private/Gameplay/GAS/Components/ECRHealthComponent.cpp"),
		TEXT("Source/ECR/Private/Gameplay/GAS/Components/ECRCharacterHealthComponent.cpp"),
		TEXT("Source/ECR/Private/Gameplay/GAS/Executions/ECRDamageExecution.cpp")

	};
	if (!ECRHealthDeathDamageFlowTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetDamageAttribute"),
		TEXT("GetDynamicAssetTags"),
		TEXT("HasTagExact"),
		TEXT("HasMatchingGameplayTag"),
		TEXT("GetEffectContext"),
		TEXT("GetEffectCauser"),
		TEXT("GetAggregatedTags"),
		TEXT("GetSourceObject"),
		TEXT("GetOwningActor"),
		TEXT("GetWorld"),
		TEXT("BroadcastMessage"),
		TEXT("IsBound"),
		TEXT("RemoveActiveEffectsWithGrantedTags"),
		TEXT("CalculateDamageAttenuationForArmorPenetration")

	};
	return ECRHealthDeathDamageFlowTests::RequireTokens(*this, TEXT("health_death_damage_flow"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}
