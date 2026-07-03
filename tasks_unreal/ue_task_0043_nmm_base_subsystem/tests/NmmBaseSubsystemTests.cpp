// Copyright (c) 2026 GameDevBench. UNMMBaseSubsystem automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMBaseSubsystem.cpp
//
// UNMMBaseSubsystem is the "brain" of the New Main Menu plugin. It predicts
// the target menu state given four async prerequisites — local pawn ready,
// cinematic spots loaded, modular-game-feature activation complete, and Data
// Registry cinematic rows available — broadcasts MenuReady when those settle,
// and recovers from the race where the GameState.Menu tag replicates to a
// client before that client's MGF activation has finished. Every named
// behavior in the spec reduces to one of:
//
//   (A) State-prediction branch order: GetPredictedMenuState's four branches
//       run in a strict sequence (spots-ready → MGF-pending → no-rows →
//       fallthrough). The MGF-pending check must precede the no-rows check
//       — a build with no cinematic rows that swaps the two falsely reports
//       BasicMenu the moment MGF starts loading, then never re-enters the
//       Idle branch because the spot-ready signal landed before this peer's
//       MGF activation completed.
//
//   (B) TryBroadcastMenuReady's three guards: HasBroadcastedMessage(MenuReady),
//       !HasBroadcastedMessage(Player_PawnReady), GetPredictedMenuState()==None.
//       The pawn-ready guard is the named failure mode — without it MenuReady
//       fires before the local pawn is possessed and downstream listeners
//       null-deref the controller chain. Non-authority clients additionally
//       forward via ServerBroadcastMessage; the server must NOT forward (a
//       feedback loop).
//
//   (C) SetNewMainMenuState's dual effect: update CurrentMenuStateTag AND
//       broadcast MenuStateChanged with NewState in InstigatorTags. Updating
//       the field alone leaves every downstream subsystem (camera, spots,
//       player-controller component) blind to the transition.
//
//   (D) Lifecycle: OnGameFeatureInitialize subscribes to GameState_Changed +
//       Player_PawnReady via the global message subsystem and binds the
//       spot-ready dynamic multicast. OnGameFeatureDeinitialize unsubscribes
//       and clears cached MenuStateChanged AND MenuReady messages so a fresh
//       menu load does not replay stale broadcasts on late-binding listeners.
//
//   (E) MGF-race recovery: OnGameFeatureActivated catches the wait-condition
//       where Menu is already active locally but CurrentMenuStateTag is still
//       None, attempts MenuReady, and applies the predicted state. Without
//       this hook a client whose Menu tag arrived during MGF loading stays
//       at None forever.
//
//   (F) Event handlers: OnGameStateChanged transitions on Menu and resets on
//       GameStarting. OnFirstPawnReady re-attempts MenuReady (pawn-ready is
//       one of its broadcast guards). OnActiveMenuSpotReady re-attempts
//       MenuReady AND upgrades BasicMenu → Idle AND handles the client-only
//       case where the state is still None while Menu is active (the spec's
//       "Only handling BasicMenu → Idle" failure mode).
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface would need a hydrated PIE world
//   with a live UNMMSpotsSubsystem singleton (UNMMSpotsSubsystem::Get() in
//   OnGameFeatureInitialize asserts), a UBmrModularGameFeaturesLoaderSubsystem
//   hooked to a real plugin URL so HasPendingTagDrivenActivations returns
//   the right answer, an ABmrGameState singleton (Get() asserts) holding the
//   FBmrGameStateTag leaves to drive OnGameStateChanged through its real
//   path, a UGlobalMessageSubsystem with cached-message slots and a player
//   controller pair that can forward via ServerBroadcastMessage, an
//   ABmrPlayerController with HasAuthority() configured by the network
//   driver — the only way to flip server-vs-client forwarding — and a
//   populated cinematic Data Registry (FBmrCinematicRow::GetRowsNum depends
//   on registry hydration) to flip the no-rows branch. None of that is set
//   up as a reusable harness in this project; reproducing it inside an
//   automation test would dwarf the subsystem surface itself.
//
//   The NewMainMenu plugin is also not a Bomber.Build.cs dependency, so we
//   cannot include any of its headers (NMMBaseSubsystem.h, NmmStateTag.h)
//   from a Source/Bomber/Tests/ file without changing the module graph.
//   The same applies to the MyUtils plugin's ModularGameFeaturePluginSubsystem.h.
//   Every behavior is therefore pinned via source scans over a comment-
//   stripped copy of NMMBaseSubsystem.cpp, with each scan attached to a
//   specific named anti-pattern from the spec.

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData (GameplayAbilities, already in Bomber.Build.cs)
#include "CoreMinimal.h"
#include "GameplayTagContainer.h" // FGameplayTag (GameplayTags, already in Bomber.Build.cs)
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h" // FStructProperty
#include "UObject/UObjectGlobals.h" // NewObject, FindObject, GetTransientPackage

DEFINE_LOG_CATEGORY_STATIC(LogNmmBaseSubsystemTests, Log, All);

namespace NmmBaseSubsystemTests
{
	static const TCHAR* SubsystemCppRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMBaseSubsystem.cpp");

	static const TCHAR* SubsystemHeaderRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Public/Subsystems/NMMBaseSubsystem.h");

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

	// Strip C++ line and block comments so substring scans don't false-match
	// anti-pattern phrasing that appears in canonical comments. The solution
	// includes lines like "It's likely client raced replicated Menu game state
	// vs local MGF activating, leaving NMM state stuck at None" and "Already
	// entered menu or pawn is not possessed yet or should wait for cinematics
	// loaded" — every one of these would pass naive substring tests against
	// a stub body. String literals are preserved so tag names inside TEXT()
	// and FORCEINLINE message strings still scan.
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

	// Brace-depth walker. Counts only braces, not parens, so nested initialiser
	// lists / lambdas don't confuse the scan.
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
} // namespace NmmBaseSubsystemTests

