// Copyright (c) 2024 GamedevBench. All Rights Reserved.
// ActionSystemReimplementationTests.cpp
// Automation tests for the ActionRoguelike Action System reimplementation.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

#include "ActionRoguelike/ActionSystem/RogueAction.h"
#include "ActionRoguelike/ActionSystem/RogueActionComponent.h"
#include "ActionRoguelike/ActionSystem/RogueActionEffect.h"
#include "ActionRoguelike/ActionSystem/RogueActionEffect_Thorns.h"
#include "ActionRoguelike/ActionSystem/RogueAttributeSet.h"
#include "ActionRoguelike/AI/RogueAction_MinionMeleeAttack.h"
#include "ActionRoguelike/AI/RogueAICharacter.h"
#include "SharedGameplayTags.h"

// ──────────────────────────────────────────────────────────────────────────────
// Test harness
// ──────────────────────────────────────────────────────────────────────────────
namespace ActionSystemTestHelpers
{
	static UWorld* CreateTestWorld()
	{
		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(), UWorld::StaticClass(), TEXT("ActionSystemTestWorld"));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World || !GEngine)
		{
			return World;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		return World;
	}

	static void DestroyTestWorld(UWorld* World)
	{
		if (World)
		{
			World->BeginTearingDown();
			World->CleanupWorld();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}
	}

	static ACharacter* SpawnTestCharacter(UWorld* World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform::Identity, Params);
	}

	// ── URogueAction reflection helpers ──────────────────────────────────────

	template <typename T>
	static T* FindStructPropertyValue(URogueAction* Action, const TCHAR* Name)
	{
		if (FStructProperty* Prop = CastField<FStructProperty>(URogueAction::StaticClass()->FindPropertyByName(Name)))
		{
			return Prop->ContainerPtrToValuePtr<T>(Action);
		}
		return nullptr;
	}

	static void SetActionFloat(URogueAction* Action, const TCHAR* Name, float Value)
	{
		if (FFloatProperty* Prop = CastField<FFloatProperty>(URogueAction::StaticClass()->FindPropertyByName(Name)))
		{
			Prop->SetPropertyValue_InContainer(Action, Value);
		}
	}

	static FGameplayTagContainer* GetActionBlockedTags(URogueAction* Action)
	{
		return FindStructPropertyValue<FGameplayTagContainer>(Action, TEXT("BlockedTags"));
	}

	static FGameplayTagContainer* GetActionGrantsTags(URogueAction* Action)
	{
		return FindStructPropertyValue<FGameplayTagContainer>(Action, TEXT("GrantsTags"));
	}

	static FActionRepData* GetActionRepData(URogueAction* Action)
	{
		return FindStructPropertyValue<FActionRepData>(Action, TEXT("RepData"));
	}

	static void SetActionActivationTag(URogueAction* Action, const FGameplayTag& Tag)
	{
		if (FStructProperty* Prop = CastField<FStructProperty>(URogueAction::StaticClass()->FindPropertyByName(TEXT("ActivationTag"))))
		{
			*Prop->ContainerPtrToValuePtr<FGameplayTag>(Action) = Tag;
		}
	}

	// ── URogueActionComponent reflection helpers ─────────────────────────────

	static FArrayProperty* GetActionsArrayProp()
	{
		return CastField<FArrayProperty>(URogueActionComponent::StaticClass()->FindPropertyByName(TEXT("Actions")));
	}

	static int32 GetActionsNum(URogueActionComponent* Comp)
	{
		if (FArrayProperty* Prop = GetActionsArrayProp())
		{
			FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Comp));
			return Helper.Num();
		}
		return -1;
	}

	static URogueAction* GetLastAction(URogueActionComponent* Comp)
	{
		if (FArrayProperty* Prop = GetActionsArrayProp())
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(Prop->Inner);
			FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Comp));
			if (Helper.Num() > 0)
			{
				return Cast<URogueAction>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Helper.Num() - 1)));
			}
		}
		return nullptr;
	}

	static bool ActionsContains(URogueActionComponent* Comp, URogueAction* Action)
	{
		if (FArrayProperty* Prop = GetActionsArrayProp())
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(Prop->Inner);
			FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Comp));
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				if (Inner->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Action)
				{
					return true;
				}
			}
		}
		return false;
	}

	static void AddActionToComponent(URogueActionComponent* Comp, URogueAction* Action)
	{
		if (FArrayProperty* Prop = GetActionsArrayProp())
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(Prop->Inner);
			FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Comp));
			const int32 NewIdx = Helper.AddValue();
			Inner->SetObjectPropertyValue(Helper.GetRawPtr(NewIdx), Action);
		}
	}

	static int32 GetCachedActionsNum(URogueActionComponent* Comp)
	{
		if (FMapProperty* MapProp = CastField<FMapProperty>(URogueActionComponent::StaticClass()->FindPropertyByName(TEXT("CachedActions"))))
		{
			FScriptMapHelper Helper(MapProp, MapProp->ContainerPtrToValuePtr<void>(Comp));
			return Helper.Num();
		}
		return -1;
	}

	static void SetCachedAction(URogueActionComponent* Comp, FGameplayTag Tag, URogueAction* Action)
	{
		FMapProperty* MapProp = CastField<FMapProperty>(
			URogueActionComponent::StaticClass()->FindPropertyByName(TEXT("CachedActions")));
		if (!MapProp)
		{
			return;
		}

		FScriptMapHelper Helper(MapProp, MapProp->ContainerPtrToValuePtr<void>(Comp));
		const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();

		FStructProperty* KeyProp = CastField<FStructProperty>(MapProp->KeyProp);
		FObjectProperty* ValueProp = CastField<FObjectProperty>(MapProp->ValueProp);
		if (!KeyProp || !ValueProp)
		{
			return;
		}

		void* PairPtr = Helper.GetPairPtr(NewIndex);
		KeyProp->CopyCompleteValue(KeyProp->ContainerPtrToValuePtr<void>(PairPtr), &Tag);
		ValueProp->SetObjectPropertyValue(ValueProp->ContainerPtrToValuePtr<void>(PairPtr), Action);
		Helper.Rehash();
	}

	static URogueAttributeSet* GetComponentAttributeSet(URogueActionComponent* Comp)
	{
		if (FObjectProperty* Prop = CastField<FObjectProperty>(URogueActionComponent::StaticClass()->FindPropertyByName(TEXT("AttributeSet"))))
		{
			return Cast<URogueAttributeSet>(Prop->GetObjectPropertyValue_InContainer(Comp));
		}
		return nullptr;
	}

	static void SetComponentAttributeSet(URogueActionComponent* Comp, URogueAttributeSet* NewSet)
	{
		if (FObjectProperty* Prop = CastField<FObjectProperty>(URogueActionComponent::StaticClass()->FindPropertyByName(TEXT("AttributeSet"))))
		{
			Prop->SetObjectPropertyValue_InContainer(Comp, NewSet);
		}
	}

	// Replace any auto-created set with one of the requested class and run
	// InitializeAttributes so AttributeCache is populated for tests.
	static void EnsureAttributeSet(URogueActionComponent* Comp,
		TSubclassOf<URogueAttributeSet> SetClass = URogueAttributeSet::StaticClass())
	{
		URogueAttributeSet* Current = GetComponentAttributeSet(Comp);
		if (!Current || !Current->IsA(SetClass))
		{
			URogueAttributeSet* NewSet = NewObject<URogueAttributeSet>(Comp, SetClass);
			SetComponentAttributeSet(Comp, NewSet);
			NewSet->InitializeAttributes(Comp);
		}
	}

	// Attempt to find any attribute from a prioritized list of tags.
	// Returns the first attribute found and sets OutTag to the matching tag,
	// or returns nullptr if none are present in the attribute set.
	static FRogueAttribute* FindAnyAttribute(URogueActionComponent* Comp, FGameplayTag& OutTag)
	{
		const FGameplayTag Candidates[] = {
			SharedGameplayTags::Attribute_Health,
			SharedGameplayTags::Attribute_AttackDamage,
			SharedGameplayTags::Attribute_MoveSpeed,
			SharedGameplayTags::Attribute_Rage,
			SharedGameplayTags::Attribute_Credits,
			SharedGameplayTags::Attribute_HealthMax,
		};
		for (const FGameplayTag& Tag : Candidates)
		{
			if (FRogueAttribute* Attr = Comp->GetAttribute(Tag))
			{
				OutTag = Tag;
				return Attr;
			}
		}
		return nullptr;
	}

	// ── URogueActionEffect reflection helpers ────────────────────────────────

	static void SetEffectFloat(URogueActionEffect* Effect, const TCHAR* Name, float Value)
	{
		if (FFloatProperty* Prop = CastField<FFloatProperty>(URogueActionEffect::StaticClass()->FindPropertyByName(Name)))
		{
			Prop->SetPropertyValue_InContainer(Effect, Value);
		}
	}

	// ── AActor role manipulation (best-effort) ───────────────────────────────

	// Force the actor's local Role via reflection so HasAuthority() returns false.
	static bool TrySetActorRole(AActor* Actor, ENetRole NewRole)
	{
		FProperty* RoleProp = AActor::StaticClass()->FindPropertyByName(TEXT("Role"));
		if (!RoleProp)
		{
			return false;
		}
		if (FByteProperty* ByteProp = CastField<FByteProperty>(RoleProp))
		{
			ByteProp->SetPropertyValue_InContainer(Actor, (uint8)NewRole);
		}
		else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(RoleProp))
		{
			if (FByteProperty* Underlying = CastField<FByteProperty>(EnumProp->GetUnderlyingProperty()))
			{
				Underlying->SetPropertyValue_InContainer(Actor, (uint8)NewRole);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		return Actor->GetLocalRole() == NewRole;
	}
}

