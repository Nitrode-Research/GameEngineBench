// Copyright (c) 2026 GameDevBench. Main-Menu spot / spots-subsystem automation tests (Bomber).
//
// Tests target the two stripped files in this task:
//   * Plugins/.../NewMainMenu/Source/NewMainMenu/Private/Components/NMMSpotComponent.cpp
//   * Plugins/.../NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMSpotsSubsystem.cpp
// Together they implement the main-menu character carousel: cycling between
// spots, per-client cinematic playback, async load of level-sequence assets,
// player-state write-through of the chosen character, and the ready-signal
// the menu state machine listens to.
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface needs a hydrated NewMainMenu
//   plugin (a Game-Feature plugin not loaded by default), the menu level
//   ("/Game/Bomber/Maps/MenuMain") with its placed BmrSkeletalMeshActor
//   spot owners, the Data-Registry rows populated with per-character
//   FBmrCinematicRow entries (each holding a soft-pointer to a per-character
//   ULevelSequence asset), and the NMMBaseSubsystem driving menu-state
//   transitions via the GlobalMessageSubsystem. None of that exists as a
//   reusable PIE harness in this project; reproducing it inside an
//   automation test would dwarf the surface under test. The NewMainMenu
//   plugin is also not a Bomber.Build.cs dependency, so we cannot include
//   any of its headers from a Source/Bomber/Tests/ file without changing
//   the module graph.
//
//   The spec and the evaluator notes pin down a small set of code-path
//   choices that separate a working impl from a plausibly-working one:
//     * GetNextSpot's wrap arithmetic must add Num before %% so left-wrap
//       from index 0 doesn't go negative (B1 / common error #5)
//     * cinematics are local-client cosmetics, gated by IsLocalController
//       and never broadcast from authority (B3, B13, evaluator B6 / common
//       error #6)
//     * SetCinematicByState rewrites the Transition state to Idle, so the
//       camera-handoff doesn't blend into a garbage pose (B14, evaluator B5)
//     * ApplyMeshOnPlayer writes the chosen mesh to the player state, not
//       to the map component (B7, B11, evaluator B4 / common error #2)
//     * NotifySpotLoaded gates the ready broadcast on AreAllSpotsLoaded,
//       so the signal fires exactly once when the last spot finishes its
//       async load (B5 / evaluator B23 / common error #3)
//     * InitMasterSequencePlayer reads the already-resolved soft-pointer
//       (CinematicRow.LevelSequence.Get()) rather than synchronously
//       loading it on the main thread; AddNewMainMenuSpot does not bulk-
//       load every cinematic (B4, B5, B12 / common error #4)
//     * ApplyCinematicState short-circuits when MasterPlayer is still null
//       so a state change demanded mid-async-load doesn't crash (B6)
//     * StopMasterSequence routes its stop through SetCinematicByState so
//       the camera handoff is cleaned up identically to a natural end
//       (B14 / B2 skip mid-cinematic cleanup)
//     * ActiveSpotPriority / MainMenuSpots are local-only UPROPERTY state
//       with no CPF_Net flag (B3, B10 — the local-browse-replication
//       failure mode)
//   These are the discriminators the spec calls out as the failure-mode
//   surface — once they are right, the carousel's runtime behavior follows.
//   Source inspection of the two stripped .cpp files is the highest-signal
//   way to check them without re-implementing the menu pipeline.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogNmmSpotsTests, Log, All);

namespace NmmSpotsTests
{
	// ---------------------------------------------------------------------
	// Source-file readers. Both stripped files live inside the NewMainMenu
	// plugin (a Game Feature plugin), not the Bomber game module.
	// ---------------------------------------------------------------------
	static const TCHAR* SpotComponentRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Components/NMMSpotComponent.cpp");
	static const TCHAR* SpotsSubsystemRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMSpotsSubsystem.cpp");
	static const TCHAR* SpotsSubsystemHeaderRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Public/Subsystems/NMMSpotsSubsystem.h");

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

	// Strips C++ line comments (// ...) and block comments (/* ... */) from
	// source text. The canonical solutions and several plausible re-impls
	// contain comments that explain anti-patterns ("don't broadcast from
	// authority", "no eager LoadSynchronous here"). A naive substring scan
	// on the raw source would falsely flag those comments as violations.
	// String literals are preserved so the scan can still see e.g.
	// TEXT("LoadSynchronous") if that ever appears in code (which would
	// itself be a smell, but is not what we filter for here).
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

	// Brace-depth walker to extract the body of a member function definition
	// (from the opening { through the matching close }). Mirrors the helper
	// used by the AiController / BombPlacementExecution tests so that nested
	// initialiser lists / lambdas don't confuse the body extraction.
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
} // namespace NmmSpotsTests