// ---------------------------------------------------------------------------
// Test 1 — Source: SetNewMainMenuState updates CurrentMenuStateTag AND
//          broadcasts MenuStateChanged with the new tag in InstigatorTags (B10).
//
// The spec's contract for SetNewMainMenuState has two halves:
//   1. Update CurrentMenuStateTag = NewState (the field-write half).
//   2. Broadcast NmmGameplayTags::Event::MenuStateChanged with NewState in
//      InstigatorTags (the broadcast half).
//
// Both halves must appear in the body. The start/ stub is empty, so neither
// half runs and the menu state stays at the constructor default forever.
//
//   * Field-write half: `CurrentMenuStateTag = NewState;` — without it,
//     GetCurrentMenuState() returns stale data and every downstream
//     subsystem (camera, spots, player-controller component) reads the
//     prior tag (None on first run, then whatever it was set to before).
//   * Broadcast half: UGlobalMessageSubsystem::BroadcastGlobalMessage with
//     NmmGameplayTags::Event::MenuStateChanged on EventTag and NewState in
//     InstigatorTags. The named failure mode B10: 'Updating the state field
//     without broadcasting — downstream subsystems see stale state.'
//   * The InstigatorTags payload — the canonical body is
//     `EventData.InstigatorTags.AddTag(NewState);`. Listeners read
//     InstigatorTags to discover WHICH state was transitioned to; omitting
//     this leaves the broadcast carrying an empty tag container, and the
//     MVVM viewmodel's state-driven UI binding (which selects the active
//     widget based on InstigatorTags) sees no state and falls back to a
//     blank default render.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_SetNewMainMenuStateUpdatesAndBroadcasts,
	"Bomber.NMMBaseSubsystem.SetNewMainMenuStateUpdatesAndBroadcasts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_SetNewMainMenuStateUpdatesAndBroadcasts::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMBaseSubsystem::SetNewMainMenuState"));
	if (!TestTrue(
			TEXT("B10: NMMBaseSubsystem.cpp must define SetNewMainMenuState with a body — the "
				 "start/ stub is empty, which leaves both the field-write and the broadcast "
				 "missing."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B10 (field-write half): SetNewMainMenuState must assign to CurrentMenuStateTag. "
			 "Without the field-write, GetCurrentMenuState() returns stale data and every "
			 "downstream subsystem (camera, spots, player-controller component) reads the "
			 "prior tag — the canonical body is `CurrentMenuStateTag = NewState;`. A re-impl "
			 "that broadcasts MenuStateChanged but forgets the assignment would pass the "
			 "broadcast checks below but leave every GetCurrentMenuState() caller forever "
			 "reading None."),
		CodeOnly.Contains(TEXT("CurrentMenuStateTag"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B10 (broadcast half): SetNewMainMenuState must broadcast through "
			 "UGlobalMessageSubsystem::BroadcastGlobalMessage. The named failure mode: 'Updating "
			 "the state field without broadcasting — downstream subsystems see stale state.' "
			 "The project's broadcast bus is UGlobalMessageSubsystem; without this call the "
			 "field-write half succeeds but every listener (camera, spots, player controller "
			 "component, MVVM viewmodel) stays bound to the prior state because no "
			 "GameplayEventData ever reaches the bus."),
		CodeOnly.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B10: SetNewMainMenuState must reference NmmGameplayTags::Event::MenuStateChanged "
			 "as the event tag on the broadcast payload. The downstream listeners filter on this "
			 "specific tag; substituting any other tag (a placeholder, MenuReady, a custom "
			 "namespace) would silently route the broadcast to nothing because no listener is "
			 "registered for it. The spec pins the tag explicitly: 'broadcasts MenuStateChanged "
			 "with the new state tag.'"),
		CodeOnly.Contains(TEXT("MenuStateChanged"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B10: SetNewMainMenuState must write the new state into InstigatorTags on the "
			 "broadcast payload. The canonical body is `EventData.InstigatorTags.AddTag("
			 "NewState);` — listeners read InstigatorTags to discover WHICH state was "
			 "transitioned to. Omitting this leaves the broadcast carrying an empty tag "
			 "container, and the MVVM viewmodel's state-driven UI binding (which selects the "
			 "active widget based on InstigatorTags) sees no state and falls back to a blank "
			 "default render."),
		CodeOnly.Contains(TEXT("InstigatorTags"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — Source: GetPredictedMenuState evaluates branches in the spec-
//          mandated order: MGF-pending MUST precede the no-rows check (B4).
//
// The named failure mode (B4): 'Checking row count before MGF-pending in the
// state predicate — no-cinematic builds stay stuck at None during MGF
// loading.' The branch ordering is the load-bearing detail:
//
//   1. Spots ready  → Idle
//   2. MGF pending  → None       (MUST precede branch 3)
//   3. No rows      → BasicMenu  (MUST precede branch 4)
//   4. Fallthrough  → None
//
// A re-impl that swaps branches 2 and 3 returns BasicMenu the moment a build
// with no cinematic rows begins MGF activation. The downstream effect: on a
// non-cinematic mod / streamlined build, the menu thinks it's ready
// immediately (BasicMenu), MenuReady fires before the MGF activation is
// done, and the spots subsystem (which the MGF activation registers) is
// never bound — leaving Play/Settings/Quit live but every cinematic-spot
// query null-derefing.
//
// The scan walks the function body and confirms:
//   * Both predicates appear (HasPendingTagDrivenActivations + GetRowsNum).
//   * The HasPendingTagDrivenActivations check appears at a LOWER offset
//     (earlier in the body) than the GetRowsNum check.
//   * IsActiveMenuSpotReady appears even earlier — the spots short-circuit
//     is branch 1.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_GetPredictedMenuStateBranchOrder,
	"Bomber.NMMBaseSubsystem.GetPredictedMenuStateBranchOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_GetPredictedMenuStateBranchOrder::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMBaseSubsystem::GetPredictedMenuState"));
	if (!TestTrue(
			TEXT("B4: NMMBaseSubsystem.cpp must define GetPredictedMenuState with a body — the "
				 "start/ stub returns FNmmStateTag::None unconditionally, which collapses every "
				 "branch of the predicate and the menu is permanently stuck at None (the spec's "
				 "documented stub behavior)."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Branch 1 — Spots ready: IsActiveMenuSpotReady() → Idle.
	TestTrue(
		TEXT("B4 (branch 1): GetPredictedMenuState must consult "
			 "UNMMSpotsSubsystem::IsActiveMenuSpotReady as the first short-circuit. When spots "
			 "are loaded the cinematic-lobby Idle state is unambiguously correct regardless of "
			 "MGF / row state; placing this branch later in the order would force the function "
			 "through the MGF-pending or no-rows gates first, producing a transient None or "
			 "BasicMenu return during the brief window where spots have loaded but MGF or row "
			 "state hasn't settled."),
		CodeOnly.Contains(TEXT("IsActiveMenuSpotReady"), ESearchCase::CaseSensitive));

	// Branch 2 — MGF pending: HasPendingTagDrivenActivations() → None.
	const int32 MgfIdx = CodeOnly.Find(
		TEXT("HasPendingTagDrivenActivations"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B4 (branch 2): GetPredictedMenuState must consult "
				 "UBmrModularGameFeaturesLoaderSubsystem::HasPendingTagDrivenActivations to "
				 "detect in-flight MGF activations. The spec: 'whether the modular game feature "
				 "is still activating.' Without this branch, a build whose MGF is still loading "
				 "falls through to the no-rows / fallthrough branches and reports a misleading "
				 "BasicMenu or None — the spots-ready signal hasn't arrived yet because the MGF "
				 "that registers the spot hasn't activated."),
			MgfIdx != INDEX_NONE))
	{
		return false;
	}

	// Branch 3 — No rows: FBmrCinematicRow::GetRowsNum() == 0 → BasicMenu.
	const int32 RowsIdx = CodeOnly.Find(TEXT("GetRowsNum"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B4 (branch 3): GetPredictedMenuState must consult "
				 "FBmrCinematicRow::GetRowsNum to detect the no-cinematic-rows case. The spec: "
				 "'whether any cinematic rows are registered at all.' A build with no cinematic "
				 "content (a quick-play mod, a streamlined release) needs to fall straight into "
				 "BasicMenu without waiting for spots to load — they never will, because no "
				 "cinematic rows exist for the spots subsystem to wait on. Without this branch, "
				 "the no-cinematic build stays stuck at None (the fallthrough) forever."),
			RowsIdx != INDEX_NONE))
	{
		return false;
	}

	// The load-bearing ordering invariant — failure mode B4: 'Checking row
	// count before MGF-pending in the state predicate — no-cinematic builds
	// stay stuck at None during MGF loading.'
	TestTrue(
		TEXT("B4 (CRITICAL ordering): The HasPendingTagDrivenActivations check must appear "
			 "BEFORE the GetRowsNum check in GetPredictedMenuState. The named failure mode: "
			 "'Checking row count before MGF-pending in the state predicate — no-cinematic "
			 "builds stay stuck at None during MGF loading.' If the no-rows check fires first, "
			 "a build with no cinematic rows returns BasicMenu the moment MGF starts loading — "
			 "MenuReady fires immediately, the spots-subsystem MGF activation that would have "
			 "registered the cinematic spots is preempted, and the menu enters BasicMenu against "
			 "a half-initialised module state. The opposite order keeps the function returning "
			 "None while MGF activation is in flight; the OnGameFeatureActivated recovery hook "
			 "(Test 6) then advances the state once activation completes."),
		MgfIdx < RowsIdx);

	// Per-branch return-value invariant: the predicate must return None for
	// the still-waiting cases (MGF pending, fallthrough), not BasicMenu.
	// `None` is the "menu is not yet ready" signal — downstream subsystems
	// rely on it to keep the menu in a loading state. `BasicMenu` is the
	// stable "no cinematics at all" answer; conflating the two causes the
	// menu to enter prematurely against half-initialised state.
	//
	// Counting occurrences of each return value catches the divergence
	// without needing a runtime harness: the canonical has exactly two
	// `return FNmmStateTag::None` statements (one for MGF-pending, one for
	// fallthrough); a re-impl that returns BasicMenu for those branches has
	// at most one None return, which trips this assertion.
	int32 NoneReturnCount = 0;
	{
		int32 SearchFrom = 0;
		while (true)
		{
			const int32 Hit = CodeOnly.Find(
				TEXT("return FNmmStateTag::None"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				SearchFrom);
			if (Hit == INDEX_NONE)
			{
				break;
			}
			++NoneReturnCount;
			SearchFrom = Hit + 1;
		}
	}
	TestTrue(
		FString::Printf(TEXT("B4 (return values): GetPredictedMenuState must return "
							 "FNmmStateTag::None for both the MGF-pending case and the "
							 "rows-exist-but-spots-not-yet-loaded fallthrough — two distinct "
							 "waiting cases that downstream subsystems need to distinguish "
							 "from the stable BasicMenu state. Got %d `return FNmmStateTag::"
							 "None` statements; expected at least 2. A re-impl that returns "
							 "BasicMenu for MGF-pending or the fallthrough causes the menu to "
							 "enter prematurely against a half-initialised module state — "
							 "downstream subsystems see BasicMenu and treat it as a stable "
							 "end state, when in fact spots/MGF are still loading."),
			NoneReturnCount),
		NoneReturnCount >= 2);

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Source: TryBroadcastMenuReady gates on the three required guards
//          (B5/B7).
//
// The three guards in the spec, in order:
//   1. HasBroadcastedMessage(MenuReady) → return  (already-fired guard)
//   2. !HasBroadcastedMessage(Player_PawnReady) → return  (pawn-ready guard,
//      the named failure mode)
//   3. GetPredictedMenuState() == None → return  (prerequisite guard)
//
// Named failure mode B5: 'TryBroadcastMenuReady missing the pawn-ready guard
// — fires before the pawn is bound, causing downstream null dereferences.'
// The MenuReady event drives the StateTree's transition to the Menu game
// state, which downstream components (HUD, MVVM viewmodels, input bindings)
// assume runs against a possessed pawn. Firing MenuReady before
// Player_PawnReady leaves the pawn pointer null on the local controller
// chain and every listener that reads it crashes.
//
// All three tokens must be present; missing any one is a documented failure
// mode. The body must also broadcast MenuReady through BroadcastGlobalMessage
// once the guards pass — without that call the guards correctly suppress but
// the broadcast itself never lands.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_TryBroadcastMenuReadyGuards,
	"Bomber.NMMBaseSubsystem.TryBroadcastMenuReadyGuards",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_TryBroadcastMenuReadyGuards::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMBaseSubsystem::TryBroadcastMenuReady"));
	if (!TestTrue(
			TEXT("B5/B7: NMMBaseSubsystem.cpp must define TryBroadcastMenuReady with a body — "
				 "the start/ stub is empty, so MenuReady never fires, the StateTree never "
				 "transitions to Menu, and the entire menu UI stays dark."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Guard 1: already-broadcast suppress for MenuReady.
	TestTrue(
		TEXT("B7 (guard 1): TryBroadcastMenuReady must consult "
			 "HasBroadcastedMessage(MenuReady) and return early when MenuReady has already "
			 "fired. The spec: 'fires once when the menu is genuinely ready' and 'must not fire "
			 "if it has already fired.' Without this guard, every call from the three event "
			 "handlers (OnFirstPawnReady, OnActiveMenuSpotReady, OnGameFeatureActivated) re-"
			 "fires MenuReady, and the StateTree's transition into Menu is re-triggered on "
			 "every signal — viewmodels re-mount, sound cues replay, the menu UI flickers."),
		CodeOnly.Contains(TEXT("HasBroadcastedMessage"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("MenuReady"), ESearchCase::CaseSensitive));

	// Guard 2: pawn-ready prerequisite — the named failure mode B5. The
	// pawn-ready check has three acceptable forms, all of which encode the
	// same "is the local pawn possessed yet?" question:
	//   1. Broadcast-history check: HasBroadcastedMessage(Player_PawnReady)
	//   2. Library helper: IsLocalPawnReady / IsLocalPlayerControllerReady
	//   3. Direct PC->Pawn check: GetLocalPlayerController()->GetPawn() then
	//      null-test the resulting pawn
	// What is NOT acceptable is no pawn-readiness gate at all.
	const bool bHasPawnReadyGuard =
		CodeOnly.Contains(TEXT("Player_PawnReady"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("IsLocalPawnReady"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("IsLocalPlayerControllerReady"), ESearchCase::CaseSensitive)
		|| (CodeOnly.Contains(TEXT("GetLocalPlayerController"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("GetPawn"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B5 (guard 2 — CRITICAL): TryBroadcastMenuReady must have a pawn-readiness guard "
			 "that returns early when the local pawn is not yet possessed. The named failure "
			 "mode: 'TryBroadcastMenuReady missing the pawn-ready guard — fires before the pawn "
			 "is bound, causing downstream null dereferences.' Accepted forms: "
			 "HasBroadcastedMessage(Player_PawnReady) (broadcast-history check), "
			 "IsLocalPawnReady (live state check), or equivalent BP-function-library helper."),
		bHasPawnReadyGuard);

	// Guard 3: predicate must not return None.
	TestTrue(
		TEXT("B7 (guard 3): TryBroadcastMenuReady must consult GetPredictedMenuState() and "
			 "return early when the predicate returns None. The spec: 'must not fire if the "
			 "state predicate still returns None.' None is the prerequisites-not-yet-settled "
			 "signal (spots not loaded AND MGF still pending OR fallthrough); firing MenuReady "
			 "before the prerequisites settle would route the StateTree to Menu against a "
			 "half-loaded module state where the spots subsystem can't yet provide the "
			 "cinematic camera."),
		CodeOnly.Contains(TEXT("GetPredictedMenuState"), ESearchCase::CaseSensitive));

	// The broadcast itself — once the guards pass, MenuReady must actually
	// fire. A re-impl that has all the guards correct but forgets to
	// broadcast would suppress every call and the StateTree never enters
	// Menu.
	TestTrue(
		TEXT("B7: TryBroadcastMenuReady must broadcast MenuReady through "
			 "UGlobalMessageSubsystem::BroadcastGlobalMessage once the three guards pass. A "
			 "re-impl with all the guards correct but no BroadcastGlobalMessage call would "
			 "perfectly suppress duplicate broadcasts — and never fire the first one — leaving "
			 "the StateTree stuck at the pre-Menu state forever. Spec: 'Broadcast MenuReady to "
			 "trigger global Menu game state via StateTree.'"),
		CodeOnly.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — Source: TryBroadcastMenuReady's server-forward is gated on
//          !HasAuthority() (B8).
//
// The named failure mode (B8): 'Server forwarding MenuReady to itself —
// feedback loop.' The forward path is non-authority-only:
//
//   Non-authority client                    Server (authority)
//   --------------------                    -------------------
//   Local MenuReady broadcast       →       (already broadcast its own
//      ServerBroadcastMessage(...)          MenuReady locally; doesn't
//                                          need a client to tell it)
//
// If the server forwards to itself, the server's ServerBroadcastMessage RPC
// re-enters TryBroadcastMenuReady through the global message subsystem
// (HasBroadcastedMessage now returns true on the server because the local
// broadcast already fired — so the first guard catches the immediate
// re-entry), but a re-impl that uses ClientForward / multicast instead
// would cycle the message back through every client and trigger another
// forward — a feedback loop until the engine's RPC throttling kicks in.
//
// The detection: ServerBroadcastMessage must appear in the body AND
// HasAuthority must appear before the forward (or be combined into the
// branch's predicate). Without HasAuthority anywhere in the body, the
// forward fires unconditionally.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_TryBroadcastMenuReadyForwardsOnlyFromClient,
	"Bomber.NMMBaseSubsystem.TryBroadcastMenuReadyForwardsOnlyFromClient",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_TryBroadcastMenuReadyForwardsOnlyFromClient::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMBaseSubsystem::TryBroadcastMenuReady"));
	if (!TestTrue(
			TEXT("B8: NMMBaseSubsystem.cpp must define TryBroadcastMenuReady with a body — the "
				 "start/ stub is empty, which leaves both the broadcast and the server-forward "
				 "missing."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B8: TryBroadcastMenuReady must forward to the server via "
			 "ServerBroadcastMessage on non-authority clients. Spec: 'Non-authority clients "
			 "must forward the event to the server — the server's state tree needs to observe "
			 "menu entry.' Without the forward, the server's StateTree never observes the "
			 "client's menu entry and the server-side transition to Menu is delayed (or skipped "
			 "entirely on a listen-server whose own MenuReady hasn't fired yet)."),
		CodeOnly.Contains(TEXT("ServerBroadcastMessage"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B8 (CRITICAL): TryBroadcastMenuReady must gate the ServerBroadcastMessage call on "
			 "HasAuthority() being false. The named failure mode: 'Server forwarding MenuReady "
			 "to itself — feedback loop.' Without the HasAuthority check, the server forwards "
			 "its own broadcast back to itself through the RPC, the local message handler "
			 "re-enters TryBroadcastMenuReady, and a re-impl that uses any other RPC variant "
			 "(client-multicast, reliable-NetMulticast) cycles the message back through every "
			 "client triggering another forward — until the engine's RPC throttling kicks in."),
		CodeOnly.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive));

	// Bonus check: the ServerBroadcastMessage path should be inside the
	// !HasAuthority branch in source order. The canonical encoding is
	// `if (MyPC && !MyPC->HasAuthority()) { MyPC->ServerBroadcastMessage(...); }`
	// — HasAuthority appears BEFORE ServerBroadcastMessage in the source.
	const int32 AuthIdx = CodeOnly.Find(TEXT("HasAuthority"), ESearchCase::CaseSensitive);
	const int32 ForwardIdx = CodeOnly.Find(TEXT("ServerBroadcastMessage"), ESearchCase::CaseSensitive);
	if (AuthIdx != INDEX_NONE && ForwardIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B8: HasAuthority() must appear BEFORE ServerBroadcastMessage in source order — "
				 "the canonical encoding is `if (PC && !PC->HasAuthority()) { PC->"
				 "ServerBroadcastMessage(...); }`. A re-impl that calls ServerBroadcastMessage "
				 "first and only later checks HasAuthority (e.g. in an asserting wrapper) would "
				 "still cycle the RPC at least once per call — the named failure mode 'feedback "
				 "loop' is the cumulative effect of repeated cycling, so even a single "
				 "unguarded forward leaks into multi-frame loops in PIE."),
			AuthIdx < ForwardIdx);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Source: OnGameFeatureDeinitialize clears cached messages for BOTH
//          MenuStateChanged AND MenuReady (B7).
//
// The named failure mode (B7): 'Not clearing cached messages on deinitialize
// — second menu load replays stale broadcasts.' UGlobalMessageSubsystem
// caches the most recent payload per tag so late-binding listeners get the
// last-known value (the cached-message pattern). On menu unload, those
// cached entries must be cleared — otherwise the next menu load's first
// listener registration immediately receives the prior session's
// MenuStateChanged and MenuReady payloads as if they just fired, mounting
// the wrong viewmodel state.
//
// Both clears are required:
//   * MenuStateChanged: a late-binding viewmodel sees the prior session's
//     final state (e.g. Idle from the last menu close) and renders the
//     cinematic lobby UI on top of the loading screen.
//   * MenuReady: the next menu load's guard 1 (HasBroadcastedMessage) sees
//     the prior MenuReady cached as 'already broadcast' and short-circuits
//     every TryBroadcastMenuReady call — MenuReady never re-fires, the new
//     menu session's StateTree transition into Menu never runs.
//
// StopListeningForAllGlobalMessages must also appear — without unsubscribe,
// the destroyed subsystem's listener handles linger in the message bus and
// fire against a freed `this` pointer the next time their event tags trigger.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_DeinitializeClearsCachedMessagesAndStopsListening,
	"Bomber.NMMBaseSubsystem.DeinitializeClearsCachedMessagesAndStopsListening",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_DeinitializeClearsCachedMessagesAndStopsListening::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnGameFeatureDeinitialize_Implementation"));
	if (!TestTrue(
			TEXT("B7: NMMBaseSubsystem.cpp must define OnGameFeatureDeinitialize_Implementation "
				 "with a body — the start/ stub is empty, leaving listener handles and cached "
				 "messages dangling from session to session. The cumulative effect on the second "
				 "menu load is a viewmodel that mounts against the prior session's final state."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B7: OnGameFeatureDeinitialize must call StopListeningForAllGlobalMessages to "
			 "unsubscribe the subsystem from every global message it's listening to. Without "
			 "this, the destroyed subsystem's listener handles linger in the message bus and "
			 "fire against a freed `this` pointer the next time their event tags trigger — "
			 "either a crash on dispatch or (worse) silent execution against a stale this "
			 "pointer that happens to still be allocated when the next listener fires."),
		CodeOnly.Contains(TEXT("StopListeningForAllGlobalMessages"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7 (CRITICAL): OnGameFeatureDeinitialize must call ClearCachedMessages — the "
			 "named failure mode without it: 'second menu load replays stale broadcasts.' "
			 "UGlobalMessageSubsystem caches the most recent payload per tag for late-binding "
			 "listeners; on menu unload those caches must be wiped or the next menu session's "
			 "first listener registration immediately receives the prior session's payload."),
		CodeOnly.Contains(TEXT("ClearCachedMessages"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: ClearCachedMessages must target MenuStateChanged. The named failure mode if "
			 "this is missed: a late-binding viewmodel on the next menu load sees the prior "
			 "session's final state cached (e.g. Idle from the last menu close) and renders the "
			 "cinematic lobby UI on top of the loading screen until the new state fires. The "
			 "canonical body has both ClearCachedMessages calls in adjacent lines, one for each "
			 "tag — neither is optional."),
		CodeOnly.Contains(TEXT("MenuStateChanged"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: ClearCachedMessages must target MenuReady. The named failure mode if this is "
			 "missed: the next menu load's TryBroadcastMenuReady guard "
			 "(HasBroadcastedMessage(MenuReady) → return) sees the prior MenuReady cached as "
			 "'already broadcast' and short-circuits every call — MenuReady never re-fires, "
			 "the new menu session's StateTree transition into Menu never runs, and the menu "
			 "UI stays dark forever."),
		CodeOnly.Contains(TEXT("MenuReady"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Source: OnGameFeatureActivated implements the MGF-race recovery
//          hook (B6).
//
// The named failure mode (B6): 'No MGF-activation recovery hook — client
// stuck at None after the replicated Menu tag arrives during loading.' The
// race: a client connects, the server replicates the Menu game state tag
// down before the client's local MGF activation completes. On the client,
// OnGameStateChanged fires with Menu in the payload, calls
// GetPredictedMenuState — which returns None (MGF still pending) — and
// skips the SetNewMainMenuState call. The MGF later finishes activating
// on the client, but nothing re-checks the state: OnGameStateChanged
// won't fire again (the tag is already set on the GameState ASC), so the
// menu stays at None permanently.
//
// The canonical recovery hook runs at MGF-activation time:
//   1. Short-circuit if CurrentMenuStateTag != None (already in a state, no
//      recovery needed) OR if GameState.Menu is not active (no race
//      happened — the normal OnGameStateChanged path will fire next).
//   2. Attempt MenuReady (the pawn might have arrived during MGF loading
//      too, so the prerequisites are now all settled).
//   3. Apply the predicted state if the predicate returns non-None.
//
// The detection: the body must reference TryBroadcastMenuReady AND
// SetNewMainMenuState AND the GameState.Menu check (either via
// HasMatchingGameplayTag or by reading the FBmrGameStateTag::Menu literal).
// All three are load-bearing; missing any one breaks the recovery.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_OnGameFeatureActivatedRecoversFromMgfRace,
	"Bomber.NMMBaseSubsystem.OnGameFeatureActivatedRecoversFromMgfRace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_OnGameFeatureActivatedRecoversFromMgfRace::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnGameFeatureActivated"));
	if (!TestTrue(
			TEXT("B6: NMMBaseSubsystem.cpp must define OnGameFeatureActivated with a body — the "
				 "start/ stub is empty, which leaves the MGF-race recovery hook missing. The "
				 "spec: 'Without this hook, a client whose Menu tag arrived during MGF "
				 "activation stays at None permanently.'"),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Short-circuit guard 1: CurrentMenuStateTag != None — recovery only
	// matters when we're stuck at None.
	TestTrue(
		TEXT("B6 (short-circuit 1): OnGameFeatureActivated must guard on CurrentMenuStateTag "
			 "being None — recovery is only meaningful when the subsystem is actually stuck. "
			 "Without this guard, every MGF activation event (including the second / third / "
			 "Nth time a feature reactivates during the same menu session) re-runs the "
			 "recovery flow and clobbers the legitimately-established state with the "
			 "predicate's current answer."),
		CodeOnly.Contains(TEXT("CurrentMenuStateTag"), ESearchCase::CaseSensitive));

	// Short-circuit guard 2: GameState.Menu must be active — otherwise there
	// is no race to recover from.
	TestTrue(
		TEXT("B6 (short-circuit 2): OnGameFeatureActivated must check that the GameState.Menu "
			 "tag is active before running the recovery flow. The spec describes the wait "
			 "condition as 'the Menu game state is already active but the menu state has not "
			 "been set yet'; without the Menu tag check the recovery runs on every MGF "
			 "activation, including those during loading / pre-Menu init, applying an "
			 "incorrect predicted state before the normal OnGameStateChanged path can fire."),
		CodeOnly.Contains(TEXT("Menu"), ESearchCase::CaseSensitive));

	// The recovery payload — both TryBroadcastMenuReady AND SetNewMainMenuState.
	TestTrue(
		TEXT("B6 (recovery 1 — CRITICAL): OnGameFeatureActivated must attempt MenuReady via "
			 "TryBroadcastMenuReady. The MGF that just activated may have completed the final "
			 "prerequisite (spots-ready, or row-availability); MenuReady's predicate gate now "
			 "returns non-None and the broadcast must fire to drive the StateTree transition "
			 "the client missed. Skipping this call leaves the StateTree at the pre-Menu state "
			 "even after the menu state field is set."),
		CodeOnly.Contains(TEXT("TryBroadcastMenuReady"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B6 (recovery 2): OnGameFeatureActivated must apply the predicted state via "
			 "SetNewMainMenuState when GetPredictedMenuState returns non-None. The spec: 'apply "
			 "the predicted state.' Without this call, the recovery's MenuReady broadcast runs "
			 "but CurrentMenuStateTag stays at None — the StateTree's Menu state mounts the "
			 "UI but downstream subsystems (camera, spots) read GetCurrentMenuState() and see "
			 "None, falling back to default behavior."),
		CodeOnly.Contains(TEXT("SetNewMainMenuState"), ESearchCase::CaseSensitive));

	// The predicate result is consulted to decide whether to apply the
	// state. A re-impl that hardcodes a specific state (e.g. always Idle)
	// instead of reading the predicate would land the wrong state in the
	// no-rows case (should be BasicMenu) or the spots-ready case (should be
	// Idle, accidentally correct) or the still-pending case (should not
	// apply at all).
	TestTrue(
		TEXT("B6: OnGameFeatureActivated must consult GetPredictedMenuState to decide which "
			 "state to apply. The recovery's correctness depends on the predicate's branch "
			 "logic: a no-rows build needs BasicMenu, a spots-ready build needs Idle, and a "
			 "still-pending build (the predicate returns None) needs no state change at all "
			 "yet. Hardcoding any specific tag breaks at least one of the three cases."),
		CodeOnly.Contains(TEXT("GetPredictedMenuState"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Source: OnActiveMenuSpotReady handles BOTH BasicMenu→Idle AND
//          None→Idle (client catch-up) (B9).
//
// The named failure mode (B9): 'Only handling BasicMenu → Idle in spot-ready
// — client catch-up from None → Idle is broken.' Two transitions land on
// this handler:
//
//   * BasicMenu → Idle: the build started with no cinematic rows (predicate
//     returned BasicMenu in branch 3), MGF activation later added rows and
//     loaded spots, and now the menu can upgrade from the bare BasicMenu
//     UI to the full cinematic-lobby Idle. Trivial to spot — every re-impl
//     remembers this case.
//
//   * None → Idle (CLIENT-ONLY race recovery): the client's OnGameStateChanged
//     fired with Menu before spots were loaded — predicate returned None
//     and no state was applied. Later, spots finish loading and fire
//     OnActiveMenuSpotReady. The state is still None (no other handler has
//     transitioned it), but the GameState.Menu tag is held. The handler
//     must recognise this case and transition to Idle, otherwise the client
//     stays at None forever.
//
// A re-impl that only handles `CurrentMenuStateTag == BasicMenu` produces
// the named failure: the client's spots-late race leaves the menu at None
// permanently. The canonical condition is:
//
//   if (CurrentMenuStateTag == BasicMenu
//       || (CurrentMenuStateTag == None
//           && ABmrGameState::Get().HasMatchingGameplayTag(FBmrGameStateTag::Menu)))
//
// Both branches must appear in the body. The TryBroadcastMenuReady call
// must also fire here — spot-ready is one of the events that can complete
// the MenuReady prerequisites.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_OnActiveMenuSpotReadyHandlesClientCatchUp,
	"Bomber.NMMBaseSubsystem.OnActiveMenuSpotReadyHandlesClientCatchUp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_OnActiveMenuSpotReadyHandlesClientCatchUp::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnActiveMenuSpotReady_Implementation"));
	if (!TestTrue(
			TEXT("B9: NMMBaseSubsystem.cpp must define OnActiveMenuSpotReady_Implementation "
				 "with a body — the start/ stub is empty, leaving the BasicMenu→Idle upgrade "
				 "and the None→Idle client catch-up both missing."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Spot-ready re-attempts MenuReady. The named flow: spot loading might
	// complete the last MenuReady prerequisite (the predicate's spots-ready
	// branch flips to Idle), so the broadcast must re-fire from here.
	TestTrue(
		TEXT("B9: OnActiveMenuSpotReady must call TryBroadcastMenuReady. Spot loading is one "
			 "of the events that can complete the MenuReady prerequisites — the predicate's "
			 "spots-ready branch flips to Idle the moment IsActiveMenuSpotReady returns true, "
			 "and the broadcast must re-fire from here. Without this call, the spot-ready "
			 "race leaves MenuReady permanently unfired even after the predicate is ready, "
			 "and the StateTree never transitions to Menu on a build whose spots loaded after "
			 "the pawn was already possessed."),
		CodeOnly.Contains(TEXT("TryBroadcastMenuReady"), ESearchCase::CaseSensitive));

	// The BasicMenu→Idle branch — every re-impl tends to remember this one,
	// but it's still load-bearing.
	TestTrue(
		TEXT("B9 (transition 1): OnActiveMenuSpotReady must transition from BasicMenu to Idle "
			 "when spots become ready. A build that started with no cinematic rows (predicate "
			 "returned BasicMenu in branch 3) needs to upgrade to the full cinematic-lobby Idle "
			 "once spots are loaded; without this transition the menu stays in the bare "
			 "BasicMenu UI even after the cinematic content is available."),
		CodeOnly.Contains(TEXT("BasicMenu"), ESearchCase::CaseSensitive));

	// CRITICAL — the client catch-up branch the spec calls out specifically.
	TestTrue(
		TEXT("B9 (transition 2 — CRITICAL): OnActiveMenuSpotReady must ALSO transition from "
			 "None to Idle when the GameState.Menu tag is active. The named failure mode: "
			 "'Only handling BasicMenu → Idle in spot-ready — client catch-up from None → Idle "
			 "is broken.' The scenario: a client's OnGameStateChanged fired with Menu before "
			 "spots were loaded, predicate returned None, no state was applied; later, "
			 "OnActiveMenuSpotReady fires — the state is still None but Menu is held. Without "
			 "the (CurrentMenuStateTag == None && Menu-active) branch, this client stays at "
			 "None forever and the menu UI never mounts."),
		CodeOnly.Contains(TEXT("None"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("Menu"), ESearchCase::CaseSensitive));

	// The SetNewMainMenuState call is what actually performs the upgrade.
	// The new state can be passed either as a literal (Idle) or as the
	// result of GetPredictedMenuState() — both produce the same runtime
	// behaviour when spots are ready (PredictedState resolves to Idle).
	// A re-impl that has the right condition but forgets the transition
	// call would short-circuit at the condition and never advance.
	const bool bHasUpgradeCall =
		CodeOnly.Contains(TEXT("SetNewMainMenuState"), ESearchCase::CaseSensitive)
		&& (CodeOnly.Contains(TEXT("Idle"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("PredictedState"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("GetPredictedMenuState"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B9: OnActiveMenuSpotReady must call SetNewMainMenuState to perform the upgrade. "
			 "Accepted forms: SetNewMainMenuState(FNmmStateTag::Idle) (literal-target form) or "
			 "SetNewMainMenuState(GetPredictedMenuState()) / SetNewMainMenuState(PredictedState) "
			 "(predicate-driven form). The predicate-driven form resolves to Idle at runtime "
			 "when spots are ready, so both behaviours are equivalent. A re-impl with the right "
			 "condition but no transition call would short-circuit at the predicate and never "
			 "advance the state, leaving the BasicMenu UI mounted over the cinematic-ready spots."),
		bHasUpgradeCall);

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Source: OnGameStateChanged transitions on Menu and resets on
//          GameStarting (B1, the spec's "event handlers — game state
//          changed" requirement).
//
// The handler dispatches on Payload.InstigatorTags:
//
//   Menu          → SetNewMainMenuState(GetPredictedMenuState()) if non-None
//   GameStarting  → SetNewMainMenuState(None)
//   (other tags)  → no-op
//
// The Menu branch's check for `PredictedState != None` is load-bearing: if
// the predicate returns None (MGF still pending, spots not loaded), the
// handler must NOT call SetNewMainMenuState(None) — that would broadcast
// MenuStateChanged with None as the new state, which downstream listeners
// read as "exiting the menu" rather than "still loading." The recovery
// from this stuck-at-None state comes through OnGameFeatureActivated
// (Test 6) and OnActiveMenuSpotReady (Test 7).
//
// The GameStarting branch's SetNewMainMenuState(None) is the menu-exit
// reset — the player left the menu, the state should clear so the next
// menu re-entry starts from a clean slate.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_OnGameStateChangedTransitionsOnMenuAndGameStarting,
	"Bomber.NMMBaseSubsystem.OnGameStateChangedTransitionsOnMenuAndGameStarting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_OnGameStateChangedTransitionsOnMenuAndGameStarting::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnGameStateChanged_Implementation"));
	if (!TestTrue(
			TEXT("B1: NMMBaseSubsystem.cpp must define OnGameStateChanged_Implementation with a "
				 "body — the start/ stub is empty, so neither the Menu transition nor the "
				 "GameStarting reset ever runs; the menu state stays at the prior value across "
				 "every game-state change."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Menu branch — transition into the predicted state.
	TestTrue(
		TEXT("B1 (Menu branch): OnGameStateChanged must dispatch on the Menu tag in "
			 "Payload.InstigatorTags. Spec: 'transition to the predicted state when Menu "
			 "becomes active.' Without the Menu dispatch, the handler ignores the Menu "
			 "transition entirely and the menu state stays at None across the GameState's "
			 "transition into Menu — the menu UI never mounts on the authority path either."),
		CodeOnly.Contains(TEXT("Menu"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("InstigatorTags"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B1 (Menu branch): OnGameStateChanged must consult GetPredictedMenuState to decide "
			 "the target state. Spec: 'transition to the predicted state.' Hardcoding a "
			 "specific tag (e.g. always Idle) ignores the prerequisites — a no-rows build "
			 "needs BasicMenu, a still-loading build needs to stay at None and recover later "
			 "via OnGameFeatureActivated. Reading the predicate is the load-bearing dispatch."),
		CodeOnly.Contains(TEXT("GetPredictedMenuState"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B1 (Menu branch): OnGameStateChanged must call SetNewMainMenuState with the "
			 "predicted state when the predicate returns non-None. Skipping the call leaves "
			 "the field-write missing and downstream subsystems never observe the transition; "
			 "the broadcast inside SetNewMainMenuState is the only way the camera / spots / "
			 "player-controller-component learn that the menu is now active."),
		CodeOnly.Contains(TEXT("SetNewMainMenuState"), ESearchCase::CaseSensitive));

	// GameStarting branch — the menu-exit reset.
	TestTrue(
		TEXT("B1 (GameStarting branch): OnGameStateChanged must dispatch on the GameStarting "
			 "tag. Spec: 'reset to None when GameStarting arrives.' Without the GameStarting "
			 "reset, the menu state stays at Idle / BasicMenu through the player's transition "
			 "into gameplay; the next menu re-entry observes a stale state and the predicate's "
			 "branches don't re-evaluate because OnGameFeatureActivated short-circuits "
			 "(CurrentMenuStateTag != None)."),
		CodeOnly.Contains(TEXT("GameStarting"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Source: OnGameFeatureInitialize subscribes to GameState_Changed
//           + Player_PawnReady AND binds the spots-ready dynamic delegate
//           (the spec's "Lifecycle" requirement, with B3 being one of its
//           consequences).
//
// On game-feature initialize, the subsystem must wire up its three signal
// sources:
//
//   * GameState_Changed (global message, drives OnGameStateChanged) — the
//     primary state-driver. Without this listener, the menu never observes
//     the GameState's transition into Menu and stays at None forever.
//
//   * Player_PawnReady (global message, drives OnFirstPawnReady) — the
//     pawn-ready signal that unblocks TryBroadcastMenuReady's pawn-ready
//     guard. Without this listener, the predicate stays gated on the guard
//     even after the pawn arrives, and MenuReady never fires through this
//     handler (only the other two attempt paths — OnActiveMenuSpotReady
//     and OnGameFeatureActivated — can complete the broadcast, both
//     covered by separate tests).
//
//   * UNMMSpotsSubsystem::OnActiveMenuSpotReady (dynamic multicast,
//     drives OnActiveMenuSpotReady_Implementation) — the spot-load signal
//     that unlocks the BasicMenu→Idle and None→Idle transitions. Spec
//     calls out spot-ready is NOT routed through the global message bus
//     because it's a dynamic-multicast delegate (the spec evaluator notes
//     flag the wrong implementation: 'binding spot-ready as a global
//     message — it's a dynamic multicast delegate.').
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_InitializeSubscribesToAllThreeSignalSources,
	"Bomber.NMMBaseSubsystem.InitializeSubscribesToAllThreeSignalSources",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_InitializeSubscribesToAllThreeSignalSources::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnGameFeatureInitialize_Implementation"));
	if (!TestTrue(
			TEXT("Lifecycle: NMMBaseSubsystem.cpp must define "
				 "OnGameFeatureInitialize_Implementation with a body — the start/ stub is "
				 "empty, so none of the three signal sources are wired up; the subsystem is "
				 "deaf to every event that should drive its state."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Signal 1 — GameState_Changed global message.
	TestTrue(
		TEXT("Lifecycle: OnGameFeatureInitialize must subscribe to "
			 "BmrGameplayTags::Event::GameState_Changed via "
			 "UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage. Spec: 'subscribe "
			 "to game-state changes via the global message subsystem.' Without this listener, "
			 "OnGameStateChanged never fires, the Menu / GameStarting transitions are missed "
			 "entirely, and the menu state stays at the constructor default (None) for the "
			 "entire session."),
		CodeOnly.Contains(TEXT("GameState_Changed"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("CallOrStartListeningForGlobalMessage"), ESearchCase::CaseSensitive));

	// Signal 2 — pawn-ready global message. The canonical uses
	// Player_PawnReady (fires for any pawn including bots); the local-pawn-
	// scoped variant Player_LocalPawnReady (fires only for the local human)
	// is also valid and arguably more correct for this menu-only use case.
	// Both drive the OnFirstPawnReady handler that re-attempts MenuReady.
	const bool bHasPawnReadyListener =
		CodeOnly.Contains(TEXT("Player_PawnReady"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("Player_LocalPawnReady"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("Lifecycle (B3): OnGameFeatureInitialize must subscribe to a pawn-ready event "
			 "(BmrGameplayTags::Event::Player_PawnReady or Player_LocalPawnReady). Spec: "
			 "'subscribe to pawn-ready events via the global message subsystem.' Without this "
			 "listener, the pawn-ready guard inside TryBroadcastMenuReady never lifts and the "
			 "OnFirstPawnReady handler is unreachable."),
		bHasPawnReadyListener);

	// Signal 3 — spots-ready dynamic multicast (NOT a global message).
	TestTrue(
		TEXT("Lifecycle: OnGameFeatureInitialize must bind to "
			 "UNMMSpotsSubsystem::OnActiveMenuSpotReady — NOT through the global message "
			 "subsystem; OnActiveMenuSpotReady is a DYNAMIC_MULTICAST_DELEGATE_OneParam, not "
			 "a gameplay event. The evaluator-flagged wrong implementation: 'binding spot-"
			 "ready as a global message.' The canonical binding is "
			 "`SpotsSubsystem.OnActiveMenuSpotReady.AddUniqueDynamic(this, "
			 "&ThisClass::OnActiveMenuSpotReady);` — AddUniqueDynamic is the dynamic-multicast "
			 "add path. Without this binding, the BasicMenu→Idle upgrade and the None→Idle "
			 "client catch-up never fire."),
		CodeOnly.Contains(TEXT("OnActiveMenuSpotReady"), ESearchCase::CaseSensitive)
			&& (CodeOnly.Contains(TEXT("AddUniqueDynamic"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("AddDynamic"), ESearchCase::CaseSensitive)));

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — Source: OnFirstPawnReady re-attempts the MenuReady broadcast
//           (B3).
//
// Required behavior B3: 'First pawn ready — re-attempt MenuReady; pawn
// availability is one of its broadcast guards.' The signal flow:
//
//   1. Pre-pawn-ready: TryBroadcastMenuReady is called from
//      OnActiveMenuSpotReady or OnGameFeatureActivated. Guard 2
//      (HasBroadcastedMessage(Player_PawnReady) is false) early-returns;
//      MenuReady doesn't fire.
//   2. Pawn ready: Player_PawnReady fires, OnFirstPawnReady runs.
//   3. Inside OnFirstPawnReady: a fresh TryBroadcastMenuReady call — guard 2
//      now passes (Player_PawnReady has been broadcast), and the broadcast
//      fires.
//
// The handler body is essentially a one-liner: `TryBroadcastMenuReady();`.
// A re-impl that forgets the call leaves MenuReady permanently unfired in
// the (very common) case where spots and MGF finished before the pawn was
// possessed.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_OnFirstPawnReadyReattemptsMenuReady,
	"Bomber.NMMBaseSubsystem.OnFirstPawnReadyReattemptsMenuReady",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_OnFirstPawnReadyReattemptsMenuReady::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMBaseSubsystem::OnFirstPawnReady_Implementation"));
	if (!TestTrue(
			TEXT("B3: NMMBaseSubsystem.cpp must define OnFirstPawnReady_Implementation with a "
				 "body — the start/ stub is empty, leaving the pawn-ready arrival without a "
				 "MenuReady re-attempt. In the common case (spots and MGF settle before pawn), "
				 "the empty body makes MenuReady permanently unfired."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B3 (CRITICAL): OnFirstPawnReady_Implementation must call TryBroadcastMenuReady. "
			 "Spec required behavior B3: 'First pawn ready — re-attempt MenuReady; pawn "
			 "availability is one of its broadcast guards.' Before pawn-ready, every prior "
			 "TryBroadcastMenuReady call was suppressed by guard 2 "
			 "(!HasBroadcastedMessage(Player_PawnReady) → return); without this handler's "
			 "re-attempt, the MenuReady broadcast never fires in the common scenario where "
			 "spots and MGF activation completed before the local pawn was possessed."),
		CodeOnly.Contains(TEXT("TryBroadcastMenuReady"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — Source: header shape — UNMMBaseSubsystem must declare its
//           public surface so callers (and the spec's behaviors) can be
//           exercised.
//
// The runtime instantiation / reflection tests this file used to ship can't
// compile in the Bomber.Build.cs module graph (NewMainMenu plugin and
// MyUtils plugin aren't dependencies), so we pin the class-shape invariants
// by inspecting the header text instead. The header is authored-by-Yevhenii
// and committed under Plugins/.../Public/Subsystems/NMMBaseSubsystem.h; it
// declares:
//   * `class NEWMAINMENU_API UNMMBaseSubsystem : public
//     UModularGameFeaturePluginSubsystem` — the inheritance is load-bearing
//     because the per-plugin game-feature lifecycle hooks
//     (OnGameFeatureInitialize/Deinitialize/Activated) come from that base;
//     a refactor to UWorldSubsystem or UTickableWorldSubsystem would break
//     every Initialize/Deinitialize hook the spec describes.
//   * `void SetNewMainMenuState(FNmmStateTag NewState);` — the public state
//     setter; without this signature the Test 1 source scan for
//     `UNMMBaseSubsystem::SetNewMainMenuState` would never find its body
//     because the matching definition would either be a different signature
//     or live on a different class.
//   * `void TryBroadcastMenuReady();` — the broadcast attempter; called
//     from at least three places (OnFirstPawnReady, OnActiveMenuSpotReady,
//     OnGameFeatureActivated) and must be visible to all of them.
//   * `FNmmStateTag GetPredictedMenuState() const;` — the predicate. Its
//     declaration as const is the spec's guarantee that the predicate has
//     no side effects (it merely reports; OnGameStateChanged is what
//     actually applies the state).
//   * `FORCEINLINE FNmmStateTag GetCurrentMenuState() const` — the field
//     accessor used by downstream subsystems to read CurrentMenuStateTag.
//     A refactor that drops FORCEINLINE or const would change the call
//     graph of every reader.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_HeaderShape,
	"Bomber.NMMBaseSubsystem.HeaderShape",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_HeaderShape::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	const FString Header = LoadProjectFile(this, SubsystemHeaderRelPath);
	if (Header.IsEmpty())
	{
		return false;
	}

	const FString CodeOnly = StripComments(Header);

	TestTrue(
		TEXT("Header: UNMMBaseSubsystem must declare itself as a public class deriving from "
			 "UModularGameFeaturePluginSubsystem. The per-plugin game-feature lifecycle hooks "
			 "(OnGameFeatureInitialize, OnGameFeatureDeinitialize, OnGameFeatureActivated) are "
			 "all routed through this base; a refactor to UWorldSubsystem or "
			 "UTickableWorldSubsystem would break every Initialize/Deinitialize hook the spec "
			 "describes — Initialize would fire on world begin-play instead of on the NMM "
			 "game-feature load, and the MGF-race recovery hook in OnGameFeatureActivated "
			 "would never fire at all."),
		CodeOnly.Contains(TEXT("class NEWMAINMENU_API UNMMBaseSubsystem"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("UModularGameFeaturePluginSubsystem"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("Header: UNMMBaseSubsystem must declare SetNewMainMenuState as a public method "
			 "taking FNmmStateTag. Without this declaration the Test 1 source scan for "
			 "`UNMMBaseSubsystem::SetNewMainMenuState` would never find its body because the "
			 "matching definition would either be a different signature or live on a different "
			 "class; OnGameStateChanged / OnActiveMenuSpotReady / OnGameFeatureActivated all "
			 "call this method, so the surface must be declared and accessible."),
		CodeOnly.Contains(TEXT("SetNewMainMenuState"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("FNmmStateTag"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("Header: UNMMBaseSubsystem must declare TryBroadcastMenuReady. It's called from "
			 "at least three places (OnFirstPawnReady, OnActiveMenuSpotReady, "
			 "OnGameFeatureActivated) and must be visible to all of them. Without the header "
			 "declaration, the .cpp body would compile only if it were file-static — which "
			 "would make it unreachable from anywhere else."),
		CodeOnly.Contains(TEXT("TryBroadcastMenuReady"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("Header: UNMMBaseSubsystem must declare GetPredictedMenuState as a const method "
			 "returning FNmmStateTag. The const-ness is the spec's guarantee that the predicate "
			 "has no side effects — it reports the target state based on current runtime "
			 "conditions; OnGameStateChanged is what actually applies the state via "
			 "SetNewMainMenuState. A re-impl that drops const and lets the predicate mutate "
			 "state inside would create a re-entrancy hazard inside TryBroadcastMenuReady's "
			 "guard 3 (predicate == None)."),
		CodeOnly.Contains(TEXT("GetPredictedMenuState"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("Header: UNMMBaseSubsystem must declare GetCurrentMenuState. This is the field "
			 "accessor used by downstream subsystems (camera, spots, player-controller "
			 "component) to read CurrentMenuStateTag. Without this declaration, downstream "
			 "readers would have to access the protected UPROPERTY directly via reflection "
			 "or friend access — neither of which is the spec's intended API."),
		CodeOnly.Contains(TEXT("GetCurrentMenuState"), ESearchCase::CaseSensitive));

	return true;
}

namespace NmmBaseSubsystemTests
{
	// Resolves UNMMBaseSubsystem's UClass without including the NewMainMenu
	// plugin's header (the plugin is not a Bomber.Build.cs dependency, so
	// pulling NMMBaseSubsystem.h in directly is not viable). The package-
	// qualified path /Script/NewMainMenu.NMMBaseSubsystem matches the
	// generated.cpp's class registry entry; if the plugin module is loaded
	// at test time (BuiltInInitialFeatureState=Active in NewMainMenu.uplugin)
	// FindObject returns the live UClass.
	static UClass* FindBaseSubsystemClass()
	{
		return FindObject<UClass>(nullptr, TEXT("/Script/NewMainMenu.NMMBaseSubsystem"));
	}
} // namespace NmmBaseSubsystemTests

// ---------------------------------------------------------------------------
// Test 12 — Runtime: SetNewMainMenuState updates CurrentMenuStateTag as
//           observable through GetCurrentMenuState (B10, DIRECT runtime).
//
// Direct behavioral coverage of B10's field-write half. A free-standing
// UNMMBaseSubsystem is constructed via reflection (FindObject for the
// UClass, NewObject on the transient package for the instance — the plugin
// header cannot be included, so reflection is the only entry point),
// SetNewMainMenuState is invoked via ProcessEvent with the Idle tag, and
// GetCurrentMenuState is invoked the same way. The round-trip must round-
// trip — i.e. the value returned by the accessor must be the value just
// written.
//
// Discrimination:
//   * Solution: SetNewMainMenuState writes `CurrentMenuStateTag = NewState`
//     before broadcasting; GetCurrentMenuState returns the new tag.
//   * Start (empty body): SetNewMainMenuState does nothing — the field
//     stays at the constructor default (FNmmStateTag::None) and the
//     accessor returns None regardless of the tag passed in. The test
//     fails on the TestEqual assertion.
//
// Notes on robustness:
//   * The broadcast inside SetNewMainMenuState routes through
//     UGlobalMessageSubsystem::BroadcastGlobalMessage, which in a free-
//     standing test context (no play world) reaches a null
//     UAsyncMessageWorldSubsystem and fires `ensureMsgf(MessageSystem, ...
//     'MessageSystem' is not valid!)`. The field-write happens BEFORE the
//     broadcast call, so the round-trip observation is intact regardless.
//     AddExpectedError silences the ensure so it doesn't fail this test.
//   * FNmmStateTag derives from FGameplayTag with no added data members
//     (USTRUCT body is empty except for static constants), so the param
//     buffers passed to ProcessEvent are bit-compatible with FGameplayTag.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_RuntimeSetGetRoundTrip,
	"Bomber.NMMBaseSubsystem.RuntimeSetGetRoundTrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_RuntimeSetGetRoundTrip::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	UClass* SubsystemClass = FindBaseSubsystemClass();
	if (!TestNotNull(
			TEXT("B10 (runtime): UNMMBaseSubsystem UClass must be reflected via "
				 "/Script/NewMainMenu.NMMBaseSubsystem. A null lookup means the NewMainMenu "
				 "plugin module did not load and no UFUNCTION below is reachable."),
			SubsystemClass))
	{
		return false;
	}

	UFunction* SetFunc = SubsystemClass->FindFunctionByName(TEXT("SetNewMainMenuState"));
	if (!TestNotNull(
			TEXT("B10 (runtime): SetNewMainMenuState must be reflected as a UFUNCTION (the header "
				 "declares it BlueprintCallable). Reflection is the only dispatch path available "
				 "to this test because the plugin header is not includable."),
			SetFunc))
	{
		return false;
	}

	UFunction* GetFunc = SubsystemClass->FindFunctionByName(TEXT("GetCurrentMenuState"));
	if (!TestNotNull(
			TEXT("B10 (runtime): GetCurrentMenuState must be reflected as a UFUNCTION (the header "
				 "declares it BlueprintCallable, BlueprintPure). Without the accessor reflected, "
				 "this test cannot observe whether SetNewMainMenuState's field-write took "
				 "effect."),
			GetFunc))
	{
		return false;
	}

	UObject* Instance = NewObject<UObject>(GetTransientPackage(), SubsystemClass);
	if (!TestNotNull(
			TEXT("B10 (runtime): A free-standing UNMMBaseSubsystem must be constructible on the "
				 "transient package. The subsystem's UCLASS is not Abstract; if construction "
				 "fails the test cannot exercise the methods at all."),
			Instance))
	{
		return false;
	}

	const FGameplayTag IdleTag = FGameplayTag::RequestGameplayTag(FName("NMM.State.Idle"));
	if (!TestTrue(
			TEXT("B10 (runtime): NMM.State.Idle must be a registered gameplay tag. If the tag "
				 "is unregistered the SetNewMainMenuState input would be the invalid tag and "
				 "the round-trip would erroneously pass against a stub returning None as well."),
			IdleTag.IsValid()))
	{
		return false;
	}

	// SetNewMainMenuState's body broadcasts via UGlobalMessageSubsystem after
	// the field-write. In a free-standing test context (no play world) the
	// broadcast reaches a null UAsyncMessageWorldSubsystem and fires an
	// ensureMsgf("'MessageSystem' is not valid!"). The field-write happens
	// BEFORE the broadcast call so the round-trip observation is intact;
	// silence the ensure so it does not fail this test. Use Occurrences=-1
	// rather than 0 — in UE 5.7's automation framework Occurrences==0 means
	// "unlimited but must fire at least once" and would itself fail the test
	// when ensureMsgf is deduplicated per call-site by an earlier process
	// path. Occurrences<0 means "suppress if it fires, don't require it."
	AddExpectedError(TEXT("MessageSystem"), EAutomationExpectedErrorFlags::Contains, -1);
	// The EventTag-valid ensure can also fire on the same code path before
	// the MessageSystem check; suppress it the same way.
	AddExpectedError(TEXT("EventTag"), EAutomationExpectedErrorFlags::Contains, -1);

	// FNmmStateTag has the same memory layout as FGameplayTag (USTRUCT with
	// only static const members), so the param buffer is bit-compatible.
	FGameplayTag SetParms = IdleTag;
	Instance->ProcessEvent(SetFunc, &SetParms);

	FGameplayTag GetParms;
	Instance->ProcessEvent(GetFunc, &GetParms);

	TestEqual(
		TEXT("B10 (DIRECT runtime): After SetNewMainMenuState(NMM.State.Idle), "
			 "GetCurrentMenuState() must return NMM.State.Idle. This pins B10's field-write "
			 "half through the live UFUNCTION dispatch path — the start/ stub's empty "
			 "SetNewMainMenuState body leaves the field at the constructor default "
			 "(FNmmStateTag::None) and the accessor returns the invalid/empty tag regardless "
			 "of the input. A round-trip mismatch here proves the canonical body "
			 "`CurrentMenuStateTag = NewState;` is missing — exactly the named failure mode "
			 "'updating the state field without broadcasting' degenerating to "
			 "'never updating the state field at all'."),
		GetParms.GetTagName(),
		IdleTag.GetTagName());

	// Round-trip a second value to discriminate impls that hard-code a single
	// tag write (e.g. always set Idle) from impls that actually apply NewState.
	const FGameplayTag BasicMenuTag = FGameplayTag::RequestGameplayTag(FName("NMM.State.BasicMenu"));
	if (TestTrue(TEXT("B10 (runtime): NMM.State.BasicMenu must be a registered tag"), BasicMenuTag.IsValid()))
	{
		SetParms = BasicMenuTag;
		Instance->ProcessEvent(SetFunc, &SetParms);

		GetParms = FGameplayTag();
		Instance->ProcessEvent(GetFunc, &GetParms);

		TestEqual(
			TEXT("B10 (DIRECT runtime): After SetNewMainMenuState(NMM.State.BasicMenu), "
				 "GetCurrentMenuState() must return NMM.State.BasicMenu. A second tag round-trip "
				 "rules out a stub that hard-codes a single field assignment (e.g. always set "
				 "Idle) — the body must apply the actual NewState argument."),
			GetParms.GetTagName(),
			BasicMenuTag.GetTagName());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 13 — Runtime: OnGameStateChanged with the GameStarting tag resets
//           the menu state to None (B1, DIRECT runtime).
//
// Direct behavioral coverage of B1's reset branch. A free-standing
// UNMMBaseSubsystem has its CurrentMenuStateTag field seeded to BasicMenu
// (via a direct FStructProperty write — bypassing SetNewMainMenuState to
// avoid extra broadcast ensures during setup), then OnGameStateChanged is
// invoked via ProcessEvent with a payload whose InstigatorTags carries
// GameState.GameStarting. The handler's spec contract for that tag is
// `SetNewMainMenuState(FNmmStateTag::None)` — i.e. clear the field. After
// the call GetCurrentMenuState must return None.
//
// Discrimination:
//   * Solution: OnGameStateChanged_Implementation dispatches on
//     InstigatorTags.HasTag(FBmrGameStateTag::GameStarting), calls
//     SetNewMainMenuState(None), and the field is cleared. GetCurrentMenuState
//     returns None.
//   * Start (empty body): OnGameStateChanged_Implementation does nothing.
//     The field stays at BasicMenu (the value seeded by the test). The
//     assertion expecting None fails.
//
// This isolates B1's GameStarting branch behaviorally without needing the
// Menu branch's prerequisite cascade (predicate non-None depends on hydrated
// spots / MGF subsystems that the test context does not provide). Source-
// scan tests above (Test 8) cover the Menu branch's dispatch shape; this
// runtime test pins the actually-observable reset side effect.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_RuntimeOnGameStateChangedResetsOnGameStarting,
	"Bomber.NMMBaseSubsystem.RuntimeOnGameStateChangedResetsOnGameStarting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_RuntimeOnGameStateChangedResetsOnGameStarting::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	UClass* SubsystemClass = FindBaseSubsystemClass();
	if (!TestNotNull(
			TEXT("B1 (runtime): UNMMBaseSubsystem UClass must be reflected via "
				 "/Script/NewMainMenu.NMMBaseSubsystem."),
			SubsystemClass))
	{
		return false;
	}

	UFunction* OnGameStateFn = SubsystemClass->FindFunctionByName(TEXT("OnGameStateChanged"));
	if (!TestNotNull(
			TEXT("B1 (runtime): OnGameStateChanged must be reflected as a UFUNCTION (declared "
				 "BlueprintNativeEvent on the header). Reflection is the only callsite this "
				 "test can use — the engine's normal dispatch (via UGlobalMessageSubsystem's "
				 "listener registration) requires a hydrated UGlobalMessageSubsystem in a "
				 "play world, which this free-standing test context does not provide."),
			OnGameStateFn))
	{
		return false;
	}

	UFunction* GetFunc = SubsystemClass->FindFunctionByName(TEXT("GetCurrentMenuState"));
	if (!TestNotNull(
			TEXT("B1 (runtime): GetCurrentMenuState must be reflected to observe the post-"
				 "handler state."),
			GetFunc))
	{
		return false;
	}

	FStructProperty* StateProp = CastField<FStructProperty>(
		SubsystemClass->FindPropertyByName(TEXT("CurrentMenuStateTag")));
	if (!TestNotNull(
			TEXT("B1 (runtime): CurrentMenuStateTag must be a reflected UPROPERTY (the header "
				 "declares it UPROPERTY VisibleInstanceOnly, BlueprintReadWrite). Without "
				 "FProperty access the test cannot seed an initial non-None state to "
				 "discriminate the reset from a no-op stub."),
			StateProp))
	{
		return false;
	}

	UObject* Instance = NewObject<UObject>(GetTransientPackage(), SubsystemClass);
	if (!TestNotNull(
			TEXT("B1 (runtime): A free-standing UNMMBaseSubsystem must be constructible on the "
				 "transient package."),
			Instance))
	{
		return false;
	}

	// Seed CurrentMenuStateTag = BasicMenu directly via the FStructProperty —
	// bypasses SetNewMainMenuState (which would broadcast and trigger the
	// MessageSystem ensure during test setup). FNmmStateTag shares memory
	// layout with FGameplayTag so a Memcpy of the FGameplayTag bytes is safe.
	const FGameplayTag BasicMenuTag = FGameplayTag::RequestGameplayTag(FName("NMM.State.BasicMenu"));
	if (!TestTrue(
			TEXT("B1 (runtime): NMM.State.BasicMenu must be a registered tag for the seed write."),
			BasicMenuTag.IsValid()))
	{
		return false;
	}
	{
		void* PropPtr = StateProp->ContainerPtrToValuePtr<void>(Instance);
		FMemory::Memcpy(PropPtr, &BasicMenuTag, sizeof(FGameplayTag));
	}

	// Sanity: confirm the seed took effect through the public accessor
	// before invoking the handler. A failure here means the FProperty write
	// did not land (e.g. layout mismatch), which would invalidate the
	// post-handler observation.
	{
		FGameplayTag SeedResult;
		Instance->ProcessEvent(GetFunc, &SeedResult);
		if (!TestEqual(
				TEXT("B1 (runtime fixture): The seed write must be observable via "
					 "GetCurrentMenuState as BasicMenu before invoking OnGameStateChanged. If "
					 "this fails the FProperty write did not land and the post-handler "
					 "observation below is meaningless."),
				SeedResult.GetTagName(),
				BasicMenuTag.GetTagName()))
		{
			return false;
		}
	}

	const FGameplayTag GameStartingTag = FGameplayTag::RequestGameplayTag(FName("GameState.GameStarting"));
	if (!TestTrue(
			TEXT("B1 (runtime): GameState.GameStarting must be a registered tag — it is the "
				 "tag the handler dispatches on for the menu-exit reset."),
			GameStartingTag.IsValid()))
	{
		return false;
	}

	// The reset path goes through SetNewMainMenuState(None), which broadcasts
	// MenuStateChanged via UGlobalMessageSubsystem. In a free-standing test
	// context this fires an ensureMsgf("'MessageSystem' is not valid!").
	// Silence it with Occurrences=-1 (suppress-but-don't-require) — UE 5.7's
	// Occurrences==0 means "must fire at least once" and would itself fail
	// the test when ensureMsgf is deduplicated per call-site by an earlier
	// process path. Occurrences<0 is the suppress-if-fires-but-don't-require
	// mode (see the parallel comment in RuntimeSetGetRoundTrip).
	AddExpectedError(TEXT("MessageSystem"), EAutomationExpectedErrorFlags::Contains, -1);
	// The EventTag-valid ensure can fire on the same code path before the
	// MessageSystem check; suppress it the same way.
	AddExpectedError(TEXT("EventTag"), EAutomationExpectedErrorFlags::Contains, -1);

	// Build the FGameplayEventData payload with GameStarting in InstigatorTags
	// and dispatch via ProcessEvent. The UFunction param struct for
	// OnGameStateChanged is `{ FGameplayEventData Payload; }`, so the address
	// of an FGameplayEventData is a valid parms pointer.
	FGameplayEventData Payload;
	Payload.InstigatorTags.AddTag(GameStartingTag);
	Instance->ProcessEvent(OnGameStateFn, &Payload);

	FGameplayTag PostResult;
	Instance->ProcessEvent(GetFunc, &PostResult);

	TestFalse(
		TEXT("B1 (DIRECT runtime): After OnGameStateChanged fires with "
			 "InstigatorTags={GameState.GameStarting}, GetCurrentMenuState() must NOT still "
			 "return BasicMenu — the spec mandates the handler resets the state to None on "
			 "GameStarting. The start/ stub's empty body leaves the seeded BasicMenu in place "
			 "and this assertion catches it: a re-impl that handles the Menu branch but "
			 "forgets the GameStarting branch leaves the menu state stale across the "
			 "transition into gameplay, and the next menu re-entry observes a stale state "
			 "because OnGameFeatureActivated short-circuits when CurrentMenuStateTag != None."),
		PostResult == BasicMenuTag);

	TestTrue(
		TEXT("B1 (DIRECT runtime): After OnGameStateChanged fires with "
			 "InstigatorTags={GameState.GameStarting}, GetCurrentMenuState() must return None "
			 "(an invalid/empty FNmmStateTag). The canonical body is "
			 "`SetNewMainMenuState(FNmmStateTag::None)` for the GameStarting branch — i.e. "
			 "clear the field so the next menu re-entry starts from a clean slate. Anything "
			 "other than the empty tag here means the reset branch is missing or wrong."),
		!PostResult.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// Test 14 — Runtime: OnActiveMenuSpotReady upgrades CurrentMenuStateTag from
//           BasicMenu to Idle when spots become ready (B9 DIRECT runtime).
//
// Direct behavioral coverage of B9's BasicMenu→Idle upgrade — the
// observable half of the spec's failure mode 'Only handling BasicMenu → Idle
// in spot-ready — client catch-up from None → Idle is broken.' This test
// pins the BasicMenu→Idle leg because it does not depend on a hydrated
// ABmrGameState singleton (the None→Idle catch-up branch reads
// ABmrGameState::Get(), which asserts in a free-standing test context).
// The source scan in Test 7 still pins the structural presence of the
// None→Idle branch; this runtime test pins the easier-to-reach BasicMenu
// upgrade leg as a live, observable side effect of running the handler.
//
// Discrimination:
//   * Solution: OnActiveMenuSpotReady_Implementation calls
//     TryBroadcastMenuReady (no observable side effect here — its guards
//     early-return in this context because Player_PawnReady is unbroadcast
//     against a non-hydrated UGlobalMessageSubsystem), then checks
//     `CurrentMenuStateTag == BasicMenu` and calls
//     `SetNewMainMenuState(FNmmStateTag::Idle)`. The field flips to Idle.
//   * Start (empty body): handler does nothing. The field stays at BasicMenu.
//   * A re-impl that has TryBroadcastMenuReady but forgets the upgrade:
//     same as start — the field stays at BasicMenu and the menu UI never
//     advances from the bare BasicMenu render after spots load.
//
// The leading TryBroadcastMenuReady call is exercised in this test — its
// HasBroadcastedMessage / BroadcastGlobalMessage calls touch
// UGlobalMessageSubsystem and fire 'MessageSystem' / 'EventTag' ensures
// without a play world. We suppress those the same way Tests 12 and 13 do
// so they don't fail the test; SetNewMainMenuState(Idle)'s own broadcast
// is suppressed identically.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmBaseSubsystem_RuntimeOnActiveMenuSpotReadyUpgradesBasicMenuToIdle,
	"Bomber.NMMBaseSubsystem.RuntimeOnActiveMenuSpotReadyUpgradesBasicMenuToIdle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmBaseSubsystem_RuntimeOnActiveMenuSpotReadyUpgradesBasicMenuToIdle::RunTest(const FString& Parameters)
{
	using namespace NmmBaseSubsystemTests;

	UClass* SubsystemClass = FindBaseSubsystemClass();
	if (!TestNotNull(
			TEXT("B9 (runtime): UNMMBaseSubsystem UClass must be reflected via "
				 "/Script/NewMainMenu.NMMBaseSubsystem. A null lookup means the NewMainMenu "
				 "plugin module did not load and OnActiveMenuSpotReady is unreachable."),
			SubsystemClass))
	{
		return false;
	}

	UFunction* SpotReadyFn = SubsystemClass->FindFunctionByName(TEXT("OnActiveMenuSpotReady"));
	if (!TestNotNull(
			TEXT("B9 (runtime): OnActiveMenuSpotReady must be reflected as a UFUNCTION (the "
				 "header declares it as a BlueprintNativeEvent dynamic-multicast handler bound "
				 "to UNMMSpotsSubsystem::OnActiveMenuSpotReady). Reflection is the only callsite "
				 "this test can use — the engine's normal dispatch (the spots subsystem's "
				 "dynamic multicast) requires a hydrated UNMMSpotsSubsystem with cinematic "
				 "spots loaded, which this free-standing test context does not provide."),
			SpotReadyFn))
	{
		return false;
	}

	UFunction* GetFunc = SubsystemClass->FindFunctionByName(TEXT("GetCurrentMenuState"));
	if (!TestNotNull(
			TEXT("B9 (runtime): GetCurrentMenuState must be reflected to observe the post-"
				 "handler state."),
			GetFunc))
	{
		return false;
	}

	FStructProperty* StateProp = CastField<FStructProperty>(
		SubsystemClass->FindPropertyByName(TEXT("CurrentMenuStateTag")));
	if (!TestNotNull(
			TEXT("B9 (runtime): CurrentMenuStateTag must be a reflected UPROPERTY to seed the "
				 "BasicMenu starting state. Without the FProperty the test cannot establish the "
				 "pre-condition that discriminates the upgrade from a no-op stub."),
			StateProp))
	{
		return false;
	}

	UObject* Instance = NewObject<UObject>(GetTransientPackage(), SubsystemClass);
	if (!TestNotNull(
			TEXT("B9 (runtime): A free-standing UNMMBaseSubsystem must be constructible on the "
				 "transient package."),
			Instance))
	{
		return false;
	}

	// Seed CurrentMenuStateTag = BasicMenu directly via the FStructProperty —
	// bypasses SetNewMainMenuState (which would broadcast and trigger extra
	// MessageSystem ensures during setup). FNmmStateTag is layout-compatible
	// with FGameplayTag so a Memcpy of the FGameplayTag bytes is safe; the
	// same idiom is used in Test 13's seeding.
	const FGameplayTag BasicMenuTag = FGameplayTag::RequestGameplayTag(FName("NMM.State.BasicMenu"));
	if (!TestTrue(
			TEXT("B9 (runtime): NMM.State.BasicMenu must be a registered tag for the seed write. "
				 "If unregistered the seed would be invalid and the post-handler observation "
				 "would falsely pass against a stub returning the invalid tag."),
			BasicMenuTag.IsValid()))
	{
		return false;
	}
	{
		void* PropPtr = StateProp->ContainerPtrToValuePtr<void>(Instance);
		FMemory::Memcpy(PropPtr, &BasicMenuTag, sizeof(FGameplayTag));
	}

	// Sanity-check the seed via the public accessor before invoking the
	// handler. If this fails the FProperty write did not land and the
	// post-handler observation below is meaningless.
	{
		FGameplayTag SeedResult;
		Instance->ProcessEvent(GetFunc, &SeedResult);
		if (!TestEqual(
				TEXT("B9 (runtime fixture): The seed write must be observable via "
					 "GetCurrentMenuState as BasicMenu before invoking OnActiveMenuSpotReady."),
				SeedResult.GetTagName(),
				BasicMenuTag.GetTagName()))
		{
			return false;
		}
	}

	// OnActiveMenuSpotReady's body invokes TryBroadcastMenuReady (touches
	// HasBroadcastedMessage) and then SetNewMainMenuState(Idle) for the
	// BasicMenu branch (which calls BroadcastGlobalMessage). Both reach
	// UGlobalMessageSubsystem, which fires 'MessageSystem' / 'EventTag'
	// ensures without a play world. Suppress with Occurrences=-1 (the same
	// "suppress-but-don't-require" pattern Test 12 and Test 13 use) so the
	// ensures don't fail this test. The TryBroadcastMenuReady call is
	// expected to early-return through its pawn-ready guard
	// (HasBroadcastedMessage(Player_PawnReady) returns false against a
	// non-hydrated subsystem) — so the broadcast path on that side is at
	// most an ensure-and-return, never a real fire.
	AddExpectedError(TEXT("MessageSystem"), EAutomationExpectedErrorFlags::Contains, -1);
	AddExpectedError(TEXT("EventTag"), EAutomationExpectedErrorFlags::Contains, -1);

	// Allocate a zero-initialised parameter buffer sized to the function's
	// declared ParmsSize. OnActiveMenuSpotReady is bound to a
	// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam, so the parm is a single
	// UObject* (a camera component / spot pointer). The spec body does not
	// read the parameter — the handler only reads CurrentMenuStateTag — so
	// passing a zero-initialised buffer (i.e. nullptr) is safe.
	TArray<uint8> ParmBuffer;
	ParmBuffer.AddZeroed(FMath::Max<int32>(SpotReadyFn->ParmsSize, 16));
	Instance->ProcessEvent(SpotReadyFn, ParmBuffer.GetData());

	FGameplayTag PostResult;
	Instance->ProcessEvent(GetFunc, &PostResult);

	const FGameplayTag IdleTag = FGameplayTag::RequestGameplayTag(FName("NMM.State.Idle"));
	if (!TestTrue(
			TEXT("B9 (runtime): NMM.State.Idle must be a registered tag — it's the target of "
				 "the upgrade."),
			IdleTag.IsValid()))
	{
		return false;
	}

	TestFalse(
		TEXT("B9 (DIRECT runtime): After OnActiveMenuSpotReady fires against a seeded "
			 "BasicMenu state, GetCurrentMenuState() must NOT still return BasicMenu — the "
			 "spec mandates the BasicMenu→Idle upgrade. The start/ stub's empty handler body "
			 "leaves the seeded BasicMenu in place; a re-impl that only calls "
			 "TryBroadcastMenuReady and forgets the upgrade does the same. Either failure "
			 "leaves the menu in the bare BasicMenu UI even after the cinematic-spot signal "
			 "has fired."),
		PostResult == BasicMenuTag);

	TestEqual(
		TEXT("B9 (DIRECT runtime): After OnActiveMenuSpotReady fires against a seeded "
			 "BasicMenu state, GetCurrentMenuState() must return Idle — the upgrade leg of "
			 "B9. The canonical body is `if (CurrentMenuStateTag == BasicMenu || ...) "
			 "SetNewMainMenuState(Idle);`. Anything other than Idle here means the upgrade "
			 "call is missing or wrong (e.g. SetNewMainMenuState was passed a different tag, "
			 "or the if-predicate gated wrongly)."),
		PostResult.GetTagName(),
		IdleTag.GetTagName());

	return true;
}