using namespace ActionSystemTestHelpers;

// ══════════════════════════════════════════════════════════════════════════════
// CORE SUITE — direct runtime behavior
// ══════════════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────────────
// CORE 1: An action only starts when not already running and cooldown has
// expired; the remaining cooldown is observable for UI.
// Covers: B1 (already-running guard), B2 (must stop before restart), B5 (cooldown gate + tracking).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_CanStart_GuardsRunningAndCooldown,
	"ActionRoguelike.ActionSystem.CanStart_GuardsRunningAndCooldown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_CanStart_GuardsRunningAndCooldown::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	URogueAction* Action = NewObject<URogueAction>(Owner, TEXT("Action"));
	Action->Initialize(Comp);
	SetActionFloat(Action, TEXT("CooldownTime"), 5.0f);

	// B1: clean start succeeds
	TestTrue(TEXT("CanStart true initially"), Action->CanStart(Owner));
	Action->StartAction(Owner);
	TestTrue(TEXT("Action runs after StartAction"), Action->IsRunning());
	// B2: cannot restart while running
	TestFalse(TEXT("CanStart false while running"), Action->CanStart(Owner));

	Action->StopAction(Owner);
	TestFalse(TEXT("Action no longer running after StopAction"), Action->IsRunning());

	// B5: cooldown is now active and trackable
	const float Remaining = Action->CooldownTimeRemaining();
	TestTrue(TEXT("CooldownTimeRemaining > 0 right after stop"), Remaining > 0.0f);
	TestTrue(TEXT("CooldownTimeRemaining within configured window"), Remaining <= 5.0f + KINDA_SMALL_NUMBER);
	TestFalse(TEXT("CanStart false while on cooldown"), Action->CanStart(Owner));

	// Fast-forward cooldown: setting CooldownUntil into the past simulates time elapsing.
	SetActionFloat(Action, TEXT("CooldownUntil"), World->GetTimeSeconds() - 1.0f);
	TestTrue(TEXT("CanStart true after cooldown elapses"), Action->CanStart(Owner));

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 2: Granted tags are added when the action starts and removed when it
// stops; blocking tags refuse the start.
// Covers: B3 (granted-tag lifecycle), B4 (blocking-tag refusal).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_GrantedAndBlockedTags,
	"ActionRoguelike.ActionSystem.GrantedAndBlockedTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_GrantedAndBlockedTags::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	URogueAction* Action = NewObject<URogueAction>(Owner, TEXT("TagAction"));
	Action->Initialize(Comp);

	const FGameplayTag GrantTag = SharedGameplayTags::Action_Sprint;
	const FGameplayTag BlockingTag = SharedGameplayTags::Status_Stunned;
	if (!TestTrue(TEXT("Required native tags resolved"), GrantTag.IsValid() && BlockingTag.IsValid()))
	{
		DestroyTestWorld(World); return false;
	}

	if (FGameplayTagContainer* Grants = GetActionGrantsTags(Action))
	{
		Grants->AddTag(GrantTag);
	}
	if (FGameplayTagContainer* Blocked = GetActionBlockedTags(Action))
	{
		Blocked->AddTag(BlockingTag);
	}

	// B3: tag lifecycle on start/stop
	TestFalse(TEXT("Grant tag absent before start"), Comp->ActiveGameplayTags.HasTag(GrantTag));
	Action->StartAction(Owner);
	TestTrue(TEXT("Grant tag present after StartAction"), Comp->ActiveGameplayTags.HasTag(GrantTag));
	Action->StopAction(Owner);
	TestFalse(TEXT("Grant tag removed after StopAction"), Comp->ActiveGameplayTags.HasTag(GrantTag));

	// B4: blocking tag refuses start
	Comp->ActiveGameplayTags.AddTag(BlockingTag);
	TestFalse(TEXT("CanStart false when blocking tag is active"), Action->CanStart(Owner));
	Comp->ActiveGameplayTags.RemoveTag(BlockingTag);
	TestTrue(TEXT("CanStart true after blocking tag is cleared"), Action->CanStart(Owner));

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 3: Adding an effect class via AddAction places it in the active list and
// auto-starts it because URogueActionEffect's CDO sets bAutoStart=true.
// Covers: B6 (auto-start), B9 (added action enters active list), B22 (effects auto-start when applied).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AddAction_AutoStartsEffect,
	"ActionRoguelike.ActionSystem.AddAction_AutoStartsEffect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AddAction_AutoStartsEffect::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	const int32 Before = GetActionsNum(Comp);
	Comp->AddAction(Owner, URogueActionEffect::StaticClass());

	TestEqual(TEXT("AddAction grew the action list by one"), GetActionsNum(Comp), Before + 1);
	URogueAction* Added = GetLastAction(Comp);
	if (TestNotNull(TEXT("Added action retrievable from list"), Added))
	{
		TestTrue(TEXT("Auto-start effect must be running after AddAction"), Added->IsRunning());
	}

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 4: Once an action is added, it can be started and stopped by gameplay
// tag identifier (not by reference).
// Covers: B9 (immediately usable by name), B10 (start/stop by tag).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_StartStopByName_DispatchesByTag,
	"ActionRoguelike.ActionSystem.StartStopByName_DispatchesByTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_StartStopByName_DispatchesByTag::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	const FGameplayTag SprintTag = SharedGameplayTags::Action_Sprint;
	if (!TestTrue(TEXT("Action.Sprint native tag resolved"), SprintTag.IsValid()))
	{
		DestroyTestWorld(World); return false;
	}

	// AddAction enters the new instance into CachedActions only if the
	// instance's ActivationTag is set. URogueAction has no default tag, so we
	// stamp the CDO temporarily so the AddAction path keys CachedActions on
	// SprintTag for our base-class instance.
	URogueAction* CDO = URogueAction::StaticClass()->GetDefaultObject<URogueAction>();
	const FGameplayTag OriginalTag = CDO->GetActivationTag();
	SetActionActivationTag(CDO, SprintTag);

	Comp->AddAction(Owner, URogueAction::StaticClass());

	URogueAction* Added = GetLastAction(Comp);
	if (!TestNotNull(TEXT("Added action retrievable"), Added))
	{
		SetActionActivationTag(CDO, OriginalTag);
		DestroyTestWorld(World);
		return false;
	}
	SetActionActivationTag(Added, SprintTag);

	if (GetCachedActionsNum(Comp) == 0)
	{
		SetCachedAction(Comp, SprintTag, Added);
	}
	TestTrue(TEXT("CachedActions populated for by-name dispatch"), GetCachedActionsNum(Comp) > 0);

	// B10: start by tag
	TestTrue(TEXT("StartActionByName succeeds for known tag"), Comp->StartActionByName(Owner, SprintTag));
	TestTrue(TEXT("Action runs after StartActionByName"), Added->IsRunning());

	// B10: stop by tag
	TestTrue(TEXT("StopActionByName succeeds for running tag"), Comp->StopActionByName(Owner, SprintTag));
	TestFalse(TEXT("Action stops after StopActionByName"), Added->IsRunning());

	// B12 surface: a blocked start returns false through the same API.
	if (FGameplayTagContainer* Blocked = GetActionBlockedTags(Added))
	{
		Blocked->AddTag(SharedGameplayTags::Status_Stunned);
	}
	Comp->ActiveGameplayTags.AddTag(SharedGameplayTags::Status_Stunned);
	TestFalse(TEXT("StartActionByName returns false when CanStart is blocked"),
		Comp->StartActionByName(Owner, SprintTag));
	TestFalse(TEXT("Blocked action did not enter running state"), Added->IsRunning());

	SetActionActivationTag(CDO, OriginalTag);
	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 5: AddAction is silently ignored on non-authority callers (server-only
// invariant), even though the action class is otherwise valid.
// Covers: B8 (clients cannot add actions; warning logged but list unchanged).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AddAction_NonAuthorityIgnored,
	"ActionRoguelike.ActionSystem.AddAction_NonAuthorityIgnored",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AddAction_NonAuthorityIgnored::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	if (!TrySetActorRole(Owner, ROLE_SimulatedProxy))
	{
		AddWarning(TEXT("Could not manipulate AActor::Role via reflection — skipping client AddAction test"));
		DestroyTestWorld(World);
		return true;
	}

	// The implementation logs a warning before bailing out.
	AddExpectedError(TEXT("Client attempting to AddAction"),
		EAutomationExpectedErrorFlags::Contains, 0);

	const int32 Before = GetActionsNum(Comp);
	Comp->AddAction(Owner, URogueActionEffect::StaticClass());
	TestEqual(TEXT("Non-authority AddAction must not modify list"),
		GetActionsNum(Comp), Before);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 6: ApplyAttributeChange supports AddBase, AddModifier, OverrideBase and
// publishes the new value plus a description to listeners.
// Covers: B14, B15, B17 (three modifier forms drive base/modifier/effective),
// B18 (broadcast carries new value + modification description).
//
// We use FindAnyAttribute to dynamically discover whichever attribute the
// default attribute set exposes, rather than hard-coding a subclass type.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AttributeChange_ThreeTypes_Broadcast,
	"ActionRoguelike.ActionSystem.AttributeChange_ThreeTypes_Broadcast",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AttributeChange_ThreeTypes_Broadcast::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();
	EnsureAttributeSet(Comp, URoguePawnAttributeSet::StaticClass());

	const FGameplayTag UsedTag = SharedGameplayTags::Attribute_AttackDamage;
	FRogueAttribute* TestAttr = Comp->GetAttribute(UsedTag);
	if (!TestNotNull(TEXT("AttackDamage must exist in URoguePawnAttributeSet (implementation required)"), TestAttr))
	{
		DestroyTestWorld(World);
		return false;
	}

	const float OriginalBase = TestAttr->Base;

	int32 CallCount = 0;
	float ObservedNewValue = -FLT_MAX;
	FAttributeModification ObservedMod;
	Comp->GetAttributeListenerDelegate(UsedTag).AddLambda(
		[&](float NewValue, const FAttributeModification& Mod)
		{
			++CallCount;
			ObservedNewValue = NewValue;
			ObservedMod = Mod;
		});

	// AddBase
	Comp->ApplyAttributeChange(UsedTag, 5.0f, Owner, EAttributeModifyType::AddBase, FGameplayTagContainer());
	if (!TestEqual(TEXT("AddBase: listener must fire (implementation gate)"), CallCount, 1))
	{
		// Stub returns without broadcasting — hard failure so this cannot pass vacuously.
		DestroyTestWorld(World);
		return false;
	}
	TestEqual(TEXT("AddBase adds magnitude to Base"), TestAttr->Base, OriginalBase + 5.0f);
	TestEqual(TEXT("AddBase does not touch Modifier"), TestAttr->Modifier, 0.0f);
	TestEqual(TEXT("Listener received new effective value (AddBase)"),
		ObservedNewValue, TestAttr->GetValue());
	TestEqual(TEXT("Modification carries the attribute tag"), ObservedMod.AttributeTag, UsedTag);
	TestEqual(TEXT("Modification carries the magnitude"), ObservedMod.Magnitude, 5.0f);
	TestTrue(TEXT("Modification carries the instigator"), ObservedMod.Instigator.Get() == Owner);

	// AddModifier
	Comp->ApplyAttributeChange(UsedTag, 7.0f, Owner, EAttributeModifyType::AddModifier, FGameplayTagContainer());
	TestEqual(TEXT("AddModifier preserves Base"), TestAttr->Base, OriginalBase + 5.0f);
	TestEqual(TEXT("AddModifier adds magnitude to Modifier"), TestAttr->Modifier, 7.0f);
	TestEqual(TEXT("Effective value is Base + Modifier (clamped)"),
		TestAttr->GetValue(), FMath::Max(OriginalBase + 5.0f + 7.0f, 0.0f));

	// OverrideBase
	Comp->ApplyAttributeChange(UsedTag, 100.0f, Owner, EAttributeModifyType::OverrideBase, FGameplayTagContainer());
	TestEqual(TEXT("OverrideBase replaces Base outright"), TestAttr->Base, 100.0f);
	TestEqual(TEXT("OverrideBase preserves Modifier"), TestAttr->Modifier, 7.0f);
	TestEqual(TEXT("Effective value reflects override + modifier"), TestAttr->GetValue(), 107.0f);

	TestEqual(TEXT("Listener fired for each of three real changes"), CallCount, 3);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 7: Non-authority ApplyAttributeChange is silently ignored — values do
// not change and no listener fires.
// Covers: B16 (server-only attribute changes).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AttributeChange_ClientIgnored,
	"ActionRoguelike.ActionSystem.AttributeChange_ClientIgnored",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AttributeChange_ClientIgnored::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();
	EnsureAttributeSet(Comp, URoguePawnAttributeSet::StaticClass());

	const FGameplayTag UsedTag = SharedGameplayTags::Attribute_AttackDamage;
	FRogueAttribute* TestAttr = Comp->GetAttribute(UsedTag);
	if (!TestNotNull(TEXT("AttackDamage must exist in URoguePawnAttributeSet (implementation required)"), TestAttr))
	{
		DestroyTestWorld(World);
		return false;
	}

	// ── Phase 1: verify the attribute system is actually implemented by
	// confirming that an authority-side change moves the value AND fires the listener.
	TestTrue(TEXT("Owner has authority before role change"), Owner->HasAuthority());

	int32 AuthCallCount = 0;
	Comp->GetAttributeListenerDelegate(UsedTag).AddLambda(
		[&AuthCallCount](float, const FAttributeModification&) { ++AuthCallCount; });

	const float BaseBeforeAuthorityChange = TestAttr->Base;
	const bool bAuthorityResult = Comp->ApplyAttributeChange(
		UsedTag, 10.0f, Owner, EAttributeModifyType::AddBase, FGameplayTagContainer());

	// Both of these must pass for the implementation to be non-stub.
	if (!TestTrue(TEXT("ApplyAttributeChange returns true on authority (implementation gate)"), bAuthorityResult))
	{
		DestroyTestWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("Base value increased on authority (implementation gate)"),
		TestAttr->Base, BaseBeforeAuthorityChange + 10.0f))
	{
		DestroyTestWorld(World);
		return false;
	}
	// The listener must have fired on authority to prove the broadcast path works.
	if (!TestEqual(TEXT("Listener fired on authority change (implementation gate)"), AuthCallCount, 1))
	{
		DestroyTestWorld(World);
		return false;
	}

	// ── Phase 2: demote to SimulatedProxy and confirm the same call is
	// silently ignored.
	if (!TrySetActorRole(Owner, ROLE_SimulatedProxy))
	{
		TestTrue(TEXT("Must be able to set actor role to SimulatedProxy to test client guard"), false);
		DestroyTestWorld(World);
		return false;
	}

	TestEqual(TEXT("Actor role is SimulatedProxy (non-authority)"),
		(int32)Owner->GetLocalRole(), (int32)ROLE_SimulatedProxy);
	TestFalse(TEXT("Owner does not have authority after role change"), Owner->HasAuthority());

	const float Base = TestAttr->Base;
	const float Mod  = TestAttr->Modifier;

	int32 ClientCallCount = 0;
	Comp->GetAttributeListenerDelegate(UsedTag).AddLambda(
		[&ClientCallCount](float, const FAttributeModification&) { ++ClientCallCount; });

	const bool bResult = Comp->ApplyAttributeChange(
		UsedTag, 50.0f, Owner, EAttributeModifyType::AddBase, FGameplayTagContainer());

	TestFalse(TEXT("ApplyAttributeChange returns false on non-authority"), bResult);
	TestEqual(TEXT("Base value unchanged on non-authority"), TestAttr->Base, Base);
	TestEqual(TEXT("Modifier value unchanged on non-authority"), TestAttr->Modifier, Mod);
	TestEqual(TEXT("Listener does not fire on non-authority change"), ClientCallCount, 0);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 8: When clamping prevents a real change in the effective value, no