// ---------------------------------------------------------------------------
// Test 1 — Cycling wrap arithmetic: left from first → last, right from last → first.
//
// B1 DIRECT (source pattern): GetNextSpot must wrap via
//   `(ActiveSpotPosition + Incrementer + Num) % Num`.
// The crucial detail is the `+ Num` term: C++ `%` on a signed negative left
// operand keeps the sign, so `(0 + -1) % N` is `-1` (a fatal index for the
// `CurrentLevelTypeSpots[NewSpotIndex]` access immediately below). Without
// the `+ Num`, left-wrap from the first slot either crashes the array
// indexing or returns the wrong spot — both visible as "left from slot 0
// doesn't go to the last slot" (the spec's first failure mode under Cycling).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_CyclingWrapArithmetic,
	"Bomber.NMMSpots.CyclingWrapArithmetic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_CyclingWrapArithmetic::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::GetNextSpot"));
	const FString CodeOnly = StripComments(Body);

	// A trivial stub like `{ return nullptr; }` is short (~20 chars) and contains
	// none of the wrap arithmetic the spec requires. Length-gating up front fails
	// the test loudly on such a body — `!Body.IsEmpty()` would pass vacuously
	// because `{}` and `{ return nullptr; }` are both non-empty strings. The
	// canonical body iterates and indexes spots — well over 200 chars.
	if (!TestTrue(
			TEXT("B1: NMMSpotsSubsystem.cpp must define GetNextSpot with a non-trivial body. "
				 "A stub body like `{ return nullptr; }` doesn't implement the cycling wrap "
				 "arithmetic the spec requires; the canonical body is hundreds of characters "
				 "and must compute `(Position + Incrementer + Num) %% Num` over the priority "
				 "array. Length check fails any body short enough to be a stub."),
			CodeOnly.Len() > 200))
	{
		return false;
	}

	// The body must contain an `if (` — the wrap arithmetic is one path,
	// but the canonical body also guards via ensureMsgf / explicit bounds.
	// A stub with `return nullptr;` has zero conditional flow; that alone
	// can't implement the cycle.
	if (!TestTrue(
			TEXT("B1: GetNextSpot's body must contain at least one `if (` — the canonical body "
				 "guards the ActiveSpotPriority lookup (ensureMsgf on bFoundActivePriority) and "
				 "may early-return on empty arrays. A body without any conditional flow is a "
				 "stub-shaped placeholder that cannot implement the cycle."),
			CodeOnly.Contains(TEXT("if ("), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("ensureMsgf"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("checkf"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The wrap must add the array size before taking modulo, otherwise
	// `(0 + -1) % N` produces -1 in C++ signed semantics and the immediate
	// indexed access `CurrentLevelTypeSpots[NewSpotIndex]` either crashes
	// the checkf inside or wraps to the wrong slot. The canonical encoding
	// uses SpotPriorities.Num() but a re-impl might use CurrentLevelTypeSpots.Num()
	// or AllSpots.Num() — accept any `+ X.Num()` pattern followed by `% X.Num()`.
	const bool bAddsArraySizeBeforeMod =
		(CodeOnly.Contains(TEXT(".Num()) %"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT(".Num()) % "), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT(".Num())%"), ESearchCase::CaseSensitive))
		&& CodeOnly.Contains(TEXT("+ "), ESearchCase::CaseSensitive)
		&& CodeOnly.Contains(TEXT("%"), ESearchCase::CaseSensitive);

	if (!TestTrue(
			TEXT("B1: GetNextSpot must wrap via `(Position + Incrementer + Num) %% Num` — note the "
				 "`+ Num` term. C++ signed `%%` keeps the sign of the left operand, so without the "
				 "additive shift `(0 + -1) %% N` evaluates to -1, and the array index that follows "
				 "either crashes the checkf guard or wraps to the wrong slot. The spec's first "
				 "Cycling rule — 'left from the first slot wraps to the last' — fails outright "
				 "with the naive `(pos + inc) %% n` formulation. (Common error #5.)"),
			bAddsArraySizeBeforeMod))
	{
		return false;
	}

	// The wrap must operate on the active-spot position (the index of the
	// currently selected slot inside the priority array) plus the incrementer
	// argument. A re-implementation that hard-codes a 0 base for the wrap
	// would always return the first/last slot regardless of where the
	// player currently is.
	if (!TestTrue(
			TEXT("B1: GetNextSpot must compute the active spot's position (IndexOfByKey or "
				 "equivalent) and apply the Incrementer relative to that position. Hard-coding a "
				 "0 base for the wrap would return the same slot regardless of where the player "
				 "currently is, breaking the spec's 'cycling triggers the spot's cinematic to play' "
				 "guarantee for every cycle except the very first."),
			CodeOnly.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("Incrementer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// And the body must reach an indexed return off the spots array — the wrap
	// arithmetic without a final indexed return into the spots array means
	// the result of the cycle is never delivered to the caller. The canonical
	// body is `return CurrentLevelTypeSpots[NewSpotIndex];` but a re-impl may
	// rename either side, so accept any `[…]` indexed access in the body.
	if (!TestTrue(
			TEXT("B1: GetNextSpot's body must contain an indexed access off the spots/priorities "
				 "array (e.g. `CurrentLevelTypeSpots[NewSpotIndex]`). A body that does the wrap "
				 "math but never indexes the array effectively no-ops the cycle — the caller "
				 "(MoveMainMenuSpot) ensureMsgf's on the result and bails, so the carousel "
				 "never advances."),
			CodeOnly.Contains(TEXT("["), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("]"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — Cinematic playback is per-client: CanChangeCinematicState bails
//          on non-local controllers and on render-movie / cinematic-mode runs.
//
// B3, B13 DIRECT (source pattern): the spec is explicit that "the cinematic
//   plays per-client; nothing about which slot is being previewed is
//   replicated to other clients" and the failure-mode list calls out
//   "broadcasting the cinematic playback from authority. The cinematic is
//   a per-client cosmetic; broadcasting it produces double-plays or
//   no-plays." The gate that prevents authority/listen-server-side
//   broadcast is CanChangeCinematicState's `IsLocalController()` check.
//   A re-impl that drops the gate runs the cinematic state machine on
//   every client + the server, producing the double-play / no-play
//   failure mode the spec names.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_CinematicLocalControllerOnly,
	"Bomber.NMMSpots.CinematicLocalControllerOnly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_CinematicLocalControllerOnly::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::CanChangeCinematicState"));
	const FString BodyCode = StripComments(Body);

	// A stub returning `return false;` is ~20 chars and contains none of the
	// gates the spec requires. `!Body.IsEmpty()` would pass vacuously on that
	// body, so length-gate up front. Canonical body has three gates plus
	// the equality short-circuit — well over 200 chars.
	if (!TestTrue(
			TEXT("B3/B13: NMMSpotComponent.cpp must define CanChangeCinematicState with a "
				 "non-trivial body. A stub `return false;` doesn't implement the local-controller "
				 "gate, the null-PC guard, or the render-movie short-circuit — without any of "
				 "those, the spot's state machine runs on every client + the server and the "
				 "spec's 'broadcasting from authority' failure mode is in play."),
			BodyCode.Len() > 200))
	{
		return false;
	}

	// The body must also contain CinematicState equality short-circuit logic
	// (the very first branch in the canonical body). A re-impl missing this
	// would re-apply identical states, which doesn't break correctness on
	// its own but is a strong indicator the body isn't the real gate.
	if (!TestTrue(
			TEXT("B3/B13: CanChangeCinematicState must compare CinematicState against the "
				 "incoming NewMenuState (the no-op short-circuit when the spot is already in "
				 "the requested state). The canonical first lines bail false on "
				 "`CinematicState == NewMenuState`; without this check, identical-state "
				 "transitions re-drive the level-sequence player every tick."),
			BodyCode.Contains(TEXT("CinematicState"), ESearchCase::CaseSensitive)
				&& BodyCode.Contains(TEXT("NewMenuState"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B3/B13: CanChangeCinematicState must check IsLocalController() on the player "
				 "controller. Without this gate, the spot's state machine runs on every client + "
				 "the server, producing the spec's 'broadcasting the cinematic playback from "
				 "authority' failure mode: double-plays in PIE listen-server, no-plays on the "
				 "dedicated-server side, and a per-client browse-index that desyncs the moment "
				 "two clients are at different points in their carousel."),
			BodyCode.Contains(TEXT("IsLocalController"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The gate must also bail when there is no local PC at all (the dedicated-
	// server case). A body that only checks `IsLocalController()` without
	// guarding the null PC dereferences it on a server tick where GetPlayerController(0)
	// returns nullptr.
	const bool bNullPCGuarded =
		BodyCode.Contains(TEXT("!MyPC"), ESearchCase::CaseSensitive)
		|| BodyCode.Contains(TEXT("if (MyPC"), ESearchCase::CaseSensitive)
		|| BodyCode.Contains(TEXT("MyPC =="), ESearchCase::CaseSensitive)
		|| BodyCode.Contains(TEXT("IsValid(MyPC)"), ESearchCase::CaseSensitive)
		|| BodyCode.Contains(TEXT("MyPC && "), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B3/B13: CanChangeCinematicState must guard the player-controller dereference. "
				 "On a dedicated server (or any tick where the local PC is not yet replicated in), "
				 "the local-PC lookup returns nullptr; an unguarded `MyPC->IsLocalController()` "
				 "faults. The canonical body bails false on `!MyPC || !MyPC->IsLocalController()`."),
			bNullPCGuarded))
	{
		return false;
	}

	// The render-movie / cinematic-mode short-circuit: when the game is being
	// recorded (PC->bCinematicMode), the menu carousel must not interfere
	// with the recording's own cinematic state machine.
	if (!TestTrue(
			TEXT("B3/B13: CanChangeCinematicState must short-circuit when the local PC is in "
				 "render-movie mode (PC->bCinematicMode). Without this guard, recording a movie "
				 "of the main menu inadvertently triggers the menu's own cinematic state machine "
				 "and double-drives the level-sequence player."),
			BodyCode.Contains(TEXT("bCinematicMode"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Transition → Idle remap: the state machine rewrites Transition
//          to Idle inside SetCinematicByState so the camera handoff doesn't
//          blend into a garbage cinematic pose (B14 / evaluator B5).
//
// The spec is explicit: "The component must remap [Transition] to 'idle'
// semantics for cinematic-playback purposes — so when the camera reaches
// the transition handoff, the spot's cinematic is already at its idle
// pose. Letting the cinematic play through the transition state produces
// visible blends to garbage." The failure-mode list also calls this out
// for skip-mid-play. The fix lives inside SetCinematicByState, and is the
// single sharpest discriminator between a working and a plausibly-working
// implementation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_TransitionRemapsToIdle,
	"Bomber.NMMSpots.TransitionRemapsToIdle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_TransitionRemapsToIdle::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::SetCinematicByState"));
	const FString CodeOnly = StripComments(Body);

	// A stub body `{}` is ~3 chars and contains none of the state-machine
	// routing. `!Body.IsEmpty()` would pass on `{}` so length-gate first.
	// The canonical body has the remap, the gate, the prev-state caching,
	// and the conditional ApplyCinematicState call — over 200 chars.
	if (!TestTrue(
			TEXT("B14: NMMSpotComponent.cpp must define SetCinematicByState with a non-trivial "
				 "body. A stub `{}` body doesn't implement the Transition→Idle remap, the "
				 "CanChangeCinematicState gate, or the CinematicState bookkeeping — without "
				 "any of those, the carousel either crashes (no gate) or blends to garbage "
				 "(no remap)."),
			CodeOnly.Len() > 200))
	{
		return false;
	}

	// The body must also gate via CanChangeCinematicState (the early-out
	// when the change is illegal). Without this gate, the assignment to
	// CinematicState happens unconditionally and a non-local client would
	// still flip its tracking state — breaking the per-client-only contract.
	if (!TestTrue(
			TEXT("B3/B14: SetCinematicByState must consult CanChangeCinematicState before "
				 "applying the transition. Without this gate, the CinematicState assignment "
				 "runs on every client + the server and the spec's 'cinematic plays per-client' "
				 "contract is broken at the state-machine level (regardless of whether the "
				 "level-sequence player itself bails)."),
			CodeOnly.Contains(TEXT("CanChangeCinematicState"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must mention Transition (the input that needs remapping) and
	// Idle (the output it gets rewritten to). Both names must appear in the
	// CODE, not just in comments — a body that mentions Transition in a
	// "TODO remap Transition→Idle" comment but never actually performs the
	// rewrite is the exact failure mode this test catches.
	if (!TestTrue(
			TEXT("B14: SetCinematicByState must reference FNmmStateTag::Transition in CODE (not "
				 "just in a comment). The remap branch needs the Transition tag as its trigger; "
				 "a body that never mentions it can't possibly remap it, and the camera handoff "
				 "blends toward a garbage cinematic pose. (Evaluator B5 / spec State remap section.)"),
			CodeOnly.Contains(TEXT("Transition"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B14: SetCinematicByState must reference FNmmStateTag::Idle in CODE. The remap "
				 "rewrites Transition → Idle, so Idle must appear as the assigned-to value. A "
				 "body that mentions Idle only in a stale comment can't perform the rewrite."),
			CodeOnly.Contains(TEXT("Idle"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The remap is an assignment from the Transition trigger to the Idle
	// value. The canonical shape is `MenuState = FNmmStateTag::Idle;` inside
	// an `if (MenuState == FNmmStateTag::Transition)` branch. We approximate
	// this by checking for an Idle-assignment somewhere in the body — a body
	// that mentions both tokens but never assigns Idle to anything (e.g.,
	// `if (MenuState == FNmmStateTag::Idle) ...` only) would not implement
	// the remap.
	const bool bAssignsIdle =
		CodeOnly.Contains(TEXT("= FNmmStateTag::Idle"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("=FNmmStateTag::Idle"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B14: SetCinematicByState must assign FNmmStateTag::Idle to the (local) MenuState "
				 "variable. The remap shape is `if (MenuState == Transition) MenuState = Idle;`. "
				 "Without the assignment, Transition flows through to ApplyCinematicState and the "
				 "level sequence plays through the transition cells — the spec's 'visible blends "
				 "to garbage' failure mode."),
			bAssignsIdle))
	{
		return false;
	}

	// And the body must reach ApplyCinematicState (or directly route to it) —
	// otherwise the state transition never produces playback. The canonical
	// body calls ApplyCinematicState() after the remap when the new state
	// differs from PrevState.
	if (!TestTrue(
			TEXT("B14: SetCinematicByState must invoke ApplyCinematicState once the state "
				 "actually changes. Without it, the remapped state is recorded but never drives "
				 "the level-sequence player — the cinematic stays frozen on whatever frame it "
				 "was last evaluated at."),
			CodeOnly.Contains(TEXT("ApplyCinematicState"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — Chosen character is persisted through the PlayerState, not the
//          map component (B7, B11 / evaluator B4 / common error #2).
//
// The spec is explicit: "The chosen character (its tag and skin index) is
// written through to the **player state**, not to the map component. The
// player state is the persistent home: it survives pool reuse, network
// dormancy, channel reopen, and respawn. Writing the choice elsewhere
// loses it on any of those transitions." This is the single highest-
// signal discriminator the evaluator notes call out. A model that writes
// to the map component "looks right" in single-player PIE — the chosen
// mesh appears on the pawn — and silently breaks on every pawn-state
// transition listed above.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_ApplyMeshWritesToPlayerState,
	"Bomber.NMMSpots.ApplyMeshWritesToPlayerState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_ApplyMeshWritesToPlayerState::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::ApplyMeshOnPlayer"));
	const FString CodeOnly = StripComments(Body);

	// A stub body `{}` is ~3 chars and contains none of the player-state
	// write-through path. `!Body.IsEmpty()` would pass vacuously on `{}`.
	// The canonical body resolves the local PlayerState, builds the mesh
	// data with fallback skin handling, and calls SetChosenMeshData — over
	// 200 chars.
	if (!TestTrue(
			TEXT("B7/B11: NMMSpotComponent.cpp must define ApplyMeshOnPlayer with a non-trivial "
				 "body. A stub `{}` body doesn't write the chosen mesh anywhere — the spec's "
				 "'chosen character is persisted on the player state' contract is unmet, and "
				 "the chosen mesh is silently dropped on every selection."),
			CodeOnly.Len() > 200))
	{
		return false;
	}

	// Positive: the body must reach for the player state to persist the
	// chosen mesh, and call SetChosenMeshData on it.
	if (!TestTrue(
			TEXT("B7: ApplyMeshOnPlayer must obtain the local player state (e.g. via "
				 "GetLocalPlayerState). The chosen character is replicated and persisted on the "
				 "player state — the local player state is where the write goes."),
			CodeOnly.Contains(TEXT("PlayerState"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("GetLocalPlayerState"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B7: ApplyMeshOnPlayer must call PlayerState->SetChosenMeshData(...). This is "
				 "the setter on ABmrPlayerState that writes the chosen mesh into a replicated "
				 "field. The setter is the only path that propagates the choice to other clients "
				 "and survives pool reuse / network dormancy / channel reopen / respawn — writing "
				 "to a private field (or worse, to the map component) is the named failure mode."),
			CodeOnly.Contains(TEXT("SetChosenMeshData"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// Negative: the body must NOT write to a map component. The failure mode
	// the evaluator names explicitly is "Writing the chosen character to the
	// map component instead of the player state". The map component is the
	// owner-side component the spot lives on; the canonical signature is
	// `MapComponent->SetMeshData(...)` (or any `MapComponent->Set*Mesh*`).
	// Even reading from the map component is a smell, but the most reliable
	// failure-mode signature is a `MapComponent->Set...` call inside this body.
	const bool bWritesToMapComponent =
		CodeOnly.Contains(TEXT("MapComponent->SetChosenMesh"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MapComponent->SetMeshData"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MapComponent->SetPlayerMesh"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetMapComponent()->SetChosenMesh"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetMapComponent()->SetMeshData"), ESearchCase::CaseSensitive);
	if (!TestFalse(
			TEXT("B11 / common error #2: ApplyMeshOnPlayer must NOT write the chosen mesh to a "
				 "map component. The map component is the per-actor grid component; writes there "
				 "are lost on pool reuse and dormancy. The spec is explicit that the chosen mesh "
				 "is replicated/persisted on the player state — writing to the map component is "
				 "the named failure mode that 'looks correct' in single-player PIE but breaks the "
				 "moment the pawn is recycled or the channel reopens."),
			bWritesToMapComponent))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — NotifySpotLoaded gates the ready broadcast on AreAllSpotsLoaded;
//          the signal fires exactly once per "active spot becomes ready"
//          event, not on every per-spot async load completion
//          (B5 / evaluator B23 / common error #3).
//
// The spec calls out the ready signal explicitly: "The signal must fire
// exactly once per 'active spot becomes ready' event, not on every spot
// touch." The evaluator notes pin the gating mechanism on
// AreAllSpotsLoaded — without it, every per-spot async load completion
// would fire the ready signal, and downstream listeners (the camera
// subsystem, the menu state machine) would receive N notifications when
// there are N spots, each pointing at whichever spot happened to finish
// loading first.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_NotifySpotLoadedGatesOnAllLoaded,
	"Bomber.NMMSpots.NotifySpotLoadedGatesOnAllLoaded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_NotifySpotLoadedGatesOnAllLoaded::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::NotifySpotLoaded"));
	const FString CodeOnly = StripComments(Body);

	// Stub body `{}` is ~3 chars and contains no gating logic at all. The
	// canonical body has the SpotComponent validity ensureMsgf, the
	// AreAllSpotsLoaded gate, and the broadcast call — well over 120 chars.
	if (!TestTrue(
			TEXT("B5: NMMSpotsSubsystem.cpp must define NotifySpotLoaded with a non-trivial "
				 "body. A stub `{}` body fires no signal at all — every spot's async load "
				 "completes silently, the camera subsystem and menu state machine never "
				 "receive the ready notification, and the menu remains frozen."),
			CodeOnly.Len() > 120))
	{
		return false;
	}

	// The body must call AreAllSpotsLoaded (the gate) before doing anything
	// that broadcasts the ready signal. Without this check, every per-spot
	// load completion enters the broadcast path, and the signal fires once
	// per loaded spot (the spec's "fires exactly once per 'active spot
	// becomes ready' event, not on every spot touch" requirement fails).
	if (!TestTrue(
			TEXT("B5 / common error #3: NotifySpotLoaded must check AreAllSpotsLoaded() before "
				 "broadcasting. Without this gate, every per-spot async load completion fires the "
				 "ready signal, and a 4-spot menu produces 4 notifications (each pointing at "
				 "whichever spot finished loading at that moment) instead of exactly one. The "
				 "spec is explicit: 'fires exactly once per active spot becomes ready event, not "
				 "on every spot touch.'"),
			CodeOnly.Contains(TEXT("AreAllSpotsLoaded"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must route the broadcast through TryBroadcastOnActiveMenuSpotReady
	// (the canonical helper) — or, equivalently, perform the highest-priority
	// loaded-spot selection + delegate broadcast inline. The simplest signal
	// that the broadcast actually happens is a call to TryBroadcastOnActiveMenuSpotReady,
	// the body of which fires OnActiveMenuSpotReady.Broadcast.
	const bool bBroadcasts =
		CodeOnly.Contains(TEXT("TryBroadcastOnActiveMenuSpotReady"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("OnActiveMenuSpotReady.Broadcast"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B5: NotifySpotLoaded must eventually fire the ready signal once the gate passes "
				 "— either by calling TryBroadcastOnActiveMenuSpotReady (the canonical helper) or "
				 "by broadcasting OnActiveMenuSpotReady directly. A body that only logs the per-"
				 "spot load and never broadcasts leaves the camera subsystem and the menu state "
				 "machine stuck waiting for a signal that never arrives."),
			bBroadcasts))
	{
		return false;
	}

	// Also check AreAllSpotsLoaded itself has the correct shape: it must
	// return false when any spot has cinematic data but no MasterPlayer yet
	// (the "still loading" condition), and false on empty (no spots = no
	// active spot = nothing to broadcast).
	const FString AreAllBody =
		ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::AreAllSpotsLoaded"));
	const FString AreAllCode = StripComments(AreAllBody);

	// AreAllSpotsLoaded in the stub returns `return false;` (~20 chars). The
	// real predicate iterates spots and consults each one's MasterPlayer —
	// the canonical body is over 150 chars (range-for + null checks + the
	// non-empty array check). A `return true;` stub (also ~20 chars) would
	// pass the broadcast on every per-spot call — that's the failure mode
	// the test most needs to catch, and the length gate fails on it too.
	if (!TestTrue(
			TEXT("B5: AreAllSpotsLoaded must have a non-trivial body — it is the gating "
				 "predicate the spec names; a `return false;`/`return true;` stub either "
				 "blocks the ready signal forever or fires it on every per-spot load completion. "
				 "Either way the spec's 'fires exactly once' guarantee is broken."),
			AreAllCode.Len() > 150))
	{
		return false;
	}

	// AreAllSpotsLoaded must contain conditional flow on each spot's loaded
	// state. A body that simply returns `MainMenuSpots.Num() > 0` would
	// pass the iteration + length checks below but never actually examine
	// whether each spot's MasterPlayer is valid. The canonical body uses
	// `if (...) return false;` inside the range-for; alternatively a
	// re-impl could use `AllOf` / `ContainsByPredicate` lambdas.
	if (!TestTrue(
			TEXT("B5: AreAllSpotsLoaded must contain conditional flow (`if (` or a predicate "
				 "lambda) over the spots. The canonical body short-circuits to `return false` "
				 "the moment it finds a spot with a non-empty cinematic row but a null "
				 "MasterPlayer. A body without any if-branch / predicate can't distinguish "
				 "'still loading' from 'fully loaded'."),
			AreAllCode.Contains(TEXT("if ("), ESearchCase::CaseSensitive)
				|| AreAllCode.Contains(TEXT("ContainsByPredicate"), ESearchCase::CaseSensitive)
				|| AreAllCode.Contains(TEXT("AllOf"), ESearchCase::CaseSensitive)
				|| AreAllCode.Contains(TEXT("AnyOf"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B5: AreAllSpotsLoaded must consult each spot's GetMasterPlayer() (the cinematic "
				 "player that gets cached after the soft-pointer resolves). A spot with a non-"
				 "empty CinematicRow but a null MasterPlayer is still loading; treating it as "
				 "'loaded' fires the ready signal before its cinematic is playable."),
			AreAllCode.Contains(TEXT("GetMasterPlayer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// And the predicate must iterate the spots collection (it consults each
	// spot in turn). The canonical body uses a range-for over MainMenuSpots.
	if (!TestTrue(
			TEXT("B5: AreAllSpotsLoaded must iterate MainMenuSpots (or an equivalent collection "
				 "of spots) to consult each one's loaded state. A body that returns true/false "
				 "without iterating the collection can't possibly reflect 'all spots loaded'."),
			AreAllCode.Contains(TEXT("MainMenuSpots"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Async cinematic loading: InitMasterSequencePlayer resolves the
//          already-loaded soft pointer (no eager LoadSynchronous bulk load);
//          AddNewMainMenuSpot does not bulk-load every cinematic at registration
//          (B4, B5, B12 / common error #4).
//
// The spec is explicit: "Eagerly loading all of them at menu open would
// hitch the main menu on first display. The implementation must: Load
// the currently-active spot's cinematic synchronously (the player is
// looking at it). Load the non-active spots' cinematics in the background."
// The evaluator notes pin the implementation pattern: InitMasterSequencePlayer
// expects the soft pointer to be already resolved (called only after the
// batch async load completes), and AddNewMainMenuSpot does NOT iterate
// the spot list calling LoadSynchronous on each cinematic asset.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_AsyncLoadingNoEagerBulkLoad,
	"Bomber.NMMSpots.AsyncLoadingNoEagerBulkLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_AsyncLoadingNoEagerBulkLoad::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString ComponentSource = LoadProjectFile(this, SpotComponentRelPath);
	if (ComponentSource.IsEmpty())
	{
		return false;
	}

	const FString InitBody =
		ExtractMemberBody(ComponentSource, TEXT("UNMMSpotComponent::InitMasterSequencePlayer"));
	const FString InitCode = StripComments(InitBody);

	// Stub body `{}` is ~3 chars and never creates a sequence player. The
	// canonical body has the already-initialised short-circuit, the soft-
	// pointer resolve, the CreateLevelSequencePlayer call, the aspect-ratio
	// override, the OnPause binding, and the NotifySpotLoaded — over 400 chars.
	if (!TestTrue(
			TEXT("B4: NMMSpotComponent.cpp must define InitMasterSequencePlayer with a "
				 "non-trivial body. A stub `{}` body never creates a level-sequence player; "
				 "MasterPlayer stays null, ApplyCinematicState short-circuits, and the "
				 "cinematic never plays."),
			InitCode.Len() > 250))
	{
		return false;
	}

	// Positive: InitMasterSequencePlayer must use the already-resolved soft
	// pointer (`.Get()`), and must create the player through the engine's
	// CreateLevelSequencePlayer factory. The factory is the only path that
	// produces a player wired to the world's PersistentLevel and the engine's
	// sequencer evaluation pipeline.
	if (!TestTrue(
			TEXT("B4: InitMasterSequencePlayer must read the cinematic via CinematicRow.LevelSequence.Get() "
				 "(the soft-pointer resolver). The contract is that the asset has been resolved by "
				 "the time this is called; calling LoadSynchronous here would defeat the spec's "
				 "'load non-active spots' cinematics in the background' requirement."),
			InitCode.Contains(TEXT(".LevelSequence.Get()"), ESearchCase::CaseSensitive)
				|| InitCode.Contains(TEXT("CinematicRow.LevelSequence"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B4: InitMasterSequencePlayer must create the player through "
				 "ULevelSequencePlayer::CreateLevelSequencePlayer — the engine factory that wires "
				 "the player into the persistent level's sequencer evaluation. Hand-rolling a "
				 "NewObject<ULevelSequencePlayer> bypasses the camera-cut binding wiring and the "
				 "cinematic plays without controlling the viewport camera."),
			InitCode.Contains(TEXT("CreateLevelSequencePlayer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// Negative: InitMasterSequencePlayer must NOT itself call LoadSynchronous.
	// The whole point of separating Init from the Add path is that Add can
	// trigger an async resolve and Init runs after the asset is in memory.
	// A body that calls LoadSynchronous here hitches the main thread the
	// same way an eager bulk load would.
	if (!TestFalse(
			TEXT("B12 / common error #4: InitMasterSequencePlayer must NOT call LoadSynchronous "
				 "on the level-sequence asset. A synchronous load here hitches the main thread on "
				 "the same frame as the spot's Init — the exact symptom the spec's 'Eagerly loading "
				 "every cinematic at startup' failure mode names. The contract is that the asset "
				 "is already resolved by the time this is called."),
			InitCode.Contains(TEXT("LoadSynchronous"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// And InitMasterSequencePlayer must notify the subsystem once it finishes,
	// so the subsystem can re-evaluate the all-spots-loaded gate. Per the
	// evaluator notes (B11): "Calls UNMMSpotsSubsystem::Get().NotifySpotLoaded(this)
	// after initializing." Without this call, AreAllSpotsLoaded's iteration
	// finds the new MasterPlayer the next time some other code happens to
	// trigger it, but the spot itself never advertises its readiness — the
	// menu can stall on a frame race.
	if (!TestTrue(
			TEXT("B4/B5: InitMasterSequencePlayer must call NotifySpotLoaded on the subsystem "
				 "once it finishes creating the player. Without that call, the subsystem's "
				 "AreAllSpotsLoaded gate is never re-evaluated and the ready signal can "
				 "stall behind a frame race even after the cinematic asset is in memory."),
			InitCode.Contains(TEXT("NotifySpotLoaded"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// AddNewMainMenuSpot must NOT iterate every registered spot to force-
	// resolve their soft pointers. The canonical body either:
	//   (a) registers the spot and calls InitMasterSequencePlayer only if the
	//       cinematic row already has a resolved soft pointer (the "late spot"
	//       case where the registry finished loading before this spot reached
	//       the registry), OR
	//   (b) waits for the registry's async-load broadcast to fire and resolves
	//       all soft pointers together.
	const FString SubsystemSource = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (SubsystemSource.IsEmpty())
	{
		return false;
	}

	const FString AddBody =
		ExtractMemberBody(SubsystemSource, TEXT("UNMMSpotsSubsystem::AddNewMainMenuSpot"));
	const FString AddCode = StripComments(AddBody);

	if (!TestTrue(
			TEXT("B5: NMMSpotsSubsystem.cpp must define AddNewMainMenuSpot with a non-trivial "
				 "body. A stub `{}` never registers the spot in MainMenuSpots, so the carousel "
				 "has nothing to cycle through. The canonical body has the ensureMsgf, the "
				 "AddUnique, and the late-spot Init branch — over 150 chars."),
			AddCode.Len() > 150))
	{
		return false;
	}

	// The body must put the new spot into the MainMenuSpots collection.
	// Without this, the subsystem holds zero spots and every cycle / lookup
	// returns null.
	if (!TestTrue(
			TEXT("B5: AddNewMainMenuSpot must register the new spot in MainMenuSpots (e.g. "
				 "via AddUnique). A body that skips registration leaves the subsystem with no "
				 "spots to track and the carousel never advances."),
			AddCode.Contains(TEXT("MainMenuSpots"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// A bulk-resolve here is the symptomatic anti-pattern. The simplest
	// signature is a LoadSynchronous call in the body — that would resolve
	// soft pointers on the main thread for every spot as it registers, which
	// is exactly what the spec forbids ("Eagerly loading every cinematic at
	// startup. Hitches the menu on open.").
	if (!TestFalse(
			TEXT("B12 / common error #4: AddNewMainMenuSpot must NOT call LoadSynchronous on the "
				 "cinematic asset(s). The spec is explicit: 'eagerly loading every cinematic at "
				 "startup hitches the menu on open'. The canonical add path either initialises "
				 "the player immediately when the cinematic row is already resolved (the late-"
				 "spot case) or waits for the data-registry's async-load broadcast — never "
				 "synchronously resolves the soft pointer at registration time."),
			AddCode.Contains(TEXT("LoadSynchronous"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// And the add path must reach for InitMasterSequencePlayer (the entry point
	// that runs once the asset is in memory). A re-impl that only stuffs the
	// new spot into MainMenuSpots and never calls InitMasterSequencePlayer
	// for late spots leaves them stuck waiting for an async-load broadcast
	// that has already fired and won't fire again until the registry is
	// re-loaded.
	if (!TestTrue(
			TEXT("B5: AddNewMainMenuSpot must reach InitMasterSequencePlayer for the late-spot "
				 "case — when a spot registers after the data registry has already finished its "
				 "batch async load, its CinematicRow is already valid and the add path must kick "
				 "off the player creation immediately. Without this branch, late spots never "
				 "advance to the loaded state and the ready signal never fires (because "
				 "AreAllSpotsLoaded sees a non-empty row with a null MasterPlayer)."),
			AddCode.Contains(TEXT("InitMasterSequencePlayer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — ApplyCinematicState short-circuits when MasterPlayer is null
//          (B6 — no crash when a state change demands a slot whose
//          cinematic hasn't finished its background load yet).
//
// The spec is explicit: "Handle the case where a state change demands a
// slot whose cinematic hasn't finished its background load yet — by
// forcing a synchronous load on demand. The cinematic must play correctly
// either way; no null-player crashes." The canonical body's first line
// is the null-MasterPlayer short-circuit: if the player is still null
// (the async resolve is in flight), bail; the deferred state will be
// applied by InitMasterSequencePlayer once the load completes.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_ApplyCinematicStateNullPlayerGuard,
	"Bomber.NMMSpots.ApplyCinematicStateNullPlayerGuard",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_ApplyCinematicStateNullPlayerGuard::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::ApplyCinematicState"));
	const FString CodeOnly = StripComments(Body);

	// Stub body `{}` is ~3 chars and never touches MasterPlayer. The
	// canonical body has the null guard, SetFrameRange, SetPlaybackSettings,
	// SetPlaybackPosition, the None short-circuit, Play, the jump
	// SetPlaybackPosition, and the Cinematic broadcast — over 400 chars.
	if (!TestTrue(
			TEXT("B6: NMMSpotComponent.cpp must define ApplyCinematicState with a non-trivial "
				 "body. A stub `{}` body never drives playback at all; even if MasterPlayer "
				 "becomes valid, the cinematic remains frozen because no Play call is issued."),
			CodeOnly.Len() > 300))
	{
		return false;
	}

	// The body must early-out when MasterPlayer is still null. The most
	// common encodings are `if (!MasterPlayer) return;` or
	// `if (MasterPlayer == nullptr) return;`. Either way, MasterPlayer
	// must be checked before any dereference (the very next lines deref
	// it for SetFrameRange, SetPlaybackSettings, etc.).
	const bool bGuardsNullPlayer =
		CodeOnly.Contains(TEXT("!MasterPlayer"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MasterPlayer == nullptr"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("nullptr == MasterPlayer"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("!IsValid(MasterPlayer)"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("IsValid(MasterPlayer)"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B6: ApplyCinematicState must guard the null-MasterPlayer case. The spec calls "
				 "this out as a required behavior: 'Handle the case where a state change demands "
				 "a slot whose cinematic hasn't finished its background load yet — no null-player "
				 "crashes.' The very next lines of the body deref MasterPlayer for SetFrameRange "
				 "/ SetPlaybackSettings / SetPlaybackPosition / Play — an unguarded entry faults "
				 "during the async-load window."),
			bGuardsNullPlayer))
	{
		return false;
	}

	// The body must also call the actual playback APIs once the guard passes.
	// A body that bails on null-MasterPlayer but never plays anything when
	// it IS valid breaks the carousel's per-cycle cinematic trigger (B2).
	if (!TestTrue(
			TEXT("B2: ApplyCinematicState must actually drive playback on the level-sequence "
				 "player once MasterPlayer is non-null. The canonical body calls SetFrameRange, "
				 "SetPlaybackSettings, SetPlaybackPosition, and Play. A body that only short-"
				 "circuits and never reaches Play leaves the cinematic frozen on its first frame."),
			CodeOnly.Contains(TEXT("MasterPlayer->Play"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("->Play("), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Mid-play skip cleanup: StopMasterSequence routes through
//          SetCinematicByState(None), so the camera handoff is cleaned up
//          identically to a natural cinematic end (B2 / B14).
//
// The spec is explicit about this: "skipping ends playback cleanly and
// applies the destination spot's selection" and "Skipping a cinematic
// mid-play without cleaning up the camera handoff. Leaves the camera
// blending toward a destination pose that's no longer current." The
// canonical fix is to route the stop through the same state-machine
// path as a natural pause — that runs the Transition→Idle remap, the
// CinematicState bookkeeping, and the SetPlaybackPosition reset. A
// body that calls MasterPlayer->Stop() directly skips all of that and
// leaves the camera blending to garbage.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_StopMasterSequenceRoutesThroughStateMachine,
	"Bomber.NMMSpots.StopMasterSequenceRoutesThroughStateMachine",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_StopMasterSequenceRoutesThroughStateMachine::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::StopMasterSequence"));
	const FString CodeOnly = StripComments(Body);

	// Stub body `{}` is ~3 chars and contains no state-machine routing. The
	// canonical body has the MasterPlayer && IsPlaying() guard + the route
	// through SetCinematicByState(None) — ~105 chars. A stub returning
	// nothing (`{}`) or a direct `MasterPlayer->Stop();` call (~27 chars)
	// both fail this gate.
	if (!TestTrue(
			TEXT("B14: NMMSpotComponent.cpp must define StopMasterSequence with a non-trivial "
				 "body. A stub `{}` body never stops the cinematic — skipping mid-play does "
				 "nothing, and the camera blends toward a destination pose that's no longer "
				 "current (the spec's named failure mode)."),
			CodeOnly.Len() > 80))
	{
		return false;
	}

	// The body must also guard on MasterPlayer->IsPlaying() — the canonical
	// signature is `if (MasterPlayer && MasterPlayer->IsPlaying())`. Without
	// the IsPlaying() check, StopMasterSequence is called on idle spots and
	// the state machine churns CinematicState back to None on every call,
	// firing the OnPause delegate spuriously.
	if (!TestTrue(
			TEXT("B14: StopMasterSequence must check MasterPlayer->IsPlaying() before routing "
				 "through SetCinematicByState. Calling the state machine on an idle player "
				 "churns CinematicState back to None pointlessly and fires OnPause spuriously, "
				 "which the OnMasterSequencePaused handler interprets as a natural end-of-play."),
			CodeOnly.Contains(TEXT("IsPlaying"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must route through SetCinematicByState, NOT call Stop() on
	// the LevelSequencePlayer directly. The state-machine path runs the
	// CinematicState bookkeeping, the Transition→Idle remap (if applicable),
	// and the SetPlaybackPosition reset that cleans up the camera handoff.
	// MasterPlayer->Stop() called directly bypasses all of that.
	if (!TestTrue(
			TEXT("B14: StopMasterSequence must route through SetCinematicByState(FNmmStateTag::None). "
				 "Calling MasterPlayer->Stop() directly bypasses the CinematicState bookkeeping and "
				 "the SetPlaybackPosition reset that cleans up the camera handoff — the spec's "
				 "named 'Skipping a cinematic mid-play without cleaning up the camera handoff' "
				 "failure mode. Routing through the state machine ensures skip-mid-play and "
				 "natural-end take the same cleanup path."),
			CodeOnly.Contains(TEXT("SetCinematicByState"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("FNmmStateTag::None"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must guard the call on the player actually being in a
	// playable state (MasterPlayer non-null AND playing). Calling
	// SetCinematicByState(None) when the spot is already idle is a no-op
	// (CanChangeCinematicState's `CinematicState == NewMenuState` short-
	// circuit), but the body should still avoid the call when MasterPlayer
	// is null to keep the intent legible and avoid the SetCinematicByState
	// log spam.
	if (!TestTrue(
			TEXT("B14: StopMasterSequence must guard on the MasterPlayer being valid. The body "
				 "is called from OnNewMainMenuStateChanged for non-current spots — those may not "
				 "yet have a player (still async-loading) and stopping them is a no-op the body "
				 "should skip cleanly without entering the state machine."),
			CodeOnly.Contains(TEXT("MasterPlayer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Local-only browse state: ActiveSpotPriority and MainMenuSpots
//          carry no CPF_Net flag (B3, B10 / common error #1).
//
// The spec is explicit: "The 'currently active spot' is local-client
// state. The cinematic plays per-client; nothing about which slot is
// being previewed is replicated to other clients. What replicates is
// the player's eventual character commitment, not their browsing." The
// failure-mode list pins this further: "Replicating the local browse
// index. Looks correct in single-player PIE; desyncs the moment two
// clients are at different points in their carousel."
//
// The subsystem header declares ActiveSpotPriority and MainMenuSpots
// as plain Transient UPROPERTYs. A re-impl that adds the `Replicated`
// specifier (or replaces UPROPERTY metadata with anything that flips
// CPF_Net on) would silently introduce the named failure mode. This
// test is a hybrid: it checks the FProperty's CPF_Net flag at runtime
// AND grep's the header source so the test still catches the violation
// when the subsystem class isn't loaded (Game Feature plugin off).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_BrowseStateIsLocalOnly,
	"Bomber.NMMSpots.BrowseStateIsLocalOnly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_BrowseStateIsLocalOnly::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Header = LoadProjectFile(this, SpotsSubsystemHeaderRelPath);
	if (Header.IsEmpty())
	{
		return false;
	}

	const FString HeaderCode = StripComments(Header);

	// The header carries the two browse-state properties: ActiveSpotPriority
	// (the priority of the currently selected slot, on this client) and
	// MainMenuSpots (the array of registered spot components). Both must be
	// declared without any replication specifier — no Replicated, no
	// ReplicatedUsing.
	TestTrue(
		TEXT("Sanity: NMMSpotsSubsystem.h must declare ActiveSpotPriority as a UPROPERTY. "
			 "The header was not stripped; if this assertion fails, the schema has shifted "
			 "and the rest of this test's expectations may not apply."),
		HeaderCode.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("Sanity: NMMSpotsSubsystem.h must declare MainMenuSpots as a UPROPERTY."),
		HeaderCode.Contains(TEXT("MainMenuSpots"), ESearchCase::CaseSensitive));

	// Walk every UPROPERTY(...) attribute block in the header. For each
	// block, find the property name in the following declaration. If the
	// declaration matches one of the browse-state properties, the attribute
	// block must NOT mention Replicated / ReplicatedUsing.
	int32 SearchPos = 0;
	bool bActiveSpotPriorityChecked = false;
	bool bMainMenuSpotsChecked = false;
	while (SearchPos < HeaderCode.Len())
	{
		const int32 PropStart = HeaderCode.Find(TEXT("UPROPERTY("), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchPos);
		if (PropStart == INDEX_NONE)
		{
			break;
		}
		const int32 AttrEnd = HeaderCode.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, PropStart);
		if (AttrEnd == INDEX_NONE)
		{
			break;
		}
		const FString AttrBlock = HeaderCode.Mid(PropStart, AttrEnd - PropStart + 1);

		// Find the declaration line — the next ';' after the attribute end
		// is the property's full declaration; the property name appears in it.
		const int32 DeclEnd = HeaderCode.Find(TEXT(";"), ESearchCase::CaseSensitive, ESearchDir::FromStart, AttrEnd);
		if (DeclEnd == INDEX_NONE)
		{
			break;
		}
		const FString Decl = HeaderCode.Mid(AttrEnd + 1, DeclEnd - AttrEnd - 1);

		auto AttrIsReplicated = [&AttrBlock]()
		{
			return AttrBlock.Contains(TEXT("Replicated"), ESearchCase::CaseSensitive);
		};

		if (Decl.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive))
		{
			TestFalse(
				TEXT("B3 / B10 / common error #1: ActiveSpotPriority must NOT carry a "
					 "Replicated (or ReplicatedUsing) UPROPERTY specifier. The spec is "
					 "explicit: 'the currently active spot is local-client state; nothing "
					 "about which slot is being previewed is replicated to other clients'. "
					 "Replicating it desyncs the moment two clients are at different points "
					 "in their carousel (the spec's named failure mode that 'looks correct "
					 "in single-player PIE')."),
				AttrIsReplicated());
			bActiveSpotPriorityChecked = true;
		}
		else if (Decl.Contains(TEXT("MainMenuSpots"), ESearchCase::CaseSensitive))
		{
			TestFalse(
				TEXT("B3 / B10: MainMenuSpots (the array of registered spot components) must "
					 "NOT carry a Replicated specifier. The registered spots are local to the "
					 "owning client (each client builds its own list from the spawned actors "
					 "in the menu level); replicating the array invites stale UObject pointers "
					 "on clients whose pawn graph differs from the server's."),
				AttrIsReplicated());
			bMainMenuSpotsChecked = true;
		}

		SearchPos = DeclEnd + 1;
	}

	if (!TestTrue(
			TEXT("Sanity: the parser found ActiveSpotPriority's UPROPERTY block — if not, the "
				 "header layout has changed and the replication check above was skipped."),
			bActiveSpotPriorityChecked))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("Sanity: the parser found MainMenuSpots' UPROPERTY block — if not, the header "
				 "layout has changed and the replication check above was skipped."),
			bMainMenuSpotsChecked))
	{
		return false;
	}

	// And the .cpp side: GetLifetimeReplicatedProps for this subsystem must
	// not exist at all (the subsystem is not an Actor and is not replicated),
	// and the cpp must not call DOREPLIFETIME on the browse-state properties.
	const FString CppSource = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (CppSource.IsEmpty())
	{
		return false;
	}

	const FString CppCode = StripComments(CppSource);
	const bool bMentionsDORep =
		CppCode.Contains(TEXT("DOREPLIFETIME(UNMMSpotsSubsystem, ActiveSpotPriority"), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT("DOREPLIFETIME(UNMMSpotsSubsystem, MainMenuSpots"), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT("DOREPLIFETIME_WITH_PARAMS_FAST(UNMMSpotsSubsystem, ActiveSpotPriority"), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT("DOREPLIFETIME_WITH_PARAMS_FAST(UNMMSpotsSubsystem, MainMenuSpots"), ESearchCase::CaseSensitive);
	if (!TestFalse(
			TEXT("B3 / B10: NMMSpotsSubsystem.cpp must NOT register ActiveSpotPriority or "
				 "MainMenuSpots through any DOREPLIFETIME macro. The subsystem is a "
				 "UModularGameFeaturePluginSubsystem, not an Actor — its UPROPERTYs are not "
				 "replicated. Even if the header somehow carried a Replicated specifier, a "
				 "DOREPLIFETIME registration would explicitly opt these properties into the "
				 "wire-protocol — which is exactly the local-browse-replication failure mode "
				 "the spec forbids."),
			bMentionsDORep))
	{
		return false;
	}

	// Without these positive USE checks, this test passes vacuously on a stub
	// .cpp that has empty bodies — `bMentionsDORep` is trivially false when
	// the cpp contains nothing. To gate the stub vs solution, the browse-state
	// properties must actually be USED in the cpp body (read for cycling /
	// active-spot lookup, written when the player moves through the carousel
	// or when the subsystem deinitialises). A stub that never touches
	// ActiveSpotPriority can't possibly be implementing the carousel; equally
	// for MainMenuSpots, which is the registry the carousel iterates.
	if (!TestTrue(
			TEXT("B3 / B10 / common error #1: NMMSpotsSubsystem.cpp must actually USE "
				 "ActiveSpotPriority — read it (e.g. in GetCurrentSpot, GetNextSpot, "
				 "HandleUnavailableMenuSpot) or write it (in MoveMainMenuSpot, "
				 "TryBroadcastOnActiveMenuSpotReady, OnGameFeatureDeinitialize). A cpp body "
				 "that never references ActiveSpotPriority can't implement the active-spot "
				 "tracking the carousel requires — the property is just dead state, and the "
				 "subsystem doesn't actually drive the cycle. The browse state being "
				 "'local-only' is meaningless if it isn't used at all."),
			CppCode.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// Note: the stub of GetMainMenuSpots has `MainMenuSpots` only as part of the
	// function signature `GetMainMenuSpots(TArray<...>& OutSpots)`. To distinguish
	// stub from solution, require an actual member ACCESS (`.` or `[`) on the
	// field — that only appears in the cpp when the body iterates / indexes /
	// mutates the array.
	const bool bMainMenuSpotsActuallyUsed =
		CppCode.Contains(TEXT("MainMenuSpots."), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT("MainMenuSpots["), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT("MainMenuSpots)"), ESearchCase::CaseSensitive)
		|| CppCode.Contains(TEXT(": MainMenuSpots"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B3 / B10: NMMSpotsSubsystem.cpp must actually USE MainMenuSpots — the array "
				 "is the registry of placed spots that the carousel iterates over (in "
				 "AddNewMainMenuSpot, GetNextSpot, AreAllSpotsLoaded, OnCinematicRowsChanged). "
				 "Require a member access (`MainMenuSpots.`, `MainMenuSpots[`, or a range-for "
				 "binding) so a stub whose only mention is the GetMainMenuSpots function "
				 "signature can't satisfy this check. Without the array actually being read or "
				 "mutated, the carousel has no registry to track."),
			bMainMenuSpotsActuallyUsed))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — Cycling actually triggers playback: MoveMainMenuSpot advances the
//           browse state and routes through the menu state machine that drives
//           the cinematic (B2 — "cycling triggers the spot's cinematic to play"
//           — plus the menu/in-game branch split, B27/B28).
//
// B1 covers the *math* of cycling (the wrap arithmetic inside GetNextSpot).
// B2 covers the *trigger* — that cycling actually produces a cinematic play
// for the new spot. The body that connects the two is MoveMainMenuSpot: it
// asks GetNextSpot for the next slot, mutates ActiveSpotPriority +
// LastMoveSpotDirection, and then either fires the menu state machine
// (which kicks the level-sequence player into Idle / Transition) or — when
// called during an in-game spot swap — applies the destination mesh
// directly via ApplyMeshOnPlayer. A stub `return nullptr;` body advances
// nothing: the left/right keys appear inert, no cinematic plays, and the
// player state never receives a write. This is the one behavior in G1
// where the trigger side has no dedicated coverage today.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_MoveMainMenuSpotUpdatesState,
	"Bomber.NMMSpots.MoveMainMenuSpotUpdatesState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_MoveMainMenuSpotUpdatesState::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::MoveMainMenuSpot"));
	const FString CodeOnly = StripComments(Body);

	// A stub `return nullptr;` body is ~20 chars and skips every spec-named
	// effect of cycling. The canonical body resolves the next spot, writes
	// the priority, writes the direction, branches on game state, and either
	// drives the menu state machine or calls ApplyMeshOnPlayer — comfortably
	// over 200 chars.
	if (!TestTrue(
			TEXT("B2: NMMSpotsSubsystem.cpp must define MoveMainMenuSpot with a non-trivial "
				 "body. A stub `return nullptr;` body never advances the carousel — the "
				 "left/right cycle keys look inert (no cinematic, no mesh swap, no priority "
				 "advance), and the spec's 'cycling triggers the spot's cinematic to play' "
				 "guarantee fails outright."),
			CodeOnly.Len() > 200))
	{
		return false;
	}

	// The body must resolve the next slot through GetNextSpot — that's the
	// single wrap-arithmetic routine that B1 verifies. A re-impl that inlines
	// its own wrap math (or that uses a different index source) duplicates
	// GetNextSpot's logic and invites drift between the two implementations:
	// one place computes `(pos + inc + Num) %% Num`, the other might miss the
	// `+ Num` term and silently re-introduce the negative-index failure mode
	// on left-wrap from the first slot.
	if (!TestTrue(
			TEXT("B1/B2: MoveMainMenuSpot must consult GetNextSpot to resolve the next slot. "
				 "Inlining the wrap math somewhere else duplicates the arithmetic and invites "
				 "drift — one site computes the wrap with the `+ Num` shift, another forgets "
				 "it and reintroduces the negative-index failure mode on left-wrap from the "
				 "first slot."),
			CodeOnly.Contains(TEXT("GetNextSpot"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must write to ActiveSpotPriority — without this assignment,
	// GetCurrentSpot keeps returning the previously-selected slot and the
	// menu state machine fires its events against the wrong spot. The cycle
	// effectively no-ops at the data layer regardless of what the state
	// machine does downstream.
	if (!TestTrue(
			TEXT("B2: MoveMainMenuSpot must update ActiveSpotPriority to the next spot's "
				 "priority value. Without this write, GetCurrentSpot continues returning the "
				 "previously-selected spot, downstream state-machine events fire against the "
				 "wrong slot, and the carousel never visibly advances even when the wrap "
				 "math is correct."),
			CodeOnly.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// LastMoveSpotDirection caches the incrementer for downstream camera /
	// transition consumers (the rail animation reads this to know which side
	// of the carousel to animate from). A body that mutates ActiveSpotPriority
	// but forgets LastMoveSpotDirection produces a camera that animates in
	// the wrong direction (or freezes on its initial pose) even though the
	// active-spot data is correct.
	if (!TestTrue(
			TEXT("B2: MoveMainMenuSpot must record the Incrementer in LastMoveSpotDirection. "
				 "The camera-rail transition reads this to decide which side of the carousel "
				 "to animate from; a missing write produces a camera that swings the wrong "
				 "way (or hangs on its initial pose) even though ActiveSpotPriority advanced."),
			CodeOnly.Contains(TEXT("LastMoveSpotDirection"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The menu-state branch must fire SetNewMainMenuState — that's the call
	// that the level-sequence player ultimately responds to (via the global
	// MenuStateChanged message routed back into SetCinematicByState). Without
	// it, ActiveSpotPriority advances silently and the new spot's cinematic
	// never starts — the spec's "cycling triggers the spot's cinematic to play"
	// fails.
	if (!TestTrue(
			TEXT("B2: MoveMainMenuSpot must fire the menu state machine via "
				 "SetNewMainMenuState (Idle for instant-switch settings, Transition otherwise) "
				 "when invoked during the menu state. Without this call, the active-spot "
				 "advances but the cinematic player never receives a state-change event — "
				 "the new spot's cinematic does not play and the spec's 'cycling triggers "
				 "the spot's cinematic to play' guarantee fails."),
			CodeOnly.Contains(TEXT("SetNewMainMenuState"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The in-game branch routes through ApplyMeshOnPlayer — that's the path
	// the spec ties to "the destination spot's selection" application after
	// a skip-mid-cinematic / mid-game spot swap. A body that branches on game
	// state but only updates ActiveSpotPriority (without ApplyMeshOnPlayer)
	// leaves the new spot's mesh unapplied on the pawn during the in-game
	// swap case.
	if (!TestTrue(
			TEXT("B2/B28: MoveMainMenuSpot must call ApplyMeshOnPlayer on the next spot in the "
				 "in-game branch (not the menu branch). The in-game swap path is what runs "
				 "during a mid-game character change; without this call, the chosen mesh "
				 "never propagates to the new pawn and the destination spot's selection "
				 "isn't applied — the spec's 'skipping ends playback cleanly and applies "
				 "the destination spot's selection' fails for the in-game branch."),
			CodeOnly.Contains(TEXT("ApplyMeshOnPlayer"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// And the body must branch on game state — both behaviors must coexist,
	// gated on whether we're in the menu or in-game. A body that only
	// executes the menu branch fires SetNewMainMenuState during an in-game
	// swap (which has no menu state machine to respond), and vice versa.
	// The canonical body checks `ABmrGameState::Get().HasMatchingGameplayTag(FBmrGameStateTag::Menu)`
	// — accept any reference to FBmrGameStateTag::Menu, HasMatchingGameplayTag,
	// or ABmrGameState as evidence the branch is in place.
	const bool bBranchesOnGameState =
		CodeOnly.Contains(TEXT("FBmrGameStateTag::Menu"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("HasMatchingGameplayTag"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("ABmrGameState"), ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B2/B27/B28: MoveMainMenuSpot must branch on the current game state — the menu "
				 "branch fires SetNewMainMenuState (cinematic-driven), the in-game branch calls "
				 "ApplyMeshOnPlayer directly. A body that always takes one branch breaks one or "
				 "the other case: cycling from the menu doesn't play a cinematic, or cycling "
				 "in-game doesn't update the mesh."),
			bBranchesOnGameState))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — Session restart restores the last-chosen spot from saved data
//           (B9). The save side of that contract is MarkCinematicAsSeen,
//           which persists the current spot's CinematicRow.Priority into
//           UNMMSaveGameData. Without this write, session restart has
//           nothing to read and the menu always opens on whichever slot
//           the priority-sort places first — not the slot the player
//           last chose.
//
// The current test surface covers B7/B11 (write the chosen mesh to the
// player state) but the player-state write is the *runtime* persistence
// path; it doesn't survive a process restart. The spec separately requires
// "A session restart restores the last-chosen spot from saved data so the
// menu opens on the right slot." — that's a save-game persistence path,
// the write side of which is this body. B9 has no dedicated coverage in
// the rest of the file.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_MarkCinematicAsSeenPersistsChoice,
	"Bomber.NMMSpots.MarkCinematicAsSeenPersistsChoice",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_MarkCinematicAsSeenPersistsChoice::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotComponentRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMSpotComponent::MarkCinematicAsSeen"));
	const FString CodeOnly = StripComments(Body);

	// A stub `{}` body is ~3 chars and never reaches the save game. The
	// canonical body has the IsCurrentSpot guard, the SaveGameData lookup,
	// and the MarkCinematicAsSeen(Priority) call — about 100 chars. A stub
	// here silently breaks B9: every session opens on the highest-priority
	// slot regardless of which slot the player actually chose last time.
	if (!TestTrue(
			TEXT("B9: NMMSpotComponent.cpp must define MarkCinematicAsSeen with a non-trivial "
				 "body. A stub `{}` body never writes anything to the save game; the load "
				 "side on next session has nothing to read, and the menu always opens on "
				 "whichever slot priority-sort orders first — not the slot the player last "
				 "chose. The spec is explicit: 'a session restart restores the last-chosen "
				 "spot from saved data so the menu opens on the right slot.'"),
			CodeOnly.Len() > 80))
	{
		return false;
	}

	// The body must gate on IsCurrentSpot — only the active spot should
	// persist its own marker. A re-impl that fires this from every spot's
	// event handler (e.g., on a global MenuStateChanged tick) would log
	// every spot as seen every time the state machine touches them; the
	// "last seen" key effectively becomes "all spots equally", and the
	// restoration path can't pick a single slot to open on.
	if (!TestTrue(
			TEXT("B9: MarkCinematicAsSeen must gate on IsCurrentSpot. The save-game marker "
				 "must reflect *the* spot the player actually viewed; a body that marks every "
				 "spot the state machine touches turns the saved key into 'every slot is "
				 "equally recent', and the load side has no way to pick the correct slot to "
				 "open on next session."),
			CodeOnly.Contains(TEXT("IsCurrentSpot"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The body must reach the save-game data object — that's the persistence
	// target that survives a process restart. Writing the marker into a
	// transient subsystem field (or into the player state, which the spec
	// already uses for the chosen-mesh) doesn't survive game exit and breaks
	// the restoration on next launch.
	if (!TestTrue(
			TEXT("B9: MarkCinematicAsSeen must obtain the save-game data object (the canonical "
				 "path is UNMMUtils::GetSaveGameData) and write the marker into it. The save "
				 "game is the only persistence target that survives a process restart — "
				 "writing the marker into a transient subsystem field or the player state "
				 "loses it on game exit, and the menu opens on the wrong slot next launch."),
			CodeOnly.Contains(TEXT("SaveGameData"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The persisted key must be the spot's CinematicRow.Priority — that's
	// the identity the lookup machinery uses (ActiveSpotPriority is a
	// priority value; GetCurrentSpot matches on Priority). A re-impl that
	// persists an array index or a UObject pointer breaks the restoration:
	// pointers don't survive process restart, indices drift the moment the
	// data registry re-orders spots, but priorities are stable identifiers
	// owned by the FBmrCinematicRow asset.
	if (!TestTrue(
			TEXT("B9: MarkCinematicAsSeen must persist the spot's CinematicRow.Priority (or "
				 "an equivalent identity derived from it) as the saved key. The cycling "
				 "machinery indexes spots by Priority — ActiveSpotPriority is a priority "
				 "value, and GetCurrentSpot matches priorities against MainMenuSpots — so "
				 "the saved marker must follow the same schema. Saving an array index drifts "
				 "the moment the data registry re-orders spots; saving a UObject pointer "
				 "doesn't survive process restart."),
			CodeOnly.Contains(TEXT("Priority"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("CinematicRow"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 12 — Background async-load entry point: OnGameFeatureInitialize binds
//           FBmrCinematicRow into the data-registry async loader; the
//           subscribe handler (OnCinematicRowsChanged) is what eventually
//           drives the non-active spots' cinematic resolves (B5 / evaluator
//           B36). And the Deinitialize counterpart must unbind the loader
//           and reset ActiveSpotPriority / MainMenuSpots so a re-entered
//           menu doesn't leak stale state or double-registered callbacks
//           into the next session (evaluator B37).
//
// The spec's "Load the non-active spots' cinematics in the background"
// requirement is satisfied by the data registry's bind-and-load mechanism:
// the subsystem asks the registry to async-resolve every FBmrCinematicRow
// soft pointer and to call OnCinematicRowsChanged when the batch finishes.
// That callback then drives ReinitializeCinematicData →
// InitMasterSequencePlayer on every registered spot. Without the bind,
// the registry never notifies the subsystem, AreAllSpotsLoaded never
// observes the resolved pointers, the ready signal never fires, and the
// menu stalls on its open frame.
//
// Test 6 covers the *negative* — no eager LoadSynchronous bulk-loads at
// AddNewMainMenuSpot or InitMasterSequencePlayer. This test covers the
// *positive* — the bind-and-load wiring that is the actual background-
// load entry point. Together they pin the spec's async-load contract.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmSpots_GameFeatureLifecycleBindsAsyncLoad,
	"Bomber.NMMSpots.GameFeatureLifecycleBindsAsyncLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmSpots_GameFeatureLifecycleBindsAsyncLoad::RunTest(const FString& Parameters)
{
	using namespace NmmSpotsTests;

	const FString Source = LoadProjectFile(this, SpotsSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString InitBody =
		ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::OnGameFeatureInitialize_Implementation"));
	const FString InitCode = StripComments(InitBody);

	// The stub body is `Super::OnGameFeatureInitialize_Implementation();` only
	// — ~55 chars. The canonical body subscribes to two global messages plus
	// calls BindAndLoad on the data registry — well over 200 chars. A length
	// gate fails the stub loudly; `!Body.IsEmpty()` would pass vacuously
	// because `{ Super::...; }` is a non-empty string.
	if (!TestTrue(
			TEXT("B5: NMMSpotsSubsystem.cpp must define OnGameFeatureInitialize_Implementation "
				 "with a non-trivial body. A Super-only stub never registers the background-"
				 "load callback with the data registry — the non-active spots' cinematic rows "
				 "never resolve, AreAllSpotsLoaded never flips, and the menu stalls on its open "
				 "frame waiting for the ready signal that the spec ties to the registry's batch "
				 "load broadcast."),
			InitCode.Len() > 150))
	{
		return false;
	}

	// The body must bind FBmrCinematicRow into the data-registry's async load
	// machinery via UDalRegistrySubsystem::BindAndLoad<FBmrCinematicRow>. That
	// call is the registered entry point: the registry kicks off the async
	// resolve of every FBmrCinematicRow's soft pointers, then fires the
	// supplied callback (OnCinematicRowsChanged) when the batch completes.
	// Without this bind, the background loads simply never start and the
	// only spots that ever reach MasterPlayer-valid state are those whose
	// rows happen to be already resolved at AddNewMainMenuSpot time (the
	// late-spot fast path) — a fragile race the spec forbids.
	if (!TestTrue(
			TEXT("B5: OnGameFeatureInitialize must call BindAndLoad on the data registry for "
				 "FBmrCinematicRow — this is the registered entry point that triggers the "
				 "background async resolve of every cinematic row's soft pointers. Without it, "
				 "background loading never starts; only spots whose rows are coincidentally "
				 "already resolved at registration time ever reach a playable state, and the "
				 "ready signal stalls on the rest forever — the spec's 'load non-active spots' "
				 "cinematics in the background' contract goes unmet."),
			InitCode.Contains(TEXT("BindAndLoad"), ESearchCase::CaseSensitive)
				&& InitCode.Contains(TEXT("FBmrCinematicRow"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The callback bound into the registry must be OnCinematicRowsChanged —
	// the function the spec ties the row-loaded broadcast to. A re-impl that
	// binds a different (or no) callback never re-triggers the per-spot
	// init chain when the registry finishes its batch load.
	if (!TestTrue(
			TEXT("B5: OnGameFeatureInitialize must register OnCinematicRowsChanged as the "
				 "data-registry callback. The registry fires this back once the FBmrCinematicRow "
				 "batch finishes its async resolve; that callback drives ReinitializeCinematicData "
				 "→ InitMasterSequencePlayer on every spot. Binding a different (or no) callback "
				 "leaves the per-spot init chain dormant after the batch load completes."),
			InitCode.Contains(TEXT("OnCinematicRowsChanged"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The init must also subscribe the subsystem's menu-state and game-state
	// handlers to the relevant global messages so the cycling / in-game spot
	// swap paths actually receive state transitions. A body that binds the
	// registry but never listens for MenuStateChanged drops every cycle event
	// on the floor — the state machine fires its broadcasts and the subsystem's
	// OnNewMainMenuStateChanged handler is never called.
	if (!TestTrue(
			TEXT("B5: OnGameFeatureInitialize must subscribe to the MenuStateChanged global "
				 "message via OnNewMainMenuStateChanged. Without this listener, the subsystem "
				 "never observes cycle / in-game-swap state transitions and the carousel freezes "
				 "on whatever spot was last broadcast-as-ready."),
			InitCode.Contains(TEXT("OnNewMainMenuStateChanged"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	// The Deinitialize counterpart must unbind from the data registry and
	// reset the per-session state. Without these clears, re-entering the
	// menu in the same process (main menu → in-game → return to menu)
	// re-uses stale ActiveSpotPriority pointing at a destroyed spot, and
	// re-subscribes the data-registry callback on top of the already-
	// registered binding — duplicate callbacks on every batch load.
	const FString DeinitBody =
		ExtractMemberBody(Source, TEXT("UNMMSpotsSubsystem::OnGameFeatureDeinitialize_Implementation"));
	const FString DeinitCode = StripComments(DeinitBody);

	if (!TestTrue(
			TEXT("B5: NMMSpotsSubsystem.cpp must define OnGameFeatureDeinitialize_Implementation "
				 "with a non-trivial body. A Super-only stub leaks the data-registry binding "
				 "across menu sessions — re-entering the menu doubles up OnCinematicRowsChanged "
				 "registrations and the registry fires the callback once per stale binding every "
				 "batch load."),
			DeinitCode.Len() > 100))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B5: OnGameFeatureDeinitialize must unbind from the data registry "
				 "(UnbindFromDataRegistryLoad) and stop listening for all global messages. "
				 "Leaking the binding across menu sessions stacks OnCinematicRowsChanged "
				 "registrations; the registry then fires the callback once per stale binding "
				 "every batch load — quickly devolves to N-times reinitialisation per load."),
			DeinitCode.Contains(TEXT("UnbindFromDataRegistryLoad"), ESearchCase::CaseSensitive)
				&& DeinitCode.Contains(TEXT("StopListeningForAllGlobalMessages"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	if (!TestTrue(
			TEXT("B5: OnGameFeatureDeinitialize must reset the per-session browse state — "
				 "MainMenuSpots.Empty(), ActiveSpotPriority = INDEX_NONE. Without these clears, "
				 "the next menu open inherits a stale ActiveSpotPriority pointing at a destroyed "
				 "spot (the previous session's MainMenuSpots pointers are dangling by then) and "
				 "the very first carousel lookup either crashes the ensureMsgf inside "
				 "GetCurrentSpot or returns a stale spot."),
			DeinitCode.Contains(TEXT("MainMenuSpots"), ESearchCase::CaseSensitive)
				&& DeinitCode.Contains(TEXT("ActiveSpotPriority"), ESearchCase::CaseSensitive)
				&& DeinitCode.Contains(TEXT("INDEX_NONE"), ESearchCase::CaseSensitive)))
	{
		return false;
	}

	return true;
}
