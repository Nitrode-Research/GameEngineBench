#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterAbilitySystemTests
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
	GASShooterAbilitySystemTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityInputActivationAndConfirmCancelPathsTest,
	"GASShooter.AbilitySystem.input_activation_and_confirm_cancel_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAbilityInputActivationAndConfirmCancelPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterAbilitySystemTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Abilities/GSAbilitySystemComponent.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2/R6): input must consume generic confirm/cancel, then route configured input-activated abilities through GAS.
	const TCHAR* RequiredTokens[] = {
		TEXT("IsGenericConfirmInputBound(InputID)"),
		TEXT("LocalInputConfirm()"),
		TEXT("IsGenericCancelInputBound(InputID)"),
		TEXT("LocalInputCancel()"),
		TEXT("GA->bActivateOnInput"),
		TEXT("TryActivateAbility(Spec.Handle)"),
		TEXT("return GetTagCount(TagToCheck)"),
		TEXT("OptionalSourceObject")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSAbilitySystemComponent input paths"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityPredictionBatchingAndLocalCuesTest,
	"GASShooter.AbilitySystem.prediction_batching_and_local_cues_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAbilityPredictionBatchingAndLocalCuesTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterAbilitySystemTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Abilities/GSAbilitySystemComponent.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R3/R4/R5): weapon activation batching, local cues, and predictive GE application must use existing GAS paths.
	const TCHAR* RequiredTokens[] = {
		TEXT("FScopedServerAbilityRPCBatcher"),
		TEXT("ExternalEndAbility()"),
		TEXT("HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Executed"),
		TEXT("HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Removed"),
		TEXT("ScopedPredictionKey"),
		TEXT("ApplyGameplayEffectToSelf(GameplayEffect, Level, EffectContext, ScopedPredictionKey)"),
		TEXT("ApplyGameplayEffectToTarget(GameplayEffect, Target, Level, Context, ScopedPredictionKey)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSAbilitySystemComponent prediction and cues"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityMultiMeshMontageReplicationStateTest,
	"GASShooter.AbilitySystem.multi_mesh_montage_replication_state_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAbilityMultiMeshMontageReplicationStateTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterAbilitySystemTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Characters/Abilities/GSAbilitySystemComponent.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R7/R8/R9): multi-mesh montage replication must tick, correct, defer, and clean up per mesh/ability.
	const TCHAR* RequiredTokens[] = {
		TEXT("CVarReplayMontageErrorThreshold"),
		TEXT("AnimMontage_UpdateReplicatedDataForMesh(MontageInfo.Mesh)"),
		TEXT("ClearAnimatingAbilityForAllMeshes(Ability)"),
		TEXT("Montage_SetPosition"),
		TEXT("bPendingMontageRep = true"),
		TEXT("ServerCurrentMontageSetNextSectionNameForMesh(")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSAbilitySystemComponent montage replication"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