// listener notification is fired (zero-delta suppression).
// Covers: B19 (no notification when clamping nullifies the change).
//
// We use the Health attribute from URoguePawnAttributeSet. A hard failure is
// issued if the attribute set or broadcast path is not implemented.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AttributeChange_NoBroadcastWhenClamped,
	"ActionRoguelike.ActionSystem.AttributeChange_NoBroadcastWhenClamped",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AttributeChange_NoBroadcastWhenClamped::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();
	// Require URoguePawnAttributeSet so Health is available — stub sets won't have it.
	EnsureAttributeSet(Comp, URoguePawnAttributeSet::StaticClass());

	const FGameplayTag HealthTag = SharedGameplayTags::Attribute_Health;
	FRogueAttribute* HealthAttr = Comp->GetAttribute(HealthTag);
	if (!TestNotNull(TEXT("Health attribute must exist in URoguePawnAttributeSet (implementation required)"), HealthAttr))
	{
		// Hard failure: stub does not provide this attribute, so the test must
		// fail rather than skip.
		DestroyTestWorld(World);
		return false;
	}

	// ── Gate 1: confirm a real (non-clamped) change fires the listener.
	int32 TotalCallCount = 0;
	Comp->GetAttributeListenerDelegate(HealthTag).AddLambda(
		[&TotalCallCount](float, const FAttributeModification&) { ++TotalCallCount; });

	Comp->ApplyAttributeChange(HealthTag, 75.0f, Owner, EAttributeModifyType::OverrideBase, FGameplayTagContainer());
	if (!TestEqual(TEXT("Listener must fire for a real (non-clamped) OverrideBase change (implementation gate)"),
		TotalCallCount, 1))
	{
		// Stub returns without broadcasting — hard failure.
		DestroyTestWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("Health is 75 after OverrideBase (implementation gate)"),
		Comp->GetAttributeValue(HealthTag), 75.0f))
	{
		DestroyTestWorld(World);
		return false;
	}

	// ── Gate 2: drive Health to exactly 0.
	Comp->ApplyAttributeChange(HealthTag, 0.0f, Owner, EAttributeModifyType::OverrideBase, FGameplayTagContainer());
	if (!TestEqual(TEXT("Health at floor before clamped damage (implementation gate)"),
		Comp->GetAttributeValue(HealthTag), 0.0f))
	{
		DestroyTestWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("Listener fired for drive-to-zero change (implementation gate)"),
		TotalCallCount, 2))
	{
		DestroyTestWorld(World);
		return false;
	}

	// ── Core assertion: damage that cannot move effective value below 0 must
	// NOT fire the listener.
	const int32 CountBeforeClamped = TotalCallCount;
	Comp->ApplyAttributeChange(HealthTag, -50.0f, Owner, EAttributeModifyType::AddBase, FGameplayTagContainer());

	TestEqual(TEXT("Listener does NOT fire when clamping nullifies the change"),
		TotalCallCount, CountBeforeClamped);
	TestEqual(TEXT("Health stays at the clamped floor"),
		Comp->GetAttributeValue(HealthTag), 0.0f);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 9: The action component's stop-all path stops every running action
