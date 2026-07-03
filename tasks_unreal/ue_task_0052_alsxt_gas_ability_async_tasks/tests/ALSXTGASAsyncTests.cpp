// Copyright 2026 GameDevBench. ALSXT GAS ability task and async-node source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ALSXTGASAsyncTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(Out, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTGASAsync_AbilityTasksSource,
	"TargetVector.ALSXTGASAsync.ability_tasks_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTGASAsync_AbilityTasksSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates ticking, enhanced input binding, targeting, and gameplay-tag ability tasks.
	FString TickSource, InputSource, TargetSource, TagSource;
	if (!TestTrue(TEXT("Tick task source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/AbilityTasks/AlsxtAbilityTaskOnTickEvent.cpp"), TickSource))
		|| !TestTrue(TEXT("Input task source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/AbilityTasks/AlsxtAbilityTaskWaitEnhancedInputEvent.cpp"), InputSource))
		|| !TestTrue(TEXT("Targeting task source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/AbilityTasks/AlsxtAbilityTaskPerformTargeting.cpp"), TargetSource))
		|| !TestTrue(TEXT("Tag task source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/AbilityTasks/AlsxtAbilityTaskWaitGameplayTagChanged.cpp"), TagSource)))
	{
		return true;
	}

	TestTrue(TEXT("Tick task must enable ticking and broadcast delta time"),
		TickSource.Contains(TEXT("bTickingTask = true")) && TickSource.Contains(TEXT("TickTask")) && TickSource.Contains(TEXT("TickEventReceived.Broadcast(DeltaTime)")));
	TestTrue(TEXT("Enhanced input task must store input config and bind to enhanced input component"),
		InputSource.Contains(TEXT("InputAction = InputAction")) && InputSource.Contains(TEXT("EventType = TriggerEventType"))
		&& InputSource.Contains(TEXT("UEnhancedInputComponent")) && InputSource.Contains(TEXT("BindAction")));
	TestTrue(TEXT("Enhanced input task must respect trigger once and clear bindings on destroy"),
		InputSource.Contains(TEXT("bTriggerOnce")) && InputSource.Contains(TEXT("bHasBeenTriggered")) && InputSource.Contains(TEXT("ClearBindingsForObject(this)")));
	TestTrue(TEXT("Targeting task must trace from avatar and emit single-hit target data"),
		TargetSource.Contains(TEXT("GetAvatarActor")) && TargetSource.Contains(TEXT("LineTraceSingleByChannel"))
		&& TargetSource.Contains(TEXT("FGameplayAbilityTargetData_SingleTargetHit")) && TargetSource.Contains(TEXT("OnTargetDataReady.Broadcast")));
	TestTrue(TEXT("Gameplay tag task must register count-change callbacks and broadcast deltas"),
		TagSource.Contains(TEXT("RegisterGameplayTagEvent")) && TagSource.Contains(TEXT("EGameplayTagEventType::AnyCountChange"))
		&& TagSource.Contains(TEXT("Added.Broadcast(NewCount, NewCount - PreviousCount)")) && TagSource.Contains(TEXT("Removed.Broadcast(NewCount, NewCount - PreviousCount)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTGASAsync_AsyncNodesSource,
	"TargetVector.ALSXTGASAsync.async_nodes_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTGASAsync_AsyncNodesSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates attribute/cooldown/effect-stack async node delegate registration, broadcasts, queries, and cleanup.
	FString AttributeSource, CooldownSource, StackSource;
	if (!TestTrue(TEXT("Attribute async source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/Nodes/AlsxtAsyncTaskAttributeChanged.cpp"), AttributeSource))
		|| !TestTrue(TEXT("Cooldown async source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/Nodes/AlsxtAsyncTaskCooldownChanged.cpp"), CooldownSource))
		|| !TestTrue(TEXT("Stack async source readable"), ALSXTGASAsyncTests::LoadSource(TEXT("Plugins/ALSXT/Source/ALSXT/Private/AbilitySystem/Nodes/AlsxtAsyncTaskEffectStackChanged.cpp"), StackSource)))
	{
		return true;
	}

	TestTrue(TEXT("Attribute async node must register and remove attribute value delegates"),
		AttributeSource.Contains(TEXT("GetGameplayAttributeValueChangeDelegate")) && AttributeSource.Contains(TEXT("AddUObject"))
		&& AttributeSource.Contains(TEXT("RemoveAll(this)")) && AttributeSource.Contains(TEXT("OnAttributeChanged.Broadcast")));
	TestTrue(TEXT("Cooldown async node must register active-effect and cooldown tag delegates"),
		CooldownSource.Contains(TEXT("OnActiveGameplayEffectAddedDelegateToSelf.AddUObject")) && CooldownSource.Contains(TEXT("RegisterGameplayTagEvent"))
		&& CooldownSource.Contains(TEXT("EGameplayTagEventType::NewOrRemoved")));
	TestTrue(TEXT("Cooldown async node must distinguish server and predicted cooldowns and broadcast begin/end"),
		CooldownSource.Contains(TEXT("UseServerCooldown")) && CooldownSource.Contains(TEXT("GetAbilityInstance_NotReplicated"))
		&& CooldownSource.Contains(TEXT("OnCooldownBegin.Broadcast")) && CooldownSource.Contains(TEXT("OnCooldownEnd.Broadcast")));
	TestTrue(TEXT("Cooldown remaining calculation must query active effects and choose longest remaining time"),
		CooldownSource.Contains(TEXT("FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags"))
		&& CooldownSource.Contains(TEXT("GetActiveEffectsTimeRemainingAndDuration")) && CooldownSource.Contains(TEXT("LongestTime")));
	TestTrue(TEXT("Effect stack async node must register add/remove/stack delegates and broadcast counts"),
		StackSource.Contains(TEXT("OnActiveGameplayEffectAddedDelegateToSelf.AddUObject"))
		&& StackSource.Contains(TEXT("OnAnyGameplayEffectRemovedDelegate().AddUObject"))
		&& StackSource.Contains(TEXT("OnGameplayEffectStackChangeDelegate"))
		&& StackSource.Contains(TEXT("OnGameplayEffectStackChange.Broadcast")));
	TestTrue(TEXT("Async nodes must mark themselves ready for destruction"),
		AttributeSource.Contains(TEXT("SetReadyToDestroy")) && CooldownSource.Contains(TEXT("SetReadyToDestroy")) && StackSource.Contains(TEXT("SetReadyToDestroy")));
	return true;
}
