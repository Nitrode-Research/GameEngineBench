// Copyright (c) 2026 GameDevBench. Player-controller input-wiring automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/Controllers/BmrPlayerController.cpp
//
// The controller owns the input lifecycle for the local player:
//   * State-driven Enhanced-Input mapping-context toggling (the flat list of
//     UBmrInputMappingContext owned by ABmrPlayerController::AllInputContexts).
//   * Input-action binding with a dynamic target resolved at bind time via
//     each action's FunctionPickerTemplate::FOnGetterObject delegate.
//   * Slate navigation suppression so UMG tab/arrow navigation does not steal
//     focus from gameplay keys.
//   * Player-state reuse on reconnect, so a returning client keeps score and
//     loadout instead of getting a freshly-spawned ABmrPlayerState.
//   * Pool-managed pawn preservation in PawnLeavingGame — Super destroys the
//     pawn, which the actor-pooling subsystem expects to reuse.
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface would need a hydrated UBmrPlayerInputDataAsset
//   (a Data-Registry-backed asset that DAL-streams the full set of UBmrInputMappingContext
//   assets, each carrying a UBmrInputAction list and per-action FFunctionPicker delegates),
//   a registered UBmrWidgetsSubsystem with AreWidgetInitialized()==true, an active
//   ABmrGameState matching one of the FBmrGameStateTag families, and a UDalSubsystem
//   actually loading the data asset on a worker. None of that is reachable from a headless
//   automation harness in this project without bootstrapping the entire menu and gameplay
//   pipeline; ApplyAllInputContexts early-outs on IsLocalController() / CanBindInputActions()
//   in the test world before any of the body under test gets to run.
//
//   The spec and evaluator notes pin a small set of code-path choices that separate
//   a working impl from a plausibly-working one — each test below targets one of the
//   required B-behaviors (B1–B7 from the task brief). Where a behavior has multiple
//   acceptable spellings (`Super::PawnLeavingGame()` vs `Super::PawnLeavingGame ()`,
//   `.HasTag(...)` vs `.HasTagExact(...)`, `bEnable == true` vs `bEnable`), the
//   assertions accept the equivalences that produce identical runtime behavior.

// Bomber (game module — kept at top per task rules)
#include "Controllers/BmrPlayerController.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "Structures/BmrGameStateTag.h"

// UE
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayerControllerTests, Log, All);

namespace PlayerControllerTests
{
	// The stripped file the task targets. The header is unchanged in this task;
	// the public API surface (SetAllInputContextsEnabled, BindInputActionsInContext,
	// SetUIInputIgnored, PawnLeavingGame override, InitPlayerState override) is
	// already declared. The .cpp's bodies are what was stripped.
	static const TCHAR* PlayerControllerRelPath =
		TEXT("Source/Bomber/Private/Controllers/BmrPlayerController.cpp");

	static FString LoadProjectFile(FAutomationTestBase* Test, const TCHAR* RelativePath)
	{
		const FString AbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		FString Source;
		if (!Test->TestTrue(
				FString::Printf(TEXT("Source file must be readable at %s"), *AbsPath),
				FFileHelper::LoadFileToString(Source, *AbsPath)))
		{
			return FString();
		}
		return Source;
	}

	// Strips C++ line and block comments from source text so a substring scan
	// does not match anti-pattern descriptions written inside comments (the
	// canonical solution references "Don't call super to avoid destroying the
	// pawn" — that line must not satisfy a `Super::` scan). String literals are
	// preserved so TEXT("MouseActivityComponent") inside a CreateDefaultSubobject
	// call still shows up.
	static FString StripComments(const FString& Source)
	{
		FString Out;
		Out.Reserve(Source.Len());
		bool bInLineComment = false;
		bool bInBlockComment = false;
		bool bInString = false;
		bool bInChar = false;
		for (int32 i = 0; i < Source.Len(); ++i)
		{
			const TCHAR C = Source[i];
			const TCHAR N = (i + 1 < Source.Len()) ? Source[i + 1] : TCHAR('\0');
			if (bInLineComment)
			{
				if (C == TCHAR('\n'))
				{
					bInLineComment = false;
					Out.AppendChar(C);
				}
				continue;
			}
			if (bInBlockComment)
			{
				if (C == TCHAR('*') && N == TCHAR('/'))
				{
					bInBlockComment = false;
					++i;
				}
				continue;
			}
			if (bInString)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++i;
				}
				else if (C == TCHAR('"'))
				{
					bInString = false;
				}
				continue;
			}
			if (bInChar)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++i;
				}
				else if (C == TCHAR('\''))
				{
					bInChar = false;
				}
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('/'))
			{
				bInLineComment = true;
				++i;
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('*'))
			{
				bInBlockComment = true;
				++i;
				continue;
			}
			if (C == TCHAR('"'))
			{
				bInString = true;
				Out.AppendChar(C);
				continue;
			}
			if (C == TCHAR('\''))
			{
				bInChar = true;
				Out.AppendChar(C);
				continue;
			}
			Out.AppendChar(C);
		}
		return Out;
	}

	// Brace-depth walker to extract a member-function body. Counts braces only,
	// not parens, so lambdas / initialiser lists nested inside the body do not
	// confuse the scan.
	static FString ExtractMemberBody(const FString& Source, const FString& Signature)
	{
		const int32 DefIdx = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (DefIdx == INDEX_NONE)
		{
			return FString();
		}
		const int32 BodyStart = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DefIdx);
		if (BodyStart == INDEX_NONE)
		{
			return FString();
		}
		int32 Depth = 0;
		for (int32 i = BodyStart; i < Source.Len(); ++i)
		{
			const TCHAR C = Source[i];
			if (C == TCHAR('{'))
			{
				++Depth;
			}
			else if (C == TCHAR('}'))
			{
				--Depth;
				if (Depth == 0)
				{
					return Source.Mid(BodyStart, i - BodyStart + 1);
				}
			}
		}
		return FString();
	}
} // namespace PlayerControllerTests

