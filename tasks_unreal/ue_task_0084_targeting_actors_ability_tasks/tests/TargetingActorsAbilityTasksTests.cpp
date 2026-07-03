#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterTargetingTaskTests
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
	GASShooterTargetingTaskTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargetTraceActorPathsTest,
	"GASShooter.TargetingTasks.trace_target_actor_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargetTraceActorPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/GSGATA_Trace.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/GSGATA_LineTrace.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/GSGATA_SphereTrace.cpp")
	};
	if (!GASShooterTargetingTaskTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2/R3/R4): trace actors must clean reticles, aim from camera, clip by range, and preserve trace/persistent-hit data.
	// The aiming-spread modifier must resolve the owning ASC through the project's typed accessor
	// (UGSAbilitySystemComponent::GetAbilitySystemComponentFromActor), not the raw actor-info pointer.
	const TCHAR* RequiredTokens[] = {
		TEXT("DestroyReticleActors()"),
		TEXT("ReticleActors.Empty()"),
		TEXT("TargetDataReadyDelegate.Clear()"),
		TEXT("ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd)"),
		TEXT("LineTraceWithFilter(HitResults"),
		TEXT("SphereTraceWithFilter(HitResults"),
		TEXT("PersistentHitResults.Add(HitResult)"),
		TEXT("GetAbilitySystemComponentFromActor(")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter target actor trace paths"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargetClientDataAndInteractionTasksTest,
	"GASShooter.TargetingTasks.client_target_data_and_interaction_tasks_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargetClientDataAndInteractionTasksTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_WaitTargetDataUsingActor.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_ServerWaitForClientTargetData.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_WaitInteractableTarget.cpp")
	};
	if (!GASShooterTargetingTaskTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R5/R6): interaction targeting and client target-data replication must use GAS delegates and consume data once.
	const TCHAR* RequiredTokens[] = {
		TEXT("CallServerSetReplicatedTargetData(GetAbilitySpecHandle()"),
		TEXT("ServerSetReplicatedTargetDataCancelled"),
		TEXT("AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).AddUObject"),
		TEXT("ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey())"),
		TEXT("AimWithPlayerController(SourceActor"),
		TEXT("World->LineTraceMultiByProfile")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter client target data and interaction tasks"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargetMontageAndLatentTaskCleanupPathsTest,
	"GASShooter.TargetingTasks.montage_and_latent_task_cleanup_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargetMontageAndLatentTaskCleanupPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_PlayMontageForMeshAndWaitForEvent.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_PlayMontageAndWaitForEvent.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_WaitChangeFOV.cpp"),
		TEXT("Source/GASShooter/Private/Characters/Abilities/AbilityTasks/GSAT_MoveSceneCompRelLocation.cpp")
	};
	if (!GASShooterTargetingTaskTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R7/R8/R9): montage and latent tasks must bind delegates, broadcast terminal events, stop cleanly, and tick interpolation.
	const TCHAR* RequiredTokens[] = {
		TEXT("AddGameplayEventTagContainerDelegate"),
		TEXT("PlayMontageForMesh(Ability, Mesh"),
		TEXT("OnCompleted.Broadcast"),
		TEXT("OnInterrupted.Broadcast"),
		TEXT("OnCancelled.Broadcast"),
		TEXT("RemoveGameplayEventTagContainerDelegate"),
		TEXT("CameraComponent->SetFieldOfView(NewFOV)"),
		TEXT("Component->SetRelativeLocation(NewLocation)")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter montage and latent ability tasks"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
