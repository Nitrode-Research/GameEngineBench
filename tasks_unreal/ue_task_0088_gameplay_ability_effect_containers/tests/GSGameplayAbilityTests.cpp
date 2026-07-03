// Copyright 2026 GameDevBench. GASShooter gameplay ability source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGSGameplayAbility_SourceContract,
	"GASShooter.GameplayAbility.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGSGameplayAbility_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates GASShooter's effect container, activation, prediction, HUD, cost, and multi-mesh montage ability logic.
	const FString SourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("Source/GASShooter/Private/Characters/Abilities/GSGameplayAbility.cpp"));

	FString Source;
	if (!TestTrue(TEXT("GSGameplayAbility source readable"), FFileHelper::LoadFileToString(Source, *SourcePath)))
	{
		return true;
	}

	TestTrue(TEXT("Ability constructor must configure GASShooter defaults and tags"),
		Source.Contains(TEXT("InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor"))
		&& Source.Contains(TEXT("bActivateOnInput = true")) && Source.Contains(TEXT("bCannotActivateWhileInteracting = true"))
		&& Source.Contains(TEXT("ActivationBlockedTags.AddTag")) && Source.Contains(TEXT("State.Dead"))
		&& Source.Contains(TEXT("State.KnockedDown")) && Source.Contains(TEXT("Ability.BlocksInteraction"))
		&& Source.Contains(TEXT("State.Interacting")) && Source.Contains(TEXT("State.InteractingRemoval")));
	TestTrue(TEXT("OnAvatarSet must activate abilities that request activate-on-grant"),
		Source.Contains(TEXT("bActivateAbilityOnGranted")) && Source.Contains(TEXT("TryActivateAbility(Spec.Handle")));
	TestTrue(TEXT("Target data helpers must build actor-array and hit-result target data handles"),
		Source.Contains(TEXT("FGameplayAbilityTargetData_ActorArray")) && Source.Contains(TEXT("TargetActorArray.Append"))
		&& Source.Contains(TEXT("FGameplayAbilityTargetData_SingleTargetHit")) && Source.Contains(TEXT("TargetData.Add(NewData)")));
	TestTrue(TEXT("Effect containers must run target types, add targets, choose levels, and build specs"),
		Source.Contains(TEXT("Container.TargetType.Get")) && Source.Contains(TEXT("GetDefaultObject"))
		&& Source.Contains(TEXT("TargetTypeCDO->GetTargets")) && Source.Contains(TEXT("ReturnSpec.AddTargets"))
		&& Source.Contains(TEXT("OverrideGameplayLevel == INDEX_NONE")) && Source.Contains(TEXT("GetAbilityLevel()"))
		&& Source.Contains(TEXT("MakeOutgoingGameplayEffectSpec")));
	TestTrue(TEXT("Effect container map lookup and application must use target data"),
		Source.Contains(TEXT("EffectContainerMap.Find")) && Source.Contains(TEXT("MakeEffectContainerSpecFromContainer"))
		&& Source.Contains(TEXT("K2_ApplyGameplayEffectSpecToTarget")) && Source.Contains(TEXT("ContainerSpec.TargetData")));
	TestTrue(TEXT("Activation gates must check interaction counts and current weapon source object"),
		Source.Contains(TEXT("GetTagCount(InteractingTag) > ASC->GetTagCount(InteractingRemovalTag)"))
		&& Source.Contains(TEXT("bSourceObjectMustEqualCurrentWeaponToActivate")) && Source.Contains(TEXT("GetCurrentWeapon"))
		&& Source.Contains(TEXT("GetSourceObject(Handle, ActorInfo)")));
	TestTrue(TEXT("Cost hooks must extend default GAS cost behavior"),
		Source.Contains(TEXT("Super::CheckCost")) && Source.Contains(TEXT("GSCheckCost(Handle, *ActorInfo)"))
		&& Source.Contains(TEXT("GSApplyCost(Handle, *ActorInfo, ActivationInfo)")) && Source.Contains(TEXT("Super::ApplyCost")));
	TestTrue(TEXT("Prediction helpers must batch activation and send target data through scoped prediction"),
		Source.Contains(TEXT("BatchRPCTryActivateAbility")) && Source.Contains(TEXT("ScopedPredictionKey.ToString"))
		&& Source.Contains(TEXT("IsValidForMorePrediction")) && Source.Contains(TEXT("FScopedPredictionWindow"))
		&& Source.Contains(TEXT("CallServerSetReplicatedTargetData")));
	TestTrue(TEXT("HUD reticle helpers must set explicit and weapon-default reticles"),
		Source.Contains(TEXT("SetHUDReticle(ReticleClass)")) && Source.Contains(TEXT("GetPrimaryHUDReticleClass")) && Source.Contains(TEXT("SetHUDReticle(nullptr)")));
	TestTrue(TEXT("Multi-mesh montage helpers must track mesh montages and route commands through GS ASC"),
		Source.Contains(TEXT("CurrentAbilityMeshMontages.Add")) && Source.Contains(TEXT("FindAbillityMeshMontage"))
		&& Source.Contains(TEXT("IsAnimatingAbilityForAnyMesh(this)")) && Source.Contains(TEXT("CurrentMontageJumpToSectionForMesh"))
		&& Source.Contains(TEXT("CurrentMontageSetNextSectionNameForMesh")) && Source.Contains(TEXT("CurrentMontageStopForMesh"))
		&& Source.Contains(TEXT("StopAllCurrentMontages")));

	return true;
}