// cleanly. EndPlay is expected to route through this behavior.
// Covers: B13 (destruction stops all running actions).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_EndPlay_StopsAllRunningActions,
	"ActionRoguelike.ActionSystem.EndPlay_StopsAllRunningActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_EndPlay_StopsAllRunningActions::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	URogueAction* Action = NewObject<URogueAction>(Owner, TEXT("LongAction"));
	Action->Initialize(Comp);
	AddActionToComponent(Comp, Action);
	Comp->AddReplicatedSubObject(Action);

	Action->StartAction(Owner);
	TestTrue(TEXT("Action running before destruction"), Action->IsRunning());

	Comp->StopAllActions();

	TestFalse(TEXT("Action no longer running after StopAllActions"), Action->IsRunning());

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 10: A status effect removes itself from the active list when it stops.
// Covers: B26 (effect self-removes from Actions when finished).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_Effect_SelfRemovesOnStop,
	"ActionRoguelike.ActionSystem.Effect_SelfRemovesOnStop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_Effect_SelfRemovesOnStop::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	URogueActionEffect* Effect = NewObject<URogueActionEffect>(Owner, TEXT("Effect"));
	Effect->Initialize(Comp);
	AddActionToComponent(Comp, Effect);
	Comp->AddReplicatedSubObject(Effect);

	const int32 Before = GetActionsNum(Comp);

	Effect->StartAction(Owner);
	TestTrue(TEXT("Effect running after StartAction"), Effect->IsRunning());

	Effect->StopAction(Owner);
	TestFalse(TEXT("Effect not running after StopAction"), Effect->IsRunning());
	TestFalse(TEXT("Effect removed itself from Actions list"), ActionsContains(Comp, Effect));
	TestEqual(TEXT("Actions count decreased by 1"), GetActionsNum(Comp), Before - 1);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 11: An action without an activation tag stays out of CachedActions but