// ---------------------------------------------------------------------------
// Test 1 — B1: SetAllInputContextsEnabled iterates AllInputContexts and only
//          touches contexts whose state-tag set matches the requested
//          FBmrGameStateTag (flat-list bitmask check, NOT a push/pop stack).
//
// The spec: "For contexts whose GameStatesBitmask matches the tag:
// AddMappingContext when bEnable is true." The canonical encoding iterates the
// controller's AllInputContexts flat list, reads each context's tag container
// (GetActiveForStates() or equivalent), and tests membership of the passed
// GameStateTag. A re-impl that maintains a parallel stack of active contexts
// — pushed on enable and popped on disable — silently leaks contexts whose
// state-set overlaps with another's, the named failure mode in B3.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_SetAllInputContextsEnabledIteratesFlatListByStateTag,
	"Bomber.PlayerController.SetAllInputContextsEnabledIteratesFlatListByStateTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_SetAllInputContextsEnabledIteratesFlatListByStateTag::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::SetAllInputContextsEnabled"));
	if (!TestTrue(
			TEXT("B1: BmrPlayerController.cpp must define SetAllInputContextsEnabled with a body — "
				 "the start/ stub leaves the body empty; the state-driven toggle is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Must walk the controller's flat list of managed contexts. A re-impl that
	// fetches contexts directly from the data asset would not respect runtime
	// AddNewInputContexts / RemoveInputContexts edits, which is the very point
	// of having a controller-owned cache.
	TestTrue(
		TEXT("B1: SetAllInputContextsEnabled must iterate the controller's AllInputContexts list. "
			 "That field is the flat, runtime-mutable cache the spec names. Reading contexts from "
			 "the data asset directly would lose any context added via AddNewInputContexts after "
			 "data-asset load, and any removed via RemoveInputContexts after a feature opt-out."),
		CodeOnly.Contains(TEXT("AllInputContexts"), ESearchCase::CaseSensitive));

	// Must read each context's state-tag set and test membership of the
	// requested GameStateTag. The header exposes GetActiveForStates() returning
	// FGameplayTagContainer; HasTag/HasTagExact/HasAny is the canonical check.
	const bool bReadsStateSet =
		CodeOnly.Contains(TEXT("GetActiveForStates"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("ActiveForStates"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GameStatesBitmask"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: SetAllInputContextsEnabled must read each context's state-tag set "
			 "(GetActiveForStates() or equivalent GameStatesBitmask accessor) so it can test the "
			 "passed FBmrGameStateTag against it. Without this read the routine cannot tell "
			 "which contexts apply to the current state and would either enable all or none."),
		bReadsStateSet);

	const bool bTestsTagMembership =
		CodeOnly.Contains(TEXT("HasTag"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("HasAny"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MatchesAny"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MatchesTag"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: SetAllInputContextsEnabled must perform a tag-membership test (.HasTag(...), "
			 ".HasTagExact(...), .HasAny(...), or equivalent) against the requested GameStateTag. "
			 "Equality on the GameplayTag struct alone is incompatible with hierarchical tags."),
		bTestsTagMembership);

	// The matching branch must reach SetInputContextEnabled — that's the helper
	// that calls UInputUtilsLibrary::SetInputContextEnabled which in turn lands
	// on the Enhanced-Input subsystem's AddMappingContext/RemoveMappingContext.
	// A re-impl that touches the subsystem directly bypasses BindInputActionsInContext
	// (which SetInputContextEnabled invokes when bEnable==true) and leaves the
	// context with no bound input actions — the spec's named failure mode for B4.
	TestTrue(
		TEXT("B1: SetAllInputContextsEnabled must route per-context toggling through "
			 "SetInputContextEnabled (the helper that wraps the Enhanced-Input subsystem call "
			 "AND invokes BindInputActionsInContext on enable). Calling the subsystem's "
			 "AddMappingContext directly skips the action-binding step and leaves the newly-"
			 "added context inert — the bound actions never fire."),
		CodeOnly.Contains(TEXT("SetInputContextEnabled"), ESearchCase::CaseSensitive));

	// Negative: B3 / B4 (failure mode #1) — no push/pop stack. The canonical
	// implementation uses the flat AllInputContexts list and per-context state
	// matching. A stack-based variant would maintain a separate "ActiveStack"
	// container and Push/Pop into it.
	const bool bUsesStackPattern =
		CodeOnly.Contains(TEXT("ActiveStack"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("ContextStack"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".Push("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".Pop("), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B3: SetAllInputContextsEnabled must NOT use a push/pop IMC stack. The spec calls "
			 "this out: 'a push/pop IMC stack silently leaks contexts when two contexts have "
			 "overlapping bitmasks' — overlapping state-set contexts both push on enter, but "
			 "the disable pops only one, leaking the other into the wrong gameplay state. The "
			 "flat-list + per-context membership test in the canonical solution is immune to "
			 "this because each context's enable/disable is computed independently from its own "
			 "state-tag set."),
		bUsesStackPattern);

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B2: Non-matching contexts are removed unconditionally on game-state
//          transitions. The canonical wiring is OnGameStateChanged → ApplyAllInputContexts,
//          and ApplyAllInputContexts iterates EVERY managed context, computing
//          per-context bShouldEnable from the live game state and dispatching
//          SetInputContextEnabled(bShouldEnable, ...) — so contexts whose
//          state-set does not include the new state get disabled regardless
//          of whether they were enabled before.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_NonMatchingContextsRemovedOnStateChange,
	"Bomber.PlayerController.NonMatchingContextsRemovedOnStateChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_NonMatchingContextsRemovedOnStateChange::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// OnGameStateChanged_Implementation must drive the re-application of ALL
	// contexts when the game state changes. That is the canonical mechanism by
	// which contexts whose state-set does not include the new state are
	// disabled — the spec's "RemoveMappingContext unconditionally for non-
	// matching contexts" requirement is met because ApplyAllInputContexts visits
	// every context and unconditionally calls SetInputContextEnabled(false, …)
	// on each one whose state-set does not match the live game state.
	const FString OnStateBody =
		ExtractMemberBody(Source, TEXT("ABmrPlayerController::OnGameStateChanged_Implementation"));
	if (!TestTrue(
			TEXT("B2: BmrPlayerController.cpp must define OnGameStateChanged_Implementation with a body — "
				 "the start/ stub is empty; the state-change → context re-apply wiring is what was stripped."),
			!OnStateBody.IsEmpty()))
	{
		return false;
	}

	const FString OnStateCode = StripComments(OnStateBody);

	TestTrue(
		TEXT("B2: OnGameStateChanged must re-apply ALL managed input contexts (call "
			 "ApplyAllInputContexts or SetAllInputContextsEnabled, the routines that iterate the "
			 "controller's AllInputContexts list). The spec: 'contexts that do NOT match the new "
			 "state get RemoveMappingContext unconditionally — regardless of bEnable.' Re-applying "
			 "across the whole list is what realises this guarantee: every context is re-evaluated "
			 "against the new state, and the non-matching ones get disabled."),
		OnStateCode.Contains(TEXT("ApplyAllInputContexts"), ESearchCase::CaseSensitive)
			|| OnStateCode.Contains(TEXT("SetAllInputContextsEnabled"), ESearchCase::CaseSensitive));

	// ApplyAllInputContexts is the routine doing the per-context disable for
	// non-matching contexts. Its body must walk every context and compute a
	// per-context bShouldEnable purely from whether the live game state
	// matches that context's state-set — NOT from a cached enabled flag, NOT
	// from a stack lookup. The canonical encoding:
	//
	//   const bool bShouldEnable = !ActiveForStates.IsEmpty()
	//       && ABmrGameState::Get().HasAnyMatchingGameplayTags(ActiveForStates);
	//   SetInputContextEnabled(bShouldEnable, InputContextIt);
	//
	// The bShouldEnable computation is from-state-set → enable/disable for
	// every iteration: a context whose state-set does not include the live
	// game state ends up with bShouldEnable==false, which inside
	// SetInputContextEnabled lands on RemoveMappingContext.
	const FString ApplyAllBody =
		ExtractMemberBody(Source, TEXT("ABmrPlayerController::ApplyAllInputContexts"));
	if (!TestTrue(
			TEXT("B2: BmrPlayerController.cpp must define ApplyAllInputContexts with a body — "
				 "that is the routine OnGameStateChanged delegates to for the per-context "
				 "disable of non-matching contexts."),
			!ApplyAllBody.IsEmpty()))
	{
		return false;
	}

	const FString ApplyAllCode = StripComments(ApplyAllBody);

	TestTrue(
		TEXT("B2: ApplyAllInputContexts must iterate the controller's AllInputContexts list. "
			 "Re-applying through any other discovery path (asking the data asset, walking the "
			 "Enhanced-Input subsystem's currently-applied list) would either miss contexts "
			 "added via AddNewInputContexts at runtime or fail to disable contexts that the "
			 "subsystem has stale entries for."),
		ApplyAllCode.Contains(TEXT("AllInputContexts"), ESearchCase::CaseSensitive));

	const bool bTestsLiveStateAgainstContextSet =
		(ApplyAllCode.Contains(TEXT("HasAnyMatchingGameplayTags"), ESearchCase::CaseSensitive)
			|| ApplyAllCode.Contains(TEXT("HasMatchingGameplayTag"), ESearchCase::CaseSensitive)
			|| ApplyAllCode.Contains(TEXT("HasAnyTag"), ESearchCase::CaseSensitive)
			|| ApplyAllCode.Contains(TEXT("HasAnyExact"), ESearchCase::CaseSensitive)
			|| ApplyAllCode.Contains(TEXT("HasAny("), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B2: ApplyAllInputContexts must compute per-context enable/disable from the LIVE "
			 "game-state tag-set tested against the context's own state-set "
			 "(ABmrGameState::Get().HasAnyMatchingGameplayTags(ActiveForStates) or equivalent). "
			 "Reading a cached 'was enabled' flag and forwarding it unchanged would never "
			 "disable a context that the previous state had enabled — the leak the spec names."),
		bTestsLiveStateAgainstContextSet);

	TestTrue(
		TEXT("B2: ApplyAllInputContexts must dispatch the per-context decision through "
			 "SetInputContextEnabled, the helper that disables (RemoveMappingContext) when its "
			 "bool argument is false. A body that only calls AddMappingContext (or only the "
			 "enable side of a custom helper) would never disable a context that should leave "
			 "the active set on a state transition."),
		ApplyAllCode.Contains(TEXT("SetInputContextEnabled"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B4: BindInputActionsInContext resolves the bind target dynamically
//          by invoking each action's FOnGetterObject delegate; the bound
//          target is the delegate's return value, NOT `this`.
//
// The spec: "Each action carries a target-resolving delegate
// (FunctionPickerTemplate::FOnGetterObject) that returns which UObject should
// receive the input event — evaluated at bind time, not hardcoded." The
// canonical encoding:
//
//   UFunctionPickerTemplate::FOnGetterObject GetOwnerFunc;
//   GetOwnerFunc.BindUFunction(StaticContext.FunctionClass->GetDefaultObject(),
//                              StaticContext.FunctionName);
//   UObject* FoundContextObj = GetOwnerFunc.Execute(GetWorld());
//   InputComponent->BindAction(Action, ETriggerEvent::Triggered, FoundContextObj, FunctionName);
//
// A re-impl that does `InputComponent->BindAction(Action, …, this, FunctionName);`
// works for the one context that happens to route through the controller and
// silently breaks every other one — e.g. a context whose actions route to the
// pawn or the cheat manager fires its delegate but lands on a method the
// controller does not implement.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_BindInputActionsInContextResolvesTargetDynamically,
	"Bomber.PlayerController.BindInputActionsInContextResolvesTargetDynamically",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_BindInputActionsInContextResolvesTargetDynamically::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::BindInputActionsInContext"));
	if (!TestTrue(
			TEXT("B4: BmrPlayerController.cpp must define BindInputActionsInContext with a body — "
				 "the start/ stub is empty; the dynamic-target binding is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Body must walk each action's StaticContext (FFunctionPicker) and bind
	// the FOnGetterObject delegate to the CDO of the function's owning class.
	// This is the spec's "evaluated at bind time, not hardcoded" mechanism.
	TestTrue(
		TEXT("B4: BindInputActionsInContext must read the per-action StaticContext (a "
			 "FFunctionPicker carrying FunctionClass + FunctionName). The picker is how the "
			 "spec routes 'this action goes to *that* receiver': hard-coding the receiver "
			 "loses the per-context routing the picker enables."),
		CodeOnly.Contains(TEXT("StaticContext"), ESearchCase::CaseSensitive));

	const bool bUsesGetterDelegate =
		CodeOnly.Contains(TEXT("FOnGetterObject"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetOwnerFunc"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("FunctionPickerTemplate"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B4: BindInputActionsInContext must use UFunctionPickerTemplate::FOnGetterObject "
			 "(or equivalent typed delegate) to wrap the picker's class+function pair. The spec "
			 "calls this delegate out by name; a re-impl that hand-rolls a target lookup loses "
			 "the picker's CDO-based dispatch and breaks any action whose getter has side effects "
			 "(walking GetWorld() into a subsystem, for example, to return the local pawn)."),
		bUsesGetterDelegate);

	TestTrue(
		TEXT("B4: BindInputActionsInContext must bind the getter delegate via BindUFunction on "
			 "the picker's FunctionClass CDO. That is the canonical 'invoke a UFunction on the "
			 "CDO with the given world' shape; a static_cast bind or a raw function-pointer call "
			 "loses the UObject-graph traversal the getter relies on."),
		CodeOnly.Contains(TEXT("BindUFunction"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("GetDefaultObject"), ESearchCase::CaseSensitive));

	// The getter must be EXECUTED (not just bound) so the returned UObject*
	// can be passed as the target of BindAction. A bound-but-never-executed
	// delegate doesn't produce a target.
	TestTrue(
		TEXT("B4: BindInputActionsInContext must invoke the getter via Execute(...) — binding it "
			 "without executing yields no target. The canonical body calls "
			 "`GetOwnerFunc.Execute(GetWorld())`."),
		CodeOnly.Contains(TEXT(".Execute("), ESearchCase::CaseSensitive));

	// BindAction's target must be the dynamic result, NOT `this`. The spec
	// failure mode: "Static BindAction(this, …) routes to wrong target in
	// context changes." A scan for the `this,` argument pattern inside a
	// BindAction call catches the common error.
	TestTrue(
		TEXT("B4: BindInputActionsInContext must invoke InputComponent->BindAction with the "
			 "resolved dynamic target, not `this`. The canonical encoding passes the value "
			 "returned by the getter delegate (e.g. FoundContextObj)."),
		CodeOnly.Contains(TEXT("BindAction"), ESearchCase::CaseSensitive));

	const bool bBindsStaticThis =
		CodeOnly.Contains(TEXT("BindAction(ActionIt, ETriggerEvent::Triggered, this,"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("BindAction(InputActionIt, ETriggerEvent::Triggered, this,"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("BindAction(Action, ETriggerEvent::Triggered, this,"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("BindAction(InputAction, ETriggerEvent::Triggered, this,"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(", this, FunctionName"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(", this, &ThisClass::"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(", this, &ABmrPlayerController::"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B4: BindInputActionsInContext must NOT pass `this` as the BindAction target. The "
			 "spec calls out the exact failure: 'Static BindAction(this, ...) — routes to wrong "
			 "target in context changes.' The same UBmrInputAction can be re-used across contexts "
			 "whose StaticContext picks different receivers (pawn for gameplay, settings widget "
			 "for in-menu); hard-coding `this` routes every flavour to the controller and the "
			 "wrong handler runs."),
		bBindsStaticThis);

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B5: PawnLeavingGame does NOT call Super.
//
// The spec / evaluator: "PawnLeavingGame is empty — intentionally does NOT
// call Super::PawnLeavingGame() to prevent pool-managed pawn destruction."
// The pawn is owned by an actor-pooling subsystem that reuses it across
// rounds; APlayerController::PawnLeavingGame() destroys the pawn, which
// breaks pool reuse on the next round.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_PawnLeavingGameDoesNotCallSuper,
	"Bomber.PlayerController.PawnLeavingGameDoesNotCallSuper",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_PawnLeavingGameDoesNotCallSuper::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::PawnLeavingGame"));
	if (!TestTrue(
			TEXT("B5: BmrPlayerController.cpp must define a PawnLeavingGame override body — the "
				 "header declares it as an override and the start/ stub provides an empty body; "
				 "a re-impl that deletes the override entirely would fall through to "
				 "APlayerController::PawnLeavingGame and destroy the pool-managed pawn."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The contract is precisely "no Super::PawnLeavingGame()" — the body may
	// still do other cleanup (notify the player state, notify the pawn), but
	// the Super call must not be present.
	const bool bCallsSuperPawnLeaving =
		CodeOnly.Contains(TEXT("Super::PawnLeavingGame"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("APlayerController::PawnLeavingGame"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B5: PawnLeavingGame must NOT call Super::PawnLeavingGame(). The spec calls this "
			 "out as the named failure mode: 'Calling Super::PawnLeavingGame() destroys the "
			 "pool-managed pawn.' The pool subsystem expects the same pawn instance to come "
			 "back when the round-respawn dispatcher picks it back up; Super tears the pawn "
			 "down (UnPossess + MarkAsGarbage on its hierarchy) and the next pool fetch "
			 "returns a stale or null reference."),
		bCallsSuperPawnLeaving);

	// The override must do the cleanup work the suppressed Super would have
	// chained. The canonical body does two things in lieu of Super:
	//   1. Notifies the inherited PlayerState that the player is leaving
	//      (PlayerState->UnregisterPlayerWithSession()) — session-side
	//      bookkeeping that must run WHILE the pawn is still attached.
	//   2. Notifies the possessed pawn of the logout
	//      (GetPawn<ABmrPawn>()->OnPostLogout(this)) — pool-return hook on
	//      the pawn itself.
	//
	// An empty body (the start/ stub) silently drops both notifications:
	// the player state is never told the session ended, the pawn never
	// runs its post-logout cleanup. A re-impl that "just deletes the
	// Super call" but adds nothing else passes the no-Super check
	// vacuously while leaving the session in a half-disconnected state —
	// on the next reconnect, the B6 reuse path lands on a player state
	// that was never told the previous session ended.
	//
	// We accept either of the two notifications as evidence the body is
	// substantive (the spec doesn't pin a single helper name): the body
	// must reference either the inherited PlayerState or the possessed
	// pawn. The two together are the only behaviour surface the override
	// has, given that the Super chain is the named no-go.
	const bool bDoesSubstantiveCleanup =
		CodeOnly.Contains(TEXT("PlayerState"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetPawn"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("InPawn"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("OnPostLogout"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("UnregisterPlayerWithSession"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B5: PawnLeavingGame must do the cleanup the suppressed Super would have chained. "
			 "The override exists precisely because Super would destroy the pool-managed pawn — "
			 "but Super also unwinds session bookkeeping on the player state and the leaving "
			 "pawn. The canonical solution replaces the Super call with two direct "
			 "notifications: PlayerState->UnregisterPlayerWithSession() (session-side cleanup "
			 "before the pawn is released) and GetPawn<ABmrPawn>()->OnPostLogout(this) (pool-"
			 "return book-keeping on the pawn). A body that *just* deletes the Super call and "
			 "does nothing else passes the no-Super check vacuously while leaving the session "
			 "in a half-disconnected state: the player state still thinks it owns a live "
			 "session, the pawn never runs its post-logout hook, and on the next reconnect "
			 "the B6 reuse path lands on a stale session. The body must reference the "
			 "inherited PlayerState (for session unregistration) or the possessed pawn (for "
			 "post-logout notification) — both is ideal, either alone is the minimum that "
			 "distinguishes a substantive override from an empty stub."),
		bDoesSubstantiveCleanup);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B6: InitPlayerState reuses an existing player state on reconnect
//          and short-circuits Super in that path.
//
// The spec: "find any existing player state already attached for this
// controller — if found, call SetPlayerState(Existing) and return. Only fall
// through to Super::InitPlayerState when none exists. This lets a
// reconnecting player resume with their score and loadout intact." The
// canonical encoding (which the api_surface quotes verbatim):
//
//   ABmrPlayerState* InPlayerState = UBmrBlueprintFunctionLibrary::GetLocalPlayerState(this);
//   if (!InPlayerState)
//   {
//       Super::InitPlayerState();
//       return;
//   }
//   // attach existing
//   PlayerState = InPlayerState;
//   PlayerState->SetOwner(this);
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_InitPlayerStateReusesExistingOnReconnect,
	"Bomber.PlayerController.InitPlayerStateReusesExistingOnReconnect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_InitPlayerStateReusesExistingOnReconnect::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::InitPlayerState"));
	if (!TestTrue(
			TEXT("B6: BmrPlayerController.cpp must define InitPlayerState with a body — the "
				 "header declares it as an override and the start/ stub is empty; the "
				 "reconnect-reuse logic is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Must search for an existing player state attached to this controller.
	// The canonical helper is UBmrBlueprintFunctionLibrary::GetLocalPlayerState;
	// any equivalent search that yields the pre-existing ABmrPlayerState (e.g.
	// world iteration that filters by the controller's NetOwningPlayer) is
	// accepted, but the body must reference some lookup before the Super branch.
	const bool bSearchesForExisting =
		CodeOnly.Contains(TEXT("GetLocalPlayerState"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetPlayerState<"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("PlayerState"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B6: InitPlayerState must look up any existing ABmrPlayerState before invoking "
			 "Super. The spec: 'find any existing player state already attached for this "
			 "controller'. The canonical lookup is "
			 "UBmrBlueprintFunctionLibrary::GetLocalPlayerState(this); equivalents that yield "
			 "the same pre-existing state are acceptable, but the body must reference some "
			 "search before deciding whether to call Super."),
		bSearchesForExisting);

	// The reuse path must NOT call Super::InitPlayerState. The spec is
	// explicit: "Only fall through to Super::InitPlayerState when none
	// exists." A body that always calls Super and then overwrites the field
	// double-allocates: Super creates a new ABmrPlayerState (incrementing
	// network-IDs, registering with the GameMode's player list), and the
	// re-assignment leaves the just-spawned state orphaned in memory — the
	// "reconnects lose score/loadout" failure mode.
	const bool bHasEarlyReturnOrConditionalSuper =
		CodeOnly.Contains(TEXT("return"), ESearchCase::CaseSensitive)
		&& (CodeOnly.Contains(TEXT("if (!"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("if(!"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("if (InPlayerState"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("if (Existing"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B6: InitPlayerState must branch on the existence of a player state. The canonical "
			 "shape has an early-return-with-Super on the not-found branch, then the reuse "
			 "fall-through. Unconditionally calling Super::InitPlayerState would always spawn "
			 "a fresh ABmrPlayerState — the spec's named failure mode: 'InitPlayerState always "
			 "spawning new — reconnects lose score/loadout.'"),
		bHasEarlyReturnOrConditionalSuper);

	// The reuse path must wire the existing player state to the controller —
	// either via SetPlayerState(Existing) (the spec's named API) or by direct
	// PlayerState = Existing + SetOwner(this) (the canonical solution shape).
	const bool bAttachesExisting =
		CodeOnly.Contains(TEXT("SetPlayerState"), ESearchCase::CaseSensitive)
		|| (CodeOnly.Contains(TEXT("PlayerState ="), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("SetOwner"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B6: InitPlayerState's reuse path must attach the found player state to this "
			 "controller. The spec names SetPlayerState(Existing); the canonical solution "
			 "assigns the inherited PlayerState field directly and calls SetOwner(this). Either "
			 "shape is acceptable — the test accepts both — but the controller must end up "
			 "owning the existing player state, otherwise the reconnect path is a no-op and "
			 "the controller has no player state at all."),
		bAttachesExisting);

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B7: The controller does NOT contain input-mode (FInputModeGameAndUI /
//          FInputModeGameOnly / SetInputMode) switching. The spec is explicit:
//          'mouse mode lives in UBmrMouseActivityComponent, not the controller.'
//
// A common re-impl wires SetInputMode(FInputModeGameAndUI()) inside the
// controller's OnToggledSettings or BeginPlay — that duplicates work the
// mouse-activity component does as the SOLE owner of cursor visibility and
// input-mode arbitration, and the two writers race on every settings-widget
// open / close.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_NoInputModeSwitchingInController,
	"Bomber.PlayerController.NoInputModeSwitchingInController",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_NoInputModeSwitchingInController::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString CodeOnly = StripComments(Source);

	const bool bSwitchesInputMode =
		CodeOnly.Contains(TEXT("FInputModeGameAndUI"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("FInputModeGameOnly"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("FInputModeUIOnly"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("SetInputMode("), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B7: BmrPlayerController.cpp must NOT switch input mode (no FInputModeGameAndUI, "
			 "FInputModeGameOnly, FInputModeUIOnly, or PC->SetInputMode(...) calls). The spec "
			 "is explicit: 'mouse mode lives in UBmrMouseActivityComponent, not the controller.' "
			 "The mouse-activity component is the sole arbiter of cursor visibility and input "
			 "mode; a second writer inside the controller produces a race on every settings "
			 "toggle (the controller sets one mode, the component reverts it on its next tick)."),
		bSwitchesInputMode);

	// Sanity: the controller must STILL own a mouse-activity component (the
	// component is created in the controller's constructor). The header
	// declares it as a UPROPERTY and the constructor names it; removing the
	// component would break the entire mouse-input path. We don't check the
	// constructor body (it lives in the unstripped portion of the file) but
	// we DO assert the file mentions UBmrMouseActivityComponent so a re-impl
	// that drops it leaves a noticeable break in the controller.
	TestTrue(
		TEXT("B7 (sanity): BmrPlayerController.cpp must continue to reference "
			 "UBmrMouseActivityComponent — the controller owns the component (via "
			 "CreateDefaultSubobject in its constructor) and the component is the legitimate "
			 "owner of mouse-mode logic. A re-impl that drops the reference here cannot be "
			 "passing the spec, because the mouse path runs through the controller-owned "
			 "component."),
		CodeOnly.Contains(TEXT("BmrMouseActivityComponent"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("MouseActivityComponent"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("MouseComponent"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Slate-navigation suppression (evaluator B8): SetUIInputIgnored
//          installs a null navigation config so tab/arrow keys don't steal
//          focus from gameplay input.
//
// The spec: "Install a null navigation config via
// FSlateApplication::Get().SetNavigationConfig(...) so tab/arrow keys don't
// steal focus from gameplay input." The canonical body builds a
// MakeShared<FMyNullNavigationConfig> (a FNullNavigationConfig subclass that
// hard-disables every directional and action navigation key) and feeds it to
// SetNavigationConfig on the slate application.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_SetUIInputIgnoredInstallsNullNavigationConfig,
	"Bomber.PlayerController.SetUIInputIgnoredInstallsNullNavigationConfig",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_SetUIInputIgnoredInstallsNullNavigationConfig::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::SetUIInputIgnored"));
	if (!TestTrue(
			TEXT("B8: BmrPlayerController.cpp must define SetUIInputIgnored with a body — the "
				 "start/ stub is empty; the slate-navigation suppression is what was stripped. "
				 "Without this body running, UMG's default navigation config still consumes "
				 "tab / arrow keys, so widgets steal directional input from the pawn."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B8: SetUIInputIgnored must reach FSlateApplication::Get().SetNavigationConfig(...). "
			 "That is the single entry point that overrides the slate navigation table. A body "
			 "that operates on a per-widget UNavigationConfig wouldn't affect global navigation "
			 "and tab/arrow would still steal focus from gameplay inputs."),
		CodeOnly.Contains(TEXT("SlateApplication"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("SetNavigationConfig"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B8: SetUIInputIgnored must install a FNullNavigationConfig (or a subclass "
			 "thereof) — the engine's no-op navigation config that returns "
			 "EUINavigation::Invalid for every key. Installing the default UNavigationConfig "
			 "would simply re-apply the engine's directional/action bindings, defeating the "
			 "purpose. The canonical solution defines an inline FMyNullNavigationConfig "
			 "subclass and MakeShared<>'s it; we accept either spelling."),
		CodeOnly.Contains(TEXT("NullNavigationConfig"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B8: SetUIInputIgnored must hand the config to SetNavigationConfig as a "
			 "TSharedRef/TSharedPtr (MakeShared / MakeShareable). SetNavigationConfig takes a "
			 "shared reference; a raw pointer or a stack-local would not satisfy the API."),
		CodeOnly.Contains(TEXT("MakeShared"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("MakeShareable"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — BeginPlay must wire the two lifecycle hooks the input pipeline
//          depends on: viewport focus + SetUIInputIgnored.
//
// Without SetUIInputIgnored() being called from BeginPlay, the null nav
// config never gets installed at the right moment — UMG widgets created
// after that point keep stealing tab/arrow keys. Without
// SetAllUserFocusToGameViewport, slate has the editor's main toolbar
// focused on first frame and gameplay input doesn't fire at all.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_BeginPlaySetsViewportFocusAndIgnoresUIInput,
	"Bomber.PlayerController.BeginPlaySetsViewportFocusAndIgnoresUIInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_BeginPlaySetsViewportFocusAndIgnoresUIInput::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	const FString Source = LoadProjectFile(this, PlayerControllerRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerController::BeginPlay"));
	if (!TestTrue(
			TEXT("BeginPlay must have a body — the start/ stub calls Super only; the lifecycle "
				 "wiring (viewport focus + UI-input ignored + data-asset subscription) is what "
				 "was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("BeginPlay must call Super::BeginPlay first. The api_surface pins the canonical "
			 "first line as `Super::BeginPlay();` — skipping Super on a PlayerController would "
			 "leave engine systems (input subsystem startup, viewport linkage) un-initialised."),
		CodeOnly.Contains(TEXT("Super::BeginPlay"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("BeginPlay must invoke SetUIInputIgnored() during the bring-up. The spec: 'install "
			 "a null navigation config so tab/arrow keys don't steal focus from gameplay input.' "
			 "The install only takes effect once SetUIInputIgnored runs; deferring it past "
			 "BeginPlay lets the first frame of UMG widgets eat directional input."),
		CodeOnly.Contains(TEXT("SetUIInputIgnored"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("BeginPlay must give game-viewport focus to the local user "
			 "(FSlateApplication::Get().SetAllUserFocusToGameViewport(...)). Without this, the "
			 "newly-loaded PIE window may inherit focus from the editor toolbar / world outliner, "
			 "and the very first frame of input goes to slate, not to the controller."),
		CodeOnly.Contains(TEXT("SetAllUserFocusToGameViewport"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Runtime UClass surface: the controller's reflected class is
//          registered and the input-management UFUNCTIONs the spec names are
//          present on it with the contracted signatures.
//
// This is a runtime check against the engine's reflection database, not a
// source-text scan. It catches re-impls that rename a method, change a
// return type, or drop a UFUNCTION specifier — none of which a source-text
// scan would flag, but each of which would silently break the Blueprint /
// data-asset call sites the rest of the pipeline depends on.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_RuntimeUClassSurface,
	"Bomber.PlayerController.RuntimeUClassSurface",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_RuntimeUClassSurface::RunTest(const FString& Parameters)
{
	UClass* ControllerClass = ABmrPlayerController::StaticClass();
	if (!TestNotNull(
			TEXT("ABmrPlayerController::StaticClass() must return a registered UClass. If this "
				 "fails the Bomber module is not loaded in the test process and none of the "
				 "runtime checks below are meaningful."),
			ControllerClass))
	{
		return false;
	}

	// The controller must continue to derive from APlayerController — a
	// re-impl that flips the parent class (e.g. to AAIController) would
	// invalidate every Super:: contract the test file just enforced.
	TestTrue(
		TEXT("ABmrPlayerController must derive from APlayerController. The Super:: contracts "
			 "(`Super::BeginPlay`, the deliberate omission of `Super::PawnLeavingGame`, the "
			 "InitPlayerState branch) only make sense if APlayerController is the parent."),
		ControllerClass->IsChildOf(APlayerController::StaticClass()));

	// SetAllInputContextsEnabled UFUNCTION must be reflected — the spec
	// quotes its signature (bool, FBmrGameStateTag) and a re-impl that drops
	// the UFUNCTION specifier would break the BlueprintCallable call sites.
	UFunction* SetAllFn = ControllerClass->FindFunctionByName(FName(TEXT("SetAllInputContextsEnabled")));
	TestNotNull(
		TEXT("B1/B2: ABmrPlayerController must expose SetAllInputContextsEnabled as a reflected "
			 "UFUNCTION — that is the state-driven toggle the spec names. A re-impl that drops "
			 "the UFUNCTION declaration breaks every Blueprint and data-asset call site that "
			 "invokes the toggle by name."),
		SetAllFn);

	// BindInputActionsInContext UFUNCTION must be reflected for the same
	// reason — the data-asset pipeline calls into it.
	UFunction* BindCtxFn = ControllerClass->FindFunctionByName(FName(TEXT("BindInputActionsInContext")));
	TestNotNull(
		TEXT("B4: ABmrPlayerController must expose BindInputActionsInContext as a reflected "
			 "UFUNCTION. The input-context lifecycle calls it from SetInputContextEnabled on "
			 "every enable transition; dropping the UFUNCTION breaks the BlueprintCallable "
			 "wiring even though the C++ call site still resolves."),
		BindCtxFn);

	// SetUIInputIgnored UFUNCTION (BlueprintCallable, named with DisplayName
	// "Set UI Input Ignored") must be reflected — Blueprint code in the
	// menu-load pipeline can invoke it.
	UFunction* SetUiIgnoredFn = ControllerClass->FindFunctionByName(FName(TEXT("SetUIInputIgnored")));
	TestNotNull(
		TEXT("B8: ABmrPlayerController must expose SetUIInputIgnored as a reflected UFUNCTION. "
			 "The header declares it with DisplayName 'Set UI Input Ignored' for Blueprint "
			 "invocation; a re-impl that demotes it to a private C++ helper would break the "
			 "menu-load Blueprint that toggles widget input."),
		SetUiIgnoredFn);

	// The state-changed dispatcher must be reflected as well — it is a
	// BlueprintNativeEvent on the header, so OnGameStateChanged_Implementation
	// is reached through the reflection dispatcher.
	UFunction* OnStateFn = ControllerClass->FindFunctionByName(FName(TEXT("OnGameStateChanged")));
	TestNotNull(
		TEXT("B2: ABmrPlayerController must expose OnGameStateChanged as a reflected UFUNCTION "
			 "(BlueprintNativeEvent). The global-message subsystem dispatches into it via "
			 "ProcessEvent; a re-impl that drops the UFUNCTION leaves the state-changed "
			 "broadcast disconnected and the context re-apply on transition never fires."),
		OnStateFn);

	// MouseComponent UPROPERTY must remain present on the class — the
	// controller owns the mouse-activity component (per the spec's 'mouse "
	// mode lives in UBmrMouseActivityComponent') and the property is how the
	// rest of the code reaches it.
	FProperty* MouseProp = ControllerClass->FindPropertyByName(FName(TEXT("MouseComponent")));
	TestNotNull(
		TEXT("B7 (sanity): ABmrPlayerController must declare the MouseComponent UPROPERTY. "
			 "The controller owns the mouse-activity component as a default subobject; "
			 "removing the property breaks GetMouseActivityComponent / "
			 "GetMouseActivityComponentChecked, and the entire mouse-mode pipeline (which "
			 "the spec says is the LEGITIMATE owner of input-mode switching) becomes "
			 "unreachable."),
		MouseProp);

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — B5 DIRECT runtime: PawnLeavingGame must leave the possessed pawn
//          alive. This is the spec's headline failure-mode for the override —
//          'Calling Super::PawnLeavingGame() destroys the pool-managed pawn.'
//
// Setup: spawn a transient game world (HasAuthority()==true), spawn an
// ABmrPlayerController, spawn a bare APawn, and Possess() the pawn so the
// controller's Pawn field is wired. The default APlayerController::
// PawnLeavingGame() implementation does:
//
//   if (GetPawn() != nullptr) { GetPawn()->Destroy(); SetPawn(nullptr); }
//
// so if the override forwards to Super, the spawned pawn here will end up
// marked-for-destruction (IsActorBeingDestroyed()==true or the weak pointer
// goes stale). The canonical solution body deliberately omits the Super
// call — leaving the pool-managed pawn intact for the actor-pool subsystem
// to hand back on the next round. Both the start/ empty-body stub and the
// canonical solution produce identical behaviour here (pawn survives), so
// this test does NOT fail against start; what it DOES catch is the named
// regression — any re-impl that calls Super::PawnLeavingGame() (the spec's
// listed failure mode) fails this assertion because the pawn is destroyed.
// ---------------------------------------------------------------------------
// Test 10 (runtime PawnLeavingGame pawn-survival) was removed: it called
// Controller->PawnLeavingGame() directly, but the header declares
// PawnLeavingGame() as protected, so the test could not compile against the
// public API surface. B5 ("no Super::PawnLeavingGame()") remains covered by
// Test 4 (source-text inspection), which is the only surface where the
// negative — the *absence* of a Super:: call — is observable from outside
// the controller.
#if 0
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerController_PawnLeavingGameRuntimePawnSurvives,
	"Bomber.PlayerController.PawnLeavingGameRuntimePawnSurvives",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerController_PawnLeavingGameRuntimePawnSurvives::RunTest(const FString& Parameters)
{
	using namespace PlayerControllerTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(
			TEXT("B5 (runtime): a transient game world must be creatable to host the controller "
				 "and its possessed pawn. Without a UWorld there is no way to verify the "
				 "behavioural pawn-destruction property below; abort the test rather than "
				 "report a false-positive."),
			World))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABmrPlayerController* Controller =
		World->SpawnActor<ABmrPlayerController>(ABmrPlayerController::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TestNotNull(
			TEXT("B5 (runtime): ABmrPlayerController must spawn into the transient game world. "
				 "If the class fails to spawn the test cannot exercise PawnLeavingGame."),
			Controller))
	{
		DestroyTestWorld(World);
		return false;
	}

	APawn* Pawn = World->SpawnActor<APawn>(APawn::StaticClass(),
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TestNotNull(
			TEXT("B5 (runtime): a bare APawn must spawn into the transient game world to play "
				 "the role of the pool-managed pawn. The actor-pool subsystem (the real owner "
				 "in production) is not loaded here; a vanilla APawn stands in for the same "
				 "destruction-versus-survival contract."),
			Pawn))
	{
		DestroyTestWorld(World);
		return false;
	}

	// Possess wires Controller->Pawn so the parent's PawnLeavingGame would
	// actually find and Destroy() the pawn on a buggy forward-to-Super path.
	// In a transient game world the controller has authority, so Possess
	// dispatches synchronously without networking.
	Controller->Possess(Pawn);
	if (!TestEqual(
			TEXT("B5 (runtime): the controller must end up with the spawned pawn as its Pawn "
				 "after Possess(). If possession silently fails the destruction assertion below "
				 "would be vacuous — APlayerController::PawnLeavingGame() only destroys when "
				 "GetPawn() returns non-null."),
			Controller->GetPawn(), static_cast<APawn*>(Pawn)))
	{
		DestroyTestWorld(World);
		return false;
	}

	// Snapshot via a weak pointer: if Super::PawnLeavingGame is invoked the
	// pawn is marked for destruction and the weak pointer goes stale on the
	// next GC sweep / immediately for synchronous Destroy().
	TWeakObjectPtr<APawn> PawnWeak(Pawn);

	// --- The system under test.
	Controller->PawnLeavingGame();

	const bool bPawnSurvived =
		PawnWeak.IsValid() && IsValid(Pawn) && !Pawn->IsActorBeingDestroyed();
	TestTrue(
		TEXT("B5 (runtime): PawnLeavingGame must NOT destroy the possessed pawn. The spec's "
			 "named failure-mode: 'Calling Super::PawnLeavingGame() destroys the pool-managed "
			 "pawn.' The default APlayerController::PawnLeavingGame() implementation calls "
			 "GetPawn()->Destroy() — observable here as IsActorBeingDestroyed()==true or the "
			 "weak pointer going stale. The canonical solution body omits the Super call "
			 "precisely so the actor-pool subsystem can hand the same pawn back on the next "
			 "round; any re-impl that forwards to Super breaks pool reuse, and this assertion "
			 "fails."),
		bPawnSurvived);

	DestroyTestWorld(World);
	return true;
}
#endif

// ---------------------------------------------------------------------------
// Test 11 (runtime SetAllInputContextsEnabled iteration) was removed.
//
// Why removed: the only inputs reachable from this headless harness — without
// bootstrapping an Enhanced-Input local-player subsystem — are (a) null
// TObjectPtr entries and (b) UBmrInputMappingContext mocks whose
// ActiveForStates container is empty. Both inputs send the canonical
// solution down the per-context `continue` skip path with NO observable
// side effect on the controller; that is exactly what the empty start/ stub
// produces too, so the test passes vacuously on both impls. Injecting a
// context with a *matching* state-set instead would force the solution down
// the dispatch path into SetInputContextEnabled →
// UInputUtilsLibrary::GetEnhancedInputSubsystem, which ensure-fails on a
// freshly-spawned controller with no local player — failing the solution
// test, not the stub one.
//
// UBmrInputMappingContext is declared `final` and GetActiveForStates() is
// FORCEINLINE, so iteration cannot be observed via a recording mock either.
// B1 / B2 coverage therefore lives in the source-text scans in
// SetAllInputContextsEnabledIteratesFlatListByStateTag (Test 1) and
// NonMatchingContextsRemovedOnStateChange (Test 2), which together pin the
// iteration shape, the tag-membership test, the dispatch through
// SetInputContextEnabled, the absence of a push/pop stack, and the
// non-matching-context disable on state change.
// ---------------------------------------------------------------------------