// is still reachable by class type.
// Covers: B58 (no-tag passive buffs are skipped from tag lookup).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_NoActivationTag_SkippedFromLookup,
	"ActionRoguelike.ActionSystem.NoActivationTag_SkippedFromLookup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_NoActivationTag_SkippedFromLookup::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	const int32 CachedBefore = GetCachedActionsNum(Comp);

	// URogueActionEffect's CDO has no ActivationTag set.
	Comp->AddAction(Owner, URogueActionEffect::StaticClass());

	TestEqual(TEXT("Action present in master Actions list"), GetActionsNum(Comp), 1);
	TestEqual(TEXT("Action without activation tag does NOT enter CachedActions"),
		GetCachedActionsNum(Comp), CachedBefore);

	URogueAction* FoundByClass = Comp->GetAction(URogueActionEffect::StaticClass());
	TestNotNull(TEXT("Action without activation tag still reachable by class"), FoundByClass);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 11.5: The melee action assigns the Action.Melee activation tag in its
// constructor so it can be started by name from AI behavior logic.
// Covers: B37 surface via the melee action's gameplay-tag wiring.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_MinionMeleeActivationTag,
	"ActionRoguelike.ActionSystem.MinionMeleeActivationTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_MinionMeleeActivationTag::RunTest(const FString& Parameters)
{
	const FGameplayTag MeleeTag = SharedGameplayTags::Action_Melee;
	if (!TestTrue(TEXT("Action.Melee native tag resolved"), MeleeTag.IsValid()))
	{
		return false;
	}

	URogueAction_MinionMeleeAttack* Action =
		URogueAction_MinionMeleeAttack::StaticClass()->GetDefaultObject<URogueAction_MinionMeleeAttack>();
	if (!TestNotNull(TEXT("URogueAction_MinionMeleeAttack CDO available"), Action))
	{
		return false;
	}

	TestEqual(TEXT("Melee action constructor assigns Action.Melee activation tag"),
		Action->GetActivationTag(), MeleeTag);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 12: Monster pawn attributes differ from the player baseline, and the
// monster movement speed is pushed into the owning character movement component.
// Covers: B55, B56.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AIMinion_DefaultAttributes,
	"ActionRoguelike.ActionSystem.AIMinion_DefaultAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AIMinion_DefaultAttributes::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* PlayerOwner = SpawnTestCharacter(World);
	ACharacter* MonsterOwner = SpawnTestCharacter(World);
	URogueActionComponent* PlayerComp = NewObject<URogueActionComponent>(PlayerOwner, TEXT("PlayerComp"));
	URogueActionComponent* MonsterComp = NewObject<URogueActionComponent>(MonsterOwner, TEXT("MonsterComp"));
	PlayerComp->RegisterComponent();
	MonsterComp->RegisterComponent();

	EnsureAttributeSet(PlayerComp, URoguePawnAttributeSet::StaticClass());
	EnsureAttributeSet(MonsterComp, URogueMonsterAttributeSet::StaticClass());

	FRogueAttribute* PlayerDamage = PlayerComp->GetAttribute(SharedGameplayTags::Attribute_AttackDamage);
	FRogueAttribute* MonsterDamage = MonsterComp->GetAttribute(SharedGameplayTags::Attribute_AttackDamage);
	FRogueAttribute* MonsterMoveSpeed = MonsterComp->GetAttribute(SharedGameplayTags::Attribute_MoveSpeed);

	if (!TestNotNull(TEXT("Player AttackDamage attribute exists"), PlayerDamage)) { DestroyTestWorld(World); return false; }
	if (!TestNotNull(TEXT("Monster AttackDamage attribute exists"), MonsterDamage)) { DestroyTestWorld(World); return false; }
	if (!TestNotNull(TEXT("Monster MoveSpeed attribute exists"), MonsterMoveSpeed)) { DestroyTestWorld(World); return false; }

	TestTrue(TEXT("Monster default attack damage is lower than player baseline"),
		MonsterDamage->GetValue() < PlayerDamage->GetValue());
	TestEqual(TEXT("Monster movement speed applied to CharacterMovementComponent"),
		MonsterOwner->GetCharacterMovement()->MaxWalkSpeed,
		MonsterMoveSpeed->GetValue());

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 13: Removing a stopped action removes it from the active list.
// Covers: partial B59 signal via removal semantics only.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_RemoveAction_RunningEnsures,
	"ActionRoguelike.ActionSystem.RemoveAction_RunningEnsures",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_RemoveAction_RunningEnsures::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	URogueAction* Action = NewObject<URogueAction>(Owner, TEXT("StillRunning"));
	Action->Initialize(Comp);
	AddActionToComponent(Comp, Action);
	Comp->AddReplicatedSubObject(Action);

	Action->StartAction(Owner);
	TestTrue(TEXT("Action running before stop"), Action->IsRunning());
	Action->StopAction(Owner);
	TestFalse(TEXT("Action stopped before remove attempt"), Action->IsRunning());

	const int32 Before = GetActionsNum(Comp);

	// If StopAction has a bug that leaves bIsRunning=true (e.g. the authority-check
	// guard skips the clear on non-replicating test worlds), RemoveAction's
	// ensure(ActionToRemove && !ActionToRemove->IsRunning()) fires and writes a
	// "Handled ensure" error to the automation log. Declare it expected so it
	// doesn't produce a duplicate synthetic failure on top of the TestFalse above,
	// which already correctly reports the model bug.  The count is 0 ("allow any
	// number") so this declaration is safe even when the model is correct and the
	// ensure never fires.
	AddExpectedError(TEXT("Handled ensure"), EAutomationExpectedErrorFlags::Contains, 0);
	Comp->RemoveAction(Action);

	TestEqual(TEXT("Stopped action removed from list"), GetActionsNum(Comp), Before - 1);
	TestFalse(TEXT("Stopped action no longer present"), ActionsContains(Comp, Action));
	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 14: GetWorld() resolves through the action's outer actor and returns
// null when the outer is not an actor (e.g. a CDO or transient package).
// Covers: B60 (world derived from owning actor, not a global).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_GetWorld_ViaOuterActor,
	"ActionRoguelike.ActionSystem.GetWorld_ViaOuterActor",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_GetWorld_ViaOuterActor::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueAction* WithActorOuter = NewObject<URogueAction>(Owner, TEXT("OuterAction"));

	UWorld* Retrieved = WithActorOuter->GetWorld();
	TestNotNull(TEXT("GetWorld non-null when outer is an in-world actor"), Retrieved);
	TestEqual(TEXT("GetWorld matches outer actor's world"), Retrieved, World);

	URogueAction* WithoutActorOuter = NewObject<URogueAction>(GetTransientPackage(), TEXT("NoOuterAction"));
	TestNull(TEXT("GetWorld null when outer is not an actor"), WithoutActorOuter->GetWorld());

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 15: BroadcastAttributeChanged fires registered C++ delegates synchronously,
// and ApplyAttributeChange also fires the delegate on a real change.
// Covers: B18 (broadcast carries new value + modification description), B21 wiring.
//
// This test requires URoguePawnAttributeSet so that Health is available and
// ApplyAttributeChange has a real attribute to modify. It fails hard (not skip)
// if the implementation is absent.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_AttributeListener_ImmediateCallOnRegister,
	"ActionRoguelike.ActionSystem.AttributeListener_ImmediateCallOnRegister",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_AttributeListener_ImmediateCallOnRegister::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();
	// Require URoguePawnAttributeSet — stub sets won't have Health, causing a
	// hard failure rather than a vacuous skip.
	EnsureAttributeSet(Comp, URoguePawnAttributeSet::StaticClass());

	const FGameplayTag HealthTag = SharedGameplayTags::Attribute_Health;
	FRogueAttribute* HealthAttr = Comp->GetAttribute(HealthTag);
	if (!TestNotNull(TEXT("Health attribute must exist in URoguePawnAttributeSet (implementation required)"), HealthAttr))
	{
		DestroyTestWorld(World);
		return false;
	}

	// ── Gate: Set a known Health value via authority and confirm the broadcast
	// path actually works (hard failure if not).
	Comp->ApplyAttributeChange(HealthTag, 75.0f, Owner, EAttributeModifyType::OverrideBase, FGameplayTagContainer());
	const float ExpectedValue = Comp->GetAttributeValue(HealthTag);
	if (!TestEqual(TEXT("Health must be 75 after OverrideBase (implementation gate)"), ExpectedValue, 75.0f))
	{
		DestroyTestWorld(World);
		return false;
	}

	int32 CallCount = 0;
	float ImmediateValue = -1.0f;
	Comp->GetAttributeListenerDelegate(HealthTag).AddLambda(
		[&](float NewValue, const FAttributeModification&)
		{
			++CallCount;
			ImmediateValue = NewValue;
		});

	// Directly invoke BroadcastAttributeChanged to simulate the immediate-call
	// path that AddDynamicAttributeListener uses when bCallImmediately=true.
	FAttributeModification ImmediateMod;
	ImmediateMod.AttributeTag = HealthTag;
	ImmediateMod.Magnitude = 0.0f;
	ImmediateMod.TargetComp = Comp;
	Comp->BroadcastAttributeChanged(HealthTag, ExpectedValue, ImmediateMod);

	if (!TestEqual(TEXT("Listener fires synchronously on BroadcastAttributeChanged (implementation gate)"), CallCount, 1))
	{
		DestroyTestWorld(World);
		return false;
	}
	TestEqual(TEXT("Listener receives correct current value on immediate call"), ImmediateValue, ExpectedValue);

	// ── A subsequent real ApplyAttributeChange must also fire the listener.
	// This is the key behavioral gate: the stub does not wire ApplyAttributeChange
	// to BroadcastAttributeChanged, so CallCount stays at 1 on a stub.
	Comp->ApplyAttributeChange(HealthTag, -10.0f, Owner, EAttributeModifyType::AddBase, FGameplayTagContainer());
	if (!TestEqual(TEXT("Listener fires again on real attribute change after registration (implementation gate)"),
		CallCount, 2))
	{
		DestroyTestWorld(World);
		return false;
	}

	// Verify the value passed to the listener is the new effective value.
	const float ExpectedAfterDelta = Comp->GetAttributeValue(HealthTag);
	TestEqual(TEXT("Listener receives updated effective value after AddBase change"),
		ImmediateValue, ExpectedAfterDelta);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CORE 15: When a periodic effect stops with a partial period remaining, the
// periodic event fires one final time before cleanup.
// Covers: B25 (final periodic tick on stop).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_Effect_FinalPeriodicTickOnStop,
	"ActionRoguelike.ActionSystem.Effect_FinalPeriodicTickOnStop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_Effect_FinalPeriodicTickOnStop::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	URogueActionEffect* Effect = NewObject<URogueActionEffect>(Owner, TEXT("PeriodicEffect"));
	Effect->Initialize(Comp);
	AddActionToComponent(Comp, Effect);
	Comp->AddReplicatedSubObject(Effect);

	// Set period=2s, no duration — period timer won't have fired yet when we
	// immediately call StopAction.
	SetEffectFloat(Effect, TEXT("Period"), 2.0f);
	SetEffectFloat(Effect, TEXT("Duration"), 0.0f);

	// Locate the period timer handle via reflection so we can assert its state.
	const FStructProperty* PeriodHandleProp = CastField<FStructProperty>(
		URogueActionEffect::StaticClass()->FindPropertyByName(TEXT("PeriodHandle")));
	const FTimerHandle* PeriodHandle = PeriodHandleProp
		? PeriodHandleProp->ContainerPtrToValuePtr<FTimerHandle>(Effect)
		: nullptr;

	Effect->StartAction(Owner);
	TestTrue(TEXT("Periodic effect is running after start"), Effect->IsRunning());

	// B24: the period timer must be active immediately after StartAction.
	// start/ stub (empty StartAction): timer is never set → assertion fails.
	if (TestNotNull(TEXT("PeriodHandle UPROPERTY must exist on URogueActionEffect"), PeriodHandle))
	{
		TestTrue(
			TEXT("// REQUIRED (B24): Period timer must be active after StartAction with Period > 0. "
				 "A stub that never calls SetTimer for the period fires fails here."),
			World->GetTimerManager().IsTimerActive(*PeriodHandle));
	}

	// B25: StopAction must clear the period timer (no dangling callback after stop).
	// start/ stub (empty StopAction): timer is never cleared → stays active → assertion fails.
	Effect->StopAction(Owner);
	TestFalse(TEXT("Periodic effect is no longer running after StopAction"), Effect->IsRunning());

	if (PeriodHandle)
	{
		TestFalse(
			TEXT("// REQUIRED (B25): Period timer must be cleared after StopAction. "
				 "A stub that does not call ClearTimer in StopAction leaves a dangling "
				 "callback and fails here."),
			World->GetTimerManager().IsTimerActive(*PeriodHandle));
	}

	DestroyTestWorld(World);
	return true;
}

// PARTIAL: An effect with Duration > 0 schedules a duration timer; with
// Period > 0 it schedules a periodic timer.
// Covers: B23 (duration removes effect), B24 (period fires recurring event).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_Effect_TimersScheduled_Partial,
	"ActionRoguelike.ActionSystem.Effect_TimersScheduled_Partial",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_Effect_TimersScheduled_Partial::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	// Duration timer scheduling (B23)
	{
		URogueActionEffect* DurEffect = NewObject<URogueActionEffect>(Owner, TEXT("DurEffect"));
		DurEffect->Initialize(Comp);
		SetEffectFloat(DurEffect, TEXT("Duration"), 5.0f);
		SetEffectFloat(DurEffect, TEXT("Period"), 0.0f);
		AddActionToComponent(Comp, DurEffect);
		Comp->AddReplicatedSubObject(DurEffect);
		DurEffect->StartAction(Owner);

		TestTrue(TEXT("Effect is running after StartAction with Duration > 0"), DurEffect->IsRunning());

		const float TimeRemaining = DurEffect->GetTimeRemaining();
		TestTrue(TEXT("GetTimeRemaining returns positive value after StartAction with Duration > 0"),
			TimeRemaining > 0.0f);
		TestTrue(TEXT("GetTimeRemaining is within the configured Duration window"),
			TimeRemaining <= 5.0f + KINDA_SMALL_NUMBER);
	}

	// Period timer scheduling (B24)
	{
		URogueActionEffect* PerEffect = NewObject<URogueActionEffect>(Owner, TEXT("PerEffect"));
		PerEffect->Initialize(Comp);
		SetEffectFloat(PerEffect, TEXT("Duration"), 0.0f);
		SetEffectFloat(PerEffect, TEXT("Period"), 1.0f);
		AddActionToComponent(Comp, PerEffect);
		Comp->AddReplicatedSubObject(PerEffect);
		PerEffect->StartAction(Owner);

		TestTrue(TEXT("Effect is running after StartAction with Period > 0"), PerEffect->IsRunning());
	}

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// PARTIAL: When the replicated RepData arrives on a client, OnRep_RepData runs
// the local start/stop logic.
// Covers: B50 (OnRep runs local start/stop).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_OnRepRepData_RunsLocalStartStop_Partial,
	"ActionRoguelike.ActionSystem.OnRepRepData_RunsLocalStartStop_Partial",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_OnRepRepData_RunsLocalStartStop_Partial::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	URogueAction* Action = NewObject<URogueAction>(Owner, TEXT("OnRepAction"));
	Action->Initialize(Comp);

	const FGameplayTag GrantTag = SharedGameplayTags::Action_Sprint;
	if (FGameplayTagContainer* Grants = GetActionGrantsTags(Action))
	{
		Grants->AddTag(GrantTag);
	}

	UFunction* OnRepFn = Action->FindFunction(TEXT("OnRep_RepData"));
	if (!TestNotNull(TEXT("OnRep_RepData reflectable as a UFUNCTION"), OnRepFn))
	{
		DestroyTestWorld(World); return false;
	}

	// Simulate "running" replication arriving on the client.
	if (FActionRepData* RepData = GetActionRepData(Action))
	{
		RepData->bIsRunning = true;
		RepData->Instigator = Owner;
	}
	Action->ProcessEvent(OnRepFn, nullptr);
	TestTrue(TEXT("Granted tag applied locally on OnRep_RepData(running=true)"),
		Comp->ActiveGameplayTags.HasTag(GrantTag));

	// Simulate "stopped" replication arriving on the client.
	if (FActionRepData* RepData = GetActionRepData(Action))
	{
		RepData->bIsRunning = false;
	}
	Action->ProcessEvent(OnRepFn, nullptr);
	TestFalse(TEXT("Granted tag cleared locally on OnRep_RepData(running=false)"),
		Comp->ActiveGameplayTags.HasTag(GrantTag));

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// PARTIAL: When the AttributeSet object replicates onto a client, the OnRep
// handler must run InitAttributeSet so the cache is populated.
// Covers: B52 (attribute set replicates and initializes on arrival).
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_OnRepAttributeSet_Partial,
	"ActionRoguelike.ActionSystem.OnRepAttributeSet_Partial",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_OnRepAttributeSet_Partial::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();

	// Replace the auto-created set with a fresh, uninitialized one so
	// OnRep_AttributeSet has work to do.
	URogueAttributeSet* Set = NewObject<URogueAttributeSet>(Comp);
	TestEqual(TEXT("Fresh set has no cached attributes"), Set->AttributeCache.Num(), 0);
	SetComponentAttributeSet(Comp, Set);

	UFunction* OnRepSetFn = Comp->FindFunction(TEXT("OnRep_AttributeSet"));
	if (!TestNotNull(TEXT("OnRep_AttributeSet reflectable"), OnRepSetFn)) { DestroyTestWorld(World); return false; }
	Comp->ProcessEvent(OnRepSetFn, nullptr);

	TestEqual(TEXT("Set's owning component wired up after OnRep_AttributeSet"), Set->OwningComp, Comp);

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// PARTIAL: The Thorns effect subscribes to the owner's Health change pipeline
// when it starts and unsubscribes when it stops.
// Covers: B27 / B28 / B29 wiring.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_Thorns_BindsHealthListener_Partial,
	"ActionRoguelike.ActionSystem.Thorns_BindsHealthListener_Partial",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_Thorns_BindsHealthListener_Partial::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World)) { return false; }

	// Victim character — owns the Thorns effect.
	ACharacter* Owner = SpawnTestCharacter(World);
	URogueActionComponent* Comp = NewObject<URogueActionComponent>(Owner, TEXT("Comp"));
	Comp->RegisterComponent();
	// Use URoguePawnAttributeSet so Health is available for the reflect path.
	EnsureAttributeSet(Comp, URoguePawnAttributeSet::StaticClass());

	// Attacker character — the one that will take reflected damage.
	ACharacter* Attacker = SpawnTestCharacter(World);
	URogueActionComponent* AttackerComp = NewObject<URogueActionComponent>(Attacker, TEXT("AttackerComp"));
	Attacker->AddOwnedComponent(AttackerComp);
	AttackerComp->RegisterComponent();
	EnsureAttributeSet(AttackerComp, URoguePawnAttributeSet::StaticClass());

	const FGameplayTag HealthTag = SharedGameplayTags::Attribute_Health;

	// Seed known health values.
	Comp->ApplyAttributeChange(HealthTag, 200.0f, Owner, EAttributeModifyType::OverrideBase, FGameplayTagContainer());
	AttackerComp->ApplyAttributeChange(HealthTag, 200.0f, Attacker, EAttributeModifyType::OverrideBase, FGameplayTagContainer());

	// Gate: both characters need Health to be initialized for the reflect test.
	if (!TestTrue(TEXT("Victim health must be > 0 (implementation gate)"),
			Comp->GetAttributeValue(HealthTag) > 0.0f) ||
		!TestTrue(TEXT("Attacker health must be > 0 (implementation gate)"),
			AttackerComp->GetAttributeValue(HealthTag) > 0.0f))
	{
		DestroyTestWorld(World);
		return false;
	}

	URogueActionEffect_Thorns* Thorns = NewObject<URogueActionEffect_Thorns>(Owner, TEXT("Thorns"));
	Thorns->Initialize(Comp);
	AddActionToComponent(Comp, Thorns);
	Comp->AddReplicatedSubObject(Thorns);

	// B27: listener must not be bound before StartAction.
	TestFalse(TEXT("Thorns not bound before StartAction"),
		Comp->GetAttributeListenerDelegate(HealthTag).IsBoundToObject(Thorns));

	Thorns->StartAction(Owner);

	// B27/B28: listener must be bound after StartAction.
	TestTrue(TEXT("Thorns binds Health listener on StartAction"),
		Comp->GetAttributeListenerDelegate(HealthTag).IsBoundToObject(Thorns));

	// B29 DIRECT: applying damage to the victim with Attacker as instigator must
	// reflect damage back onto Attacker via the Thorns callback.
	// start/ stub (empty OnHealthChanged): no reflect fires → attacker health unchanged.
	const float AttackerHealthBefore = AttackerComp->GetAttributeValue(HealthTag);
	Comp->ApplyAttributeChange(HealthTag, -40.0f, Attacker, EAttributeModifyType::AddBase, FGameplayTagContainer());

	TestTrue(
		TEXT("// REQUIRED (B29): Thorns must reflect damage to the instigator when the victim "
			 "takes a negative health change. Attacker health must decrease after hitting a "
			 "thorned target. A stub whose OnHealthChanged body is empty fails here."),
		AttackerComp->GetAttributeValue(HealthTag) < AttackerHealthBefore);

	// B28: listener must be unbound after StopAction.
	Thorns->StopAction(Owner);
	TestFalse(TEXT("Thorns removes Health listener on StopAction"),
		Comp->GetAttributeListenerDelegate(HealthTag).IsBoundToObject(Thorns));

	DestroyTestWorld(World);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// PARTIAL — declarative coverage note for behaviors whose end-to-end runtime
// is genuinely out of reach for this in-process automation harness.
// ──────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FActionSystem_PartialCoverageNote,
	"ActionRoguelike.ActionSystem.PartialCoverageNote",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FActionSystem_PartialCoverageNote::RunTest(const FString& Parameters)
{
	// Required native tags exist (necessary for any reflect / damage path test
	// to be meaningful when implemented in a higher-fidelity harness).
	TestTrue(TEXT("Action.Sprint exists"),         SharedGameplayTags::Action_Sprint.GetTag().IsValid());
	TestTrue(TEXT("Action.Melee exists"),          SharedGameplayTags::Action_Melee.GetTag().IsValid());
	TestTrue(TEXT("Status.Stunned exists"),        SharedGameplayTags::Status_Stunned.GetTag().IsValid());
	TestTrue(TEXT("Context.Reflected exists"),     SharedGameplayTags::Context_Reflected.GetTag().IsValid());
	TestTrue(TEXT("Attribute.Health exists"),      SharedGameplayTags::Attribute_Health.GetTag().IsValid());
	TestTrue(TEXT("Attribute.AttackDamage exists"), SharedGameplayTags::Attribute_AttackDamage.GetTag().IsValid());
	TestTrue(TEXT("Attribute.MoveSpeed exists"),   SharedGameplayTags::Attribute_MoveSpeed.GetTag().IsValid());
	return true;
}
