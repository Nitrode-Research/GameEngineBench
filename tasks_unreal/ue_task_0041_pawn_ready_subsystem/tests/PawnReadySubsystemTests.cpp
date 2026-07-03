// Copyright (c) 2026 GameDevBench. UBmrPawnReadySubsystem automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/Subsystems/BmrPawnReadySubsystem.cpp
//
// UBmrPawnReadySubsystem is the per-world readiness arbiter for ABmrPawn.
// It tracks three independent signals per pawn — pawn-possessed (from the
// player controller's OnPossess path), player-state-init (from the player
// state's init path), and pawn-added-to-grid (from the pawn's map component
// joining the generated map) — and broadcasts two global messages (a generic
// Player_PawnReady for every ready pawn, plus a local-only Player_LocalPawnReady
// for the human-controlled local pawn) once all three signals have arrived in
// any order. The possession check is network-topology-aware: client-world bots,
// remote autonomous proxies, and locally-controlled-but-not-yet-possessed pawns
// each need a different answer to "is this pawn possessed?" — and only one of
// the six rows in the truth table returns false.
//
// Why source-inspection coverage:
//   Direct runtime coverage needs a hydrated ABmrGeneratedMap (the singleton
//   asserts on access), an ABmrPawn instantiated by ABmrGameMode with its
//   Mover-2.0 BackendLiaisonComp registered against a network driver, an
//   ABmrPlayerState replicated through the same driver, an ABmrPlayerController
//   with a populated cheat manager and possessed pawn, the GlobalMessageSubsystem
//   set up to capture Player_PawnReady / Player_LocalPawnReady broadcasts, and
//   — to exercise rows 4, 5, 6 of the IsPawnPossessed truth table — a multi-
//   client PIE session where the test code can observe a non-authority world
//   from this client's viewpoint while a separate authority world holds the
//   server-side state. None of that exists as a reusable PIE harness in this
//   project; reproducing it inside an automation test would dwarf the subsystem
//   surface itself, and the private FOnReadyData struct cannot be inspected
//   from outside translation-unit scope to assert on the marked flags.
//
//   The spec and the evaluator notes pin down the load-bearing behaviors that
//   separate a working impl from a plausibly-working one:
//     B1–B3: Each of the three Broadcast_* entry points marks its specific
//            field in FOnReadyData (bIsPossessed / PlayerState weak-ptr /
//            bIsAddedOnGeneratedMap) and then calls the internal broadcaster.
//     B4–B7: IsPawnPossessed walks the six-row truth table. Row 4 (client-world
//            bot, !possessed && !locallyControlled && bIsBot) returns true;
//            Row 5 (local autonomous proxy not yet possessed) is the *only*
//            false-returning row; Row 6 (remote other-client player) returns
//            true. IsReady must delegate possession-checking to IsPawnPossessed
//            rather than reading bIsPossessed directly — the named failure mode
//            is "client-world bots and remote players never become ready."
//     B8:    Each Broadcast_* entry point early-returns when IsReady(&Pawn) is
//            already true, so a late signal arriving after the pawn is ready
//            doesn't re-broadcast.
//     B9:    Each Broadcast_* entry point gates on UUtilsLibrary::HasWorldBegunPlay()
//            so pre-BeginPlay signals don't fire before listeners exist.
//     B10:   TryBroadcastOnReady_Internal emits BOTH Player_PawnReady (for every
//            ready pawn) AND Player_LocalPawnReady (only when the pawn is
//            IsLocallyControlled() && IsPlayerControlled()), so viewmodels can
//            distinguish the local pawn from other pawns in the same broadcast.
//     B11:   The FOnReadyData struct in the header stores the pawn as a
//            TWeakObjectPtr — a strong pointer would keep destroyed pawns alive
//            in GC across map transitions.
//     B12:   Deinitialize calls Reset (which empties OnReadyHandles), so stale
//            records don't leak across map transitions.
//
//   Each test below targets one of these behavior groups, scanning the
//   comment-stripped body of the relevant member function so that anti-pattern
//   descriptions written in canonical comments ("Skip if already ready", "Is
//   host, both players and bots that should be possessed", "Matches Row 4: Bot
//   (In Client World), where controller does not exist by design", "Broadcast
//   Local Pawn Ready event only for locally-controlled player-controlled pawns")
//   cannot satisfy the assertions. A final reflection test confirms the
//   subsystem's UClass exists and is a UWorldSubsystem so a rename or base-class
//   regression doesn't silently break the world-subsystem registration.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "Subsystems/WorldSubsystem.h"

// Bomber
#include "Subsystems/BmrPawnReadySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogPawnReadySubsystemTests, Log, All);

namespace PawnReadySubsystemTests
{
	static const TCHAR* SubsystemCppRelPath =
		TEXT("Source/Bomber/Private/Subsystems/BmrPawnReadySubsystem.cpp");
	static const TCHAR* SubsystemHeaderRelPath =
		TEXT("Source/Bomber/Public/Subsystems/BmrPawnReadySubsystem.h");

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
	// anti-pattern descriptions written in canonical comments. The solution's
	// own comments include the entire IsPawnPossessed truth table in a block
	// comment ("| Row | Entity | Details | Authority | Possession | Locally
	// Controlled | Result |"), the words "Bot (In Client World)", "Player -
	// Local Client (Autonomous Proxy)", "Player - Other Client or Remote Host",
	// and the phrase "Skip if already ready" appearing three times — every one
	// of these would pass a naive substring test even against a stub body.
	// String literals are preserved so ensureMsgf format strings and tag names
	// inside TEXT() still scan.
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

	// Brace-depth walker. Counts braces only, not parens, so nested initialiser
	// lists / lambdas (FindByPredicate's lambda captures the pawn by reference)
	// don't confuse the scan.
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

	static int32 FindOffsetIn(const FString& Body, const FString& Needle)
	{
		return Body.Find(Needle, ESearchCase::CaseSensitive);
	}
} // namespace PawnReadySubsystemTests

// ---------------------------------------------------------------------------
// Test 1 — B1+B2+B3: each of the three Broadcast_* entry points marks the
//          field for its specific signal and then drives the internal
//          broadcaster.
//
// The three signals arrive in unpredictable order over the network; the
// subsystem's contract is that *each* entry point writes its own field on
// the per-pawn record (bIsPossessed for OnPawnPossessed, the PlayerState
// weak-ptr for OnPlayerStateInit, bIsAddedOnGeneratedMap for OnPawnAdded)
// and then runs TryBroadcastOnReady_Internal to check whether the AND of
// all three is now true. A re-impl that marks the wrong field — or marks
// none — silently leaves the pawn stuck unready forever; nothing else in
// the codebase calls TryBroadcastOnReady_Internal, so the three writes are
// the only thing that can drive the ready broadcast.
//
// Broadcast_OnPlayerStateInit has the extra wrinkle that the player state
// holds the pawn pointer (replicated state on the controller pair), so the
// entry point has to extract the pawn via PlayerState.GetPawn<ABmrPawn>()
// before it can find-or-add a record. A re-impl that uses GetPawn<APawn>()
// or skips the templated extraction reads back a base APawn pointer that
// can't drive the predicate match in OnReadyHandles.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_ThreeSignalsMarkOwnField,
	"Bomber.PawnReadySubsystem.ThreeSignalsMarkOwnField",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_ThreeSignalsMarkOwnField::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// --- Broadcast_OnPawnPossessed marks bIsPossessed = true ---
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnPossessed"));
		if (!TestTrue(
				TEXT("B1: BmrPawnReadySubsystem.cpp must define Broadcast_OnPawnPossessed with a "
					 "body — the start/ stub is empty; the field-mark + internal broadcast is what "
					 "was stripped."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);

		TestTrue(
			TEXT("B1: Broadcast_OnPawnPossessed must mark FOnReadyData::bIsPossessed = true on the "
				 "per-pawn record. This is the only entry point that signals the OnPossess path; "
				 "without the write, the possession flag stays false and (combined with the truth "
				 "table) the host pawn — the simplest case — never becomes ready."),
			CodeOnly.Contains(TEXT("bIsPossessed"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("true"), ESearchCase::CaseSensitive));

		TestTrue(
			TEXT("B1: Broadcast_OnPawnPossessed must reach into FindOrAdd to obtain or create the "
				 "per-pawn record before writing the flag. There is no other accessor in the file "
				 "that returns a mutable FOnReadyData; a re-impl that constructs a local "
				 "FOnReadyData and writes the flag there would discard the write."),
			CodeOnly.Contains(TEXT("FindOrAdd"), ESearchCase::CaseSensitive));

		TestTrue(
			TEXT("B1: Broadcast_OnPawnPossessed must call TryBroadcastOnReady_Internal after the "
				 "field write — the field-mark alone doesn't fire the global message; the internal "
				 "broadcaster is the only thing that does. The spec: 'each entry point marks its "
				 "field in the pawn's readiness record, then checks whether all conditions are now "
				 "satisfied.'"),
			CodeOnly.Contains(TEXT("TryBroadcastOnReady_Internal"), ESearchCase::CaseSensitive));
	}

	// --- Broadcast_OnPlayerStateInit assigns the PlayerState weak-ptr ---
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Broadcast_OnPlayerStateInit"));
		if (!TestTrue(
				TEXT("B2: BmrPawnReadySubsystem.cpp must define Broadcast_OnPlayerStateInit with a "
					 "body — the start/ stub is empty."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);

		TestTrue(
			TEXT("B2: Broadcast_OnPlayerStateInit must extract the pawn via "
				 "PlayerState.GetPawn<ABmrPawn>(). The player state owns the controller-pair's "
				 "pawn pointer; the entry point parameter is the PlayerState, but FindOrAdd takes "
				 "the pawn. Without the templated GetPawn, the entry point can't find the per-pawn "
				 "record this signal applies to."),
			CodeOnly.Contains(TEXT("GetPawn"), ESearchCase::CaseSensitive)
				&& (CodeOnly.Contains(TEXT("<ABmrPawn>"), ESearchCase::CaseSensitive)
					|| CodeOnly.Contains(TEXT("< ABmrPawn >"), ESearchCase::CaseSensitive)));

		TestTrue(
			TEXT("B2: Broadcast_OnPlayerStateInit must write the PlayerState weak-ptr on the "
				 "per-pawn record (FindOrAdd(*Pawn).PlayerState = &PlayerState or equivalent). "
				 "IsReady reads FoundHandle->PlayerState.IsValid() as one of the ready-conjuncts; "
				 "without this write the player state stays null on the record and the pawn never "
				 "becomes ready even when the other two signals have fired."),
			CodeOnly.Contains(TEXT("PlayerState"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("FindOrAdd"), ESearchCase::CaseSensitive));

		TestTrue(
			TEXT("B2: Broadcast_OnPlayerStateInit must call TryBroadcastOnReady_Internal after the "
				 "field write. As with OnPawnPossessed, the field-mark alone never fires the global "
				 "message — the internal broadcaster is the gate."),
			CodeOnly.Contains(TEXT("TryBroadcastOnReady_Internal"), ESearchCase::CaseSensitive));
	}

	// --- Broadcast_OnPawnAdded marks bIsAddedOnGeneratedMap = true ---
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnAdded"));
		if (!TestTrue(
				TEXT("B3: BmrPawnReadySubsystem.cpp must define Broadcast_OnPawnAdded with a body — "
					 "the start/ stub is empty."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);

		TestTrue(
			TEXT("B3: Broadcast_OnPawnAdded must mark FOnReadyData::bIsAddedOnGeneratedMap = true. "
				 "This is the only entry point that signals the map-component-joined-grid path; "
				 "without the write the grid-arrival flag stays false on every record and IsReady "
				 "(which conjuncts bIsAddedOnGeneratedMap) returns false for every pawn forever."),
			CodeOnly.Contains(TEXT("bIsAddedOnGeneratedMap"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("true"), ESearchCase::CaseSensitive));

		TestTrue(
			TEXT("B3: Broadcast_OnPawnAdded must reach into FindOrAdd and run "
				 "TryBroadcastOnReady_Internal, matching the structure of the other two entry "
				 "points. The three Broadcast_* methods are symmetric — same FindOrAdd / mark / "
				 "broadcast flow — so the readiness check fires regardless of which signal "
				 "completed the AND-set."),
			CodeOnly.Contains(TEXT("FindOrAdd"), ESearchCase::CaseSensitive)
				&& CodeOnly.Contains(TEXT("TryBroadcastOnReady_Internal"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B9: each Broadcast_* entry point gates on
//          UUtilsLibrary::HasWorldBegunPlay() so pre-BeginPlay signals are
//          dropped.
//
// The spec's named failure mode: "Processing signals before BeginPlay —
// fires before listeners exist." The three signal sources (player
// controller possession, player state init, map component add) can all
// fire during world initialization, *before* the global message subsystem's
// listeners are wired up. Any broadcast that lands before HasWorldBegunPlay
// flips true is broadcast into the void — the cached-message replay would
// preserve it for the first new subscriber, but the GAS-event payload is a
// transient FGameplayEventData and gets dropped on the floor.
//
// HasWorldBegunPlay is the project's gate of choice (the alternative,
// World->HasBegunPlay(), exists but UUtilsLibrary wraps it with the play-
// world check; substituting the raw World call would slip through PIE-
// preview worlds where HasBegunPlay flips true earlier than the play world
// is fully wired).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_BroadcastsGateOnHasWorldBegunPlay,
	"Bomber.PawnReadySubsystem.BroadcastsGateOnHasWorldBegunPlay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_BroadcastsGateOnHasWorldBegunPlay::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const TArray<FString> Entrypoints = {
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnPossessed"),
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPlayerStateInit"),
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnAdded"),
	};

	for (const FString& Signature : Entrypoints)
	{
		const FString Body = ExtractMemberBody(Source, Signature);
		if (!TestTrue(
				FString::Printf(
					TEXT("B9: %s must have a body for the HasWorldBegunPlay guard to live in."),
					*Signature),
				!Body.IsEmpty()))
		{
			return false;
		}

		const FString CodeOnly = StripComments(Body);

		TestTrue(
			FString::Printf(
				TEXT("B9: %s must gate on UUtilsLibrary::HasWorldBegunPlay() (positive guard or "
					 "negative early-return). The named failure mode: 'Processing signals before "
					 "BeginPlay — fires before listeners exist.' Three signal sources can fire "
					 "during world init; a broadcast that lands before listeners are wired drops "
					 "the GAS event payload silently."),
				*Signature),
			CodeOnly.Contains(TEXT("HasWorldBegunPlay"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B8: each Broadcast_* entry point early-returns when the pawn is
//          already ready, so a late signal doesn't re-broadcast.
//
// The spec's named failure mode: "Re-broadcasting when a late signal
// arrives after the pawn is already ready." The three signals can arrive
// in any order; once all three have landed, IsReady returns true and the
// pawn-ready broadcast fires. A fourth signal — duplicate possession, a
// repeated map-add, a re-fired player-state init — would, without a guard,
// run TryBroadcastOnReady_Internal again, and the global message would fire
// a second time. Downstream listeners (viewmodels, sound cues, gameplay
// systems) react to each broadcast; a second Player_PawnReady is a logic
// error that re-runs every on-ready handler.
//
// The guard is symmetric across the three entry points: check IsReady
// against the pawn (Broadcast_OnPlayerStateInit checks the extracted pawn,
// the other two check the parameter address). The canonical body wraps the
// field write + broadcaster call in `if (HasWorldBegunPlay && !IsReady(...))`
// or returns early before either.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_BroadcastsSkipWhenAlreadyReady,
	"Bomber.PawnReadySubsystem.BroadcastsSkipWhenAlreadyReady",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_BroadcastsSkipWhenAlreadyReady::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const TArray<FString> Entrypoints = {
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnPossessed"),
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPlayerStateInit"),
		TEXT("UBmrPawnReadySubsystem::Broadcast_OnPawnAdded"),
	};

	for (const FString& Signature : Entrypoints)
	{
		const FString Body = ExtractMemberBody(Source, Signature);
		if (!TestTrue(
				FString::Printf(
					TEXT("B8: %s must have a body for the IsReady guard to live in."),
					*Signature),
				!Body.IsEmpty()))
		{
			return false;
		}

		const FString CodeOnly = StripComments(Body);

		TestTrue(
			FString::Printf(
				TEXT("B8: %s must consult IsReady against the pawn before doing any field-mark / "
					 "broadcast. The named failure mode: 'Re-broadcasting when a late signal "
					 "arrives after the pawn is already ready.' Without this guard a duplicate "
					 "signal re-fires every Player_PawnReady listener (viewmodels, sound, gameplay) "
					 "as if the pawn just became ready again."),
				*Signature),
			CodeOnly.Contains(TEXT("IsReady"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B4+B5+B6+B7: IsPawnPossessed walks the six-row truth table and
//          returns the topology-aware answer, NOT a bare bIsPossessed read.
//
// The spec's truth table (reproduced from the solution's block comment):
//   Row 1: Player local/host, has authority, possessed, locally controlled  → true
//   Row 2: Bot in host world, has authority, possessed, not locally ctrl    → true
//   Row 3: Player local client (autonomous proxy), possessed, locally ctrl  → true
//   Row 4: Bot in client world, no authority, NOT possessed, not locally    → true  ★
//   Row 5: Player local client (autonomous proxy), NOT possessed, locally   → false ★
//   Row 6: Player other client / remote host, NOT possessed, not locally    → true  ★
//
// The three starred rows are where the topology gets non-trivial:
//   B4 (Row 4): client-world bots arrive as replicated pawns; their AI
//     controller only exists on the server, so bIsPossessed is false on
//     this client and IsLocallyControlled() is false too. A naive
//     `return bIsPossessed` returns false here and the bot never broadcasts
//     ready on clients — viewmodels show the bot in an "uninitialized"
//     state forever.
//   B5 (Row 5): the local player has joined the session and the engine has
//     spawned the autonomous proxy, but the server's possession hasn't
//     replicated down yet. bIsPossessed is false, IsLocallyControlled() is
//     true (because the autonomous proxy IS the local view). This is the
//     ONLY row that legitimately must wait — the broadcast can't fire
//     until the possession message arrives. Any IsPawnPossessed that
//     returns true here over-eagerly fires Player_PawnReady before the
//     local player's input is wired.
//   B6 (Row 6): a remote peer's player has been replicated to this client
//     as a non-locally-controlled non-bot. Their possession lives on their
//     own peer; we will never see bIsPossessed flip true on this client's
//     copy. A `bIsPossessed`-only check leaves remote players permanently
//     unready from this client's viewpoint.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_IsPawnPossessedHandlesTopologyRows,
	"Bomber.PawnReadySubsystem.IsPawnPossessedHandlesTopologyRows",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_IsPawnPossessedHandlesTopologyRows::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::IsPawnPossessed"));
	if (!TestTrue(
			TEXT("B7: BmrPawnReadySubsystem.cpp must define IsPawnPossessed with a body — the "
				 "start/ stub returns false unconditionally, which collapses every row of the truth "
				 "table to 'never possessed' and no pawn ever becomes ready."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Rows 1, 2, 3 share `bIsPossessed == true` — once the per-pawn record
	// has bIsPossessed marked, every other condition is moot. The canonical
	// body short-circuits with `if (FoundHandle.bIsPossessed) { return true; }`.
	TestTrue(
		TEXT("B4–B6: IsPawnPossessed must short-circuit when FoundHandle.bIsPossessed is true — "
			 "rows 1, 2, 3 of the truth table (host player, host bot, local client autonomous "
			 "proxy that has been possessed) all collapse to this case. Without the short-circuit, "
			 "a possessed local player on the host might still fall through to the topology "
			 "branches and return the wrong answer depending on bot/locally-controlled state."),
		CodeOnly.Contains(TEXT("bIsPossessed"), ESearchCase::CaseSensitive));

	// The truth table is reached by reading three boolean topology axes: pawn
	// authority, player-state bot-ness, pawn locally-controlled. A re-impl
	// that doesn't read all three can't distinguish Row 4 from Row 5 from
	// Row 6.
	TestTrue(
		TEXT("B4–B6: IsPawnPossessed must read FoundHandle.Pawn->HasAuthority() to distinguish "
			 "authoritative-world rows (1, 2) from non-authoritative-world rows (3, 4, 5, 6). "
			 "Without this branch, the implementation can't separate the host's pawns (which "
			 "should be readable through bIsPossessed alone) from client-world pawns (which "
			 "need topology-aware fallbacks)."),
		CodeOnly.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: IsPawnPossessed must read FoundHandle.PlayerState->IsABot() to identify Row 4 "
			 "(client-world bots — possessed only on the server, never on this client). The named "
			 "failure mode: 'Bots on clients: AI controllers only exist on the server. Client-"
			 "world bots arrive as replicated pawns without local possession — they must still be "
			 "treated as possessed and count toward readiness.' Without the IsABot read, Row 4 "
			 "collapses into Row 6 (or the unhandled-condition assert) and bots stay unready."),
		CodeOnly.Contains(TEXT("IsABot"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B5–B6: IsPawnPossessed must read FoundHandle.Pawn->IsLocallyControlled() to "
			 "distinguish Row 5 (local autonomous proxy not yet possessed — the ONE row that "
			 "must wait) from Row 6 (remote other-client player — already ready from this "
			 "client's viewpoint). Without IsLocallyControlled, the two rows collapse and either "
			 "the local player broadcasts ready before possession arrives, or remote players "
			 "never broadcast ready at all."),
		CodeOnly.Contains(TEXT("IsLocallyControlled"), ESearchCase::CaseSensitive));

	// Row 4 — bot in client world — returns true. The signal: IsABot() &&
	// !IsLocallyControlled() && !HasAuthority(). The canonical body has an
	// explicit `if (bIsBot && !bIsLocallyControlled) { return true; }` after
	// the authority short-circuit.
	TestTrue(
		TEXT("B4: IsPawnPossessed must return true for client-world bots (Row 4). The canonical "
			 "encoding combines IsABot() && !IsLocallyControlled() (no authority is implicit from "
			 "having fallen past the authority short-circuit). A re-impl that omits this branch "
			 "leaves bots permanently unready on every non-authoritative client — the named "
			 "failure mode the spec calls out specifically."),
		(CodeOnly.Contains(TEXT("bIsBot"), ESearchCase::CaseSensitive)
		 || CodeOnly.Contains(TEXT("IsABot"), ESearchCase::CaseSensitive))
			&& CodeOnly.Contains(TEXT("return true"), ESearchCase::CaseSensitive));

	// Row 5 — local autonomous proxy not yet possessed — is the ONLY false-
	// returning row. The signal: IsLocallyControlled() && !bIsPossessed (no
	// authority is implicit). The canonical body has an explicit
	// `if (bIsLocallyControlled && !FoundHandle.bIsPossessed) { return false; }`.
	TestTrue(
		TEXT("B6: IsPawnPossessed must return false for the local autonomous proxy that has not "
			 "yet been possessed by the server (Row 5). This is the ONLY row in the truth table "
			 "that returns false; the spec calls it out as 'the only case that must genuinely "
			 "wait for the server's possession to replicate down.' Without this branch, the "
			 "local pawn broadcasts ready before possession arrives and Player_LocalPawnReady "
			 "fires against an unpossessed proxy."),
		CodeOnly.Contains(TEXT("return false"), ESearchCase::CaseSensitive));

	// Row 6 — remote other-client player — returns true. The signal:
	// !IsABot() && !IsLocallyControlled() && !HasAuthority(). A naive
	// `bIsPossessed`-only check or a re-impl that bundles Row 6 with the
	// unhandled-condition assert leaves remote players unready from every
	// peer's viewpoint.
	TestTrue(
		TEXT("B5: IsPawnPossessed must return true for remote other-client players (Row 6) — "
			 "!IsABot() && !IsLocallyControlled() on a non-authoritative client. The spec: "
			 "'Remote other-client players: possessed on their controlling peer — from this "
			 "client's viewpoint they are ready.' A `bIsPossessed`-only check never flips true "
			 "for them on this client (the possession state lives on their own peer), so they "
			 "stay unready forever — the named failure mode 'client-world bots and remote players "
			 "never become ready.'"),
		CodeOnly.Contains(TEXT("return true"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B7: IsReady delegates to IsPawnPossessed and does NOT read
//          FoundHandle->bIsPossessed directly as the possession signal.
//
// The spec's named failure mode: "Checking bIsPossessed directly for the
// possession condition — client-world bots and remote players never become
// ready." IsReady is the conjunctive check (pawn valid && player state
// valid && bIsAddedOnGeneratedMap && <possession>). The <possession>
// position is where the topology-aware decision lives; reading
// FoundHandle->bIsPossessed there would collapse the whole truth table to
// Row 1 (i.e. "only possessed-on-this-peer pawns are ready"), which is
// exactly what Rows 4 and 6 are designed to avoid.
//
// The canonical IsReady reads the four conjuncts as:
//   FoundHandle->Pawn.IsValid() && FoundHandle->PlayerState.IsValid()
//   && FoundHandle->bIsAddedOnGeneratedMap && IsPawnPossessed(*FoundHandle)
// — the IsPawnPossessed call is the load-bearing piece this test pins.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_IsReadyDelegatesToIsPawnPossessed,
	"Bomber.PawnReadySubsystem.IsReadyDelegatesToIsPawnPossessed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_IsReadyDelegatesToIsPawnPossessed::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// The pawn-overload of IsReady is the one that walks the conjunctive
	// check; the PlayerState-overload is a thin forwarder. Pin the pawn
	// overload.
	const FString Body = ExtractMemberBody(
		Source, TEXT("UBmrPawnReadySubsystem::IsReady(const ABmrPawn"));
	if (!TestTrue(
			TEXT("B7: BmrPawnReadySubsystem.cpp must define IsReady(const ABmrPawn*) with a body — "
				 "the start/ stub returns false unconditionally, which makes every Broadcast_* "
				 "entry point's `!IsReady(...)` guard always fire and the already-ready skip "
				 "(test 3) silently re-broadcasts."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Required conjunct: bIsAddedOnGeneratedMap — the spec calls this out as
	// one of the three signals that must all be true before ready.
	TestTrue(
		TEXT("B7: IsReady must check FoundHandle->bIsAddedOnGeneratedMap as one of the "
			 "conjuncts. The spec defines readiness as three signals all true — possession, "
			 "player-state init, and map-add. Dropping the map-add conjunct would let pawns "
			 "broadcast ready before the cell grid knows about them, and SnapActorOnLevel calls "
			 "downstream would assert on a missing cell."),
		CodeOnly.Contains(TEXT("bIsAddedOnGeneratedMap"), ESearchCase::CaseSensitive));

	// Load-bearing assertion: the possession conjunct must come from
	// IsPawnPossessed, not from a raw bIsPossessed read. The named failure
	// mode is exactly this.
	TestTrue(
		TEXT("B7: IsReady must delegate the possession conjunct to IsPawnPossessed(*FoundHandle), "
			 "NOT read FoundHandle->bIsPossessed directly. The named failure mode: 'Checking "
			 "bIsPossessed directly for the possession condition — client-world bots and remote "
			 "players never become ready.' IsPawnPossessed walks the topology truth table that "
			 "covers rows 4 and 6 (client-world bots / remote players) where bIsPossessed is "
			 "permanently false on this client; bypassing it collapses every non-authority "
			 "non-locally-controlled pawn to 'never ready' forever."),
		CodeOnly.Contains(TEXT("IsPawnPossessed"), ESearchCase::CaseSensitive));

	// PlayerState weak-ptr validity is the other strong-pointer-equivalent
	// the spec calls out — without it, a destroyed player state's stale
	// record would test ready against a dangling pointer inside
	// IsPawnPossessed (which dereferences PlayerState to read IsABot()).
	TestTrue(
		TEXT("B7: IsReady must validate the PlayerState weak-ptr (IsValid()) before passing the "
			 "record to IsPawnPossessed — IsPawnPossessed dereferences PlayerState to read "
			 "IsABot(), and a destroyed-but-still-referenced player state crashes the read. The "
			 "weak-ptr's IsValid() guard is what makes 'destroyed pawns leaked records' (B11) "
			 "safe to leave in OnReadyHandles until the next Reset()."),
		CodeOnly.Contains(TEXT("IsValid"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B10: TryBroadcastOnReady_Internal emits BOTH Player_PawnReady
//          (for every ready pawn) AND Player_LocalPawnReady (only for the
//          IsLocallyControlled() && IsPlayerControlled() pawn).
//
// The spec: "Broadcast Player_PawnReady for all pawns. Additionally
// broadcast Player_LocalPawnReady only for pawns that are locally
// controlled by a human player — this lets viewmodels distinguish the
// local player's pawn from other pawns in the same broadcast."
//
// Two distinct named failures live here:
//   * Emitting only Player_PawnReady: viewmodels cannot distinguish the
//     local pawn from other ready pawns (they all get the same tag), so
//     the UI either binds to the first ready pawn it sees (wrong on every
//     non-host peer) or has to walk every PawnReady payload looking for
//     IsLocallyControlled (re-deriving the gating the subsystem should
//     have done once).
//   * Emitting Player_LocalPawnReady without the IsLocallyControlled +
//     IsPlayerControlled gate: every peer's bot and every remote player
//     fires Player_LocalPawnReady too, and the local-pawn-bound viewmodel
//     binds to whichever pawn happens to broadcast last.
//
// The IsPlayerControlled half of the gate is what excludes locally-spawned
// bots in single-player / host mode (bIsLocallyControlled is true for a
// host bot if it's puppeted by a local AI controller, but IsPlayerControlled
// is false). A re-impl that gates only on IsLocallyControlled fires
// Player_LocalPawnReady for the host's bots too.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_TryBroadcastEmitsBothTagsWithLocalGate,
	"Bomber.PawnReadySubsystem.TryBroadcastEmitsBothTagsWithLocalGate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_TryBroadcastEmitsBothTagsWithLocalGate::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UBmrPawnReadySubsystem::TryBroadcastOnReady_Internal"));
	if (!TestTrue(
			TEXT("B10: BmrPawnReadySubsystem.cpp must define TryBroadcastOnReady_Internal with a "
				 "body — the start/ stub is empty; the dual-broadcast logic is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Internal gate: TryBroadcastOnReady_Internal must early-out when the
	// pawn isn't ready yet. This is the secondary IsReady check — the
	// per-entry-point IsReady is the "skip if ALREADY ready" guard;
	// TryBroadcastOnReady_Internal's IsReady is the "fire only if NOW ready"
	// gate. Without it, every signal write triggers a broadcast regardless
	// of the AND-set state.
	TestTrue(
		TEXT("B10: TryBroadcastOnReady_Internal must call IsReady against the pawn before "
			 "broadcasting. The three Broadcast_* entry points write their fields and call this "
			 "method; until all three writes have landed IsReady is false. Without the gate, the "
			 "first signal fires Player_PawnReady before the other two have arrived and "
			 "downstream listeners see a half-initialised pawn."),
		CodeOnly.Contains(TEXT("IsReady"), ESearchCase::CaseSensitive));

	// Required broadcast 1: Player_PawnReady fires for every ready pawn.
	// The canonical encoding builds an FGameplayEventData with EventTag =
	// BmrGameplayTags::Event::Player_PawnReady and calls
	// UGlobalMessageSubsystem::BroadcastGlobalMessage(Payload).
	TestTrue(
		TEXT("B10: TryBroadcastOnReady_Internal must broadcast Player_PawnReady through "
			 "UGlobalMessageSubsystem::BroadcastGlobalMessage. The spec: 'broadcast "
			 "Player_PawnReady for all pawns.' Without this the entire subsystem is silent — "
			 "every ready record stays observable only by polling IsReady, which no caller does."),
		CodeOnly.Contains(TEXT("Player_PawnReady"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive));

	// Required broadcast 2: Player_LocalPawnReady fires gated on
	// IsLocallyControlled && IsPlayerControlled.
	TestTrue(
		TEXT("B10: TryBroadcastOnReady_Internal must also broadcast Player_LocalPawnReady. The "
			 "named failure mode: 'Emitting only Player_PawnReady — viewmodels cannot distinguish "
			 "the local pawn.' A single tag forces every UI viewmodel that needs to bind to the "
			 "local pawn (HUD, ability bar, ammo counter) to walk the payload re-deriving "
			 "IsLocallyControlled on every broadcast — and to ignore bots / remote players."),
		CodeOnly.Contains(TEXT("Player_LocalPawnReady"), ESearchCase::CaseSensitive));

	// The local broadcast must be gated. Both gates required:
	//   IsLocallyControlled() — excludes remote players (Row 6).
	//   IsPlayerControlled()  — excludes local bots (the host's own bots
	//                            are locally-controlled by their AI
	//                            controller, but not player-controlled).
	TestTrue(
		TEXT("B10: The Player_LocalPawnReady broadcast must be gated on Pawn.IsLocallyControlled() "
			 "AND Pawn.IsPlayerControlled(). IsLocallyControlled alone fires the local-pawn tag "
			 "for the host's own bots (they're locally-controlled by their AI controller); "
			 "IsPlayerControlled alone fires it for remote players (they're player-controlled on "
			 "their owning peer). Only the conjunction picks out 'the human player at this "
			 "machine's keyboard.'"),
		CodeOnly.Contains(TEXT("IsLocallyControlled"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("IsPlayerControlled"), ESearchCase::CaseSensitive));

	// Ordering: the Player_PawnReady broadcast must precede the
	// Player_LocalPawnReady broadcast. The local tag is described in the
	// spec as "additionally" — i.e. it adds to the generic tag, it doesn't
	// replace it. A re-impl that fires only Player_LocalPawnReady (skipping
	// the generic tag for the local pawn) would break every listener that
	// binds to Player_PawnReady-for-all-pawns and expects the local pawn in
	// the payload set.
	const int32 GenericIdx = FindOffsetIn(CodeOnly, TEXT("Player_PawnReady"));
	const int32 LocalIdx = FindOffsetIn(CodeOnly, TEXT("Player_LocalPawnReady"));
	if (GenericIdx != INDEX_NONE && LocalIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B10: Player_PawnReady must broadcast BEFORE Player_LocalPawnReady (or be "
				 "unconditional while the local tag is gated). The local tag is described in the "
				 "spec as 'additionally' — it adds to the generic tag, it doesn't replace it. A "
				 "re-impl that fires only Player_LocalPawnReady for the local pawn (skipping the "
				 "generic tag) breaks every listener that expects every ready pawn — including "
				 "the local one — to appear in the Player_PawnReady stream."),
			GenericIdx < LocalIdx);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — B12: Deinitialize calls Reset, and Reset empties OnReadyHandles.
//
// The spec's named failure mode: "Not resetting records on deinitialization
// — stale records leak across map transitions." UWorldSubsystem instances
// are tied to a UWorld; when the world tears down (map transition, PIE
// session end), Deinitialize is called and the subsystem instance is
// destroyed. But the OnReadyHandles TArray holds TWeakObjectPtr<ABmrPawn>
// records whose pawns may have been GC'd or, worse, transferred to a new
// world via streaming. Without an explicit Reset(), a re-impl that relies
// on the destructor alone misses the TWeakObjectPtr cleanup (weak refs in
// dangling FOnReadyData entries persist in the engine's weak-pointer table
// briefly until the next GC sweep), and any code that introspects the
// subsystem before destruction sees zombie records.
//
// Reset is also exposed publicly as a manual cleanup hook; the spec /
// header both call it out as "Perform cleanup." A no-op Reset (or a Reset
// that empties only part of the state) means callers that explicitly
// invoke it for a controlled restart still get stale records.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_DeinitializeCallsResetEmptiesHandles,
	"Bomber.PawnReadySubsystem.DeinitializeCallsResetEmptiesHandles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_DeinitializeCallsResetEmptiesHandles::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// --- Reset() empties OnReadyHandles ---
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Reset"));
		if (!TestTrue(
				TEXT("B12: BmrPawnReadySubsystem.cpp must define Reset with a body — the start/ "
					 "stub is empty, so map transitions leak every record from the prior session."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);

		TestTrue(
			TEXT("B12: Reset must empty OnReadyHandles. The TArray holds the per-pawn records "
				 "that drive every readiness decision; without the empty call, the array carries "
				 "TWeakObjectPtr entries (some dangling, some live) into the next session and the "
				 "first IsReady call after the cross-map transition either returns true against a "
				 "stale record (firing Player_PawnReady for a pawn that no longer exists) or "
				 "asserts inside IsPawnPossessed when it dereferences the weak-ptr."),
			CodeOnly.Contains(TEXT("OnReadyHandles"), ESearchCase::CaseSensitive)
				&& (CodeOnly.Contains(TEXT("Empty"), ESearchCase::CaseSensitive)
					|| CodeOnly.Contains(TEXT("Reset"), ESearchCase::CaseSensitive)
					|| CodeOnly.Contains(TEXT("="), ESearchCase::CaseSensitive)));
	}

	// --- Deinitialize calls Reset (and Super::Deinitialize) ---
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Deinitialize"));
		if (!TestTrue(
				TEXT("B12: BmrPawnReadySubsystem.cpp must define Deinitialize with a body."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);

		TestTrue(
			TEXT("B12: Deinitialize must call Super::Deinitialize() — the UWorldSubsystem base "
				 "does deregistration bookkeeping; skipping it leaves the subsystem-collection "
				 "registry in an inconsistent state across the world's destruction."),
			CodeOnly.Contains(TEXT("Super::Deinitialize"), ESearchCase::CaseSensitive));

		TestTrue(
			TEXT("B12: Deinitialize must call Reset() to clear OnReadyHandles before the world "
				 "tears down. The named failure mode: 'Not resetting records on deinitialization "
				 "— stale records leak across map transitions.' Without the explicit Reset, the "
				 "subsystem destructor leaves the TArray in scope long enough for cross-world "
				 "introspection to observe zombie records."),
			CodeOnly.Contains(TEXT("Reset"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — B11: FOnReadyData stores the pawn (and player state) as
//          TWeakObjectPtr in the header.
//
// The spec's named failure mode: "Holding strong pointers to pawns in the
// readiness record — keeps destroyed pawns alive in GC." OnReadyHandles is
// a TArray of FOnReadyData; UObject UPROPERTY tracking does not reach into
// struct fields declared outside a UPROPERTY macro, and the subsystem's
// FOnReadyData is a private POD struct with no UPROPERTY markup. If the
// Pawn / PlayerState fields were raw TObjectPtr or naked pointers, the
// only thing preventing GC from collecting them would be other references
// in the system — but during cross-map cleanup, the level's outer is
// destroyed first and the record is the last surviving reference. A weak-
// ptr breaks the cycle: the pawn / player state goes to GC, and on next
// access the record's IsValid() check returns false.
//
// The task's header is described as "unchanged" by the strip — i.e. the
// FOnReadyData struct as it ships includes the TWeakObjectPtr fields. This
// test pins that representation against a refactor that mistakenly
// promotes them to strong references.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_OnReadyDataUsesWeakObjectPtr,
	"Bomber.PawnReadySubsystem.OnReadyDataUsesWeakObjectPtr",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_OnReadyDataUsesWeakObjectPtr::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	const FString Header = LoadProjectFile(this, SubsystemHeaderRelPath);
	if (Header.IsEmpty())
	{
		return false;
	}

	// The header defines FOnReadyData inside the subsystem's private section.
	// Extract the struct body specifically — a header-wide TWeakObjectPtr
	// scan would also catch unrelated members and miss a refactor that
	// changes only the FOnReadyData fields.
	const FString StructBody = ExtractMemberBody(Header, TEXT("struct FOnReadyData"));
	if (!TestTrue(
			TEXT("B11: BmrPawnReadySubsystem.h must declare struct FOnReadyData inside the "
				 "subsystem class. Without this struct (or with a rename) the rest of the "
				 "subsystem fails to compile, but a future header refactor that flattens the "
				 "fields onto the subsystem could lose the weak-ptr representation silently — "
				 "this test pins the struct's existence."),
			!StructBody.IsEmpty()))
	{
		return false;
	}

	const FString StructCodeOnly = StripComments(StructBody);

	TestTrue(
		TEXT("B11: FOnReadyData must store the Pawn as a TWeakObjectPtr<ABmrPawn>. The named "
			 "failure mode: 'Holding strong pointers to pawns in the readiness record — keeps "
			 "destroyed pawns alive in GC.' A strong reference (raw pointer, TObjectPtr) without "
			 "a UPROPERTY would either keep the pawn alive past world teardown (preventing the "
			 "level's GC pass from completing) or, with UPROPERTY, drag the readiness record "
			 "into the engine's reference graph and complicate cross-map cleanup."),
		StructCodeOnly.Contains(TEXT("TWeakObjectPtr"), ESearchCase::CaseSensitive)
			&& StructCodeOnly.Contains(TEXT("ABmrPawn"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: FOnReadyData must store the PlayerState as a TWeakObjectPtr<const "
			 "ABmrPlayerState> (or equivalent weak-ptr). IsPawnPossessed dereferences "
			 "PlayerState->IsABot(); a strong-ptr representation would extend the player state's "
			 "lifetime past the controller-pair's destruction, and a raw-ptr representation "
			 "would risk a dangling read after the player state is GC'd. The weak-ptr makes "
			 "the IsValid() guard in IsReady safe."),
		StructCodeOnly.Contains(TEXT("TWeakObjectPtr"), ESearchCase::CaseSensitive)
			&& StructCodeOnly.Contains(TEXT("ABmrPlayerState"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — UClass reflection sanity check.
//
// The subsystem class itself must:
//   * exist in the engine's reflection database under the stable name
//     "BmrPawnReadySubsystem" (UCLASS strips the U prefix in reflection),
//   * be a UWorldSubsystem subclass (the per-world lifecycle is what makes
//     the map-transition Reset matter; a UGameInstanceSubsystem refactor
//     would change the cleanup semantics entirely),
//   * NOT be abstract (a stripped subsystem that compiles but never
//     instantiates would silently leave every Broadcast_* call a no-op
//     because the world's subsystem collection would skip it).
//
// This catches rename / base-class / abstract-flag regressions that source
// scans alone wouldn't notice — the engine reflection database is the
// authoritative view of what the runtime sees.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_ClassReflectionShape,
	"Bomber.PawnReadySubsystem.ClassReflectionShape",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_ClassReflectionShape::RunTest(const FString& Parameters)
{
	UClass* Class = UBmrPawnReadySubsystem::StaticClass();
	if (!TestNotNull(
			TEXT("UBmrPawnReadySubsystem::StaticClass() must be resolvable — the GENERATED_BODY "
				 "macro plus the .generated.h include should register the class with UE's "
				 "reflection database at module load. A null StaticClass means the UCLASS "
				 "declaration was dropped or the generated header is stale."),
			Class))
	{
		return false;
	}

	TestEqual(
		TEXT("UBmrPawnReadySubsystem must keep its reflection name 'BmrPawnReadySubsystem'. The "
			 "name is referenced by the world subsystem collection's class-based lookup; a "
			 "rename would either break GetSubsystem<UBmrPawnReadySubsystem>() callers or, if "
			 "the rename is consistent, silently break Blueprint references that use the string "
			 "form."),
		Class->GetName(),
		FString(TEXT("BmrPawnReadySubsystem")));

	TestTrue(
		TEXT("UBmrPawnReadySubsystem must derive from UWorldSubsystem — the per-world lifecycle "
			 "is what makes the Reset-on-Deinitialize cleanup matter. A refactor to "
			 "UGameInstanceSubsystem would change the cleanup semantics: the records would "
			 "survive across worlds (the very leak the spec calls out) and the Deinitialize "
			 "would fire only at game-instance teardown."),
		Class->IsChildOf(UWorldSubsystem::StaticClass()));

	TestFalse(
		TEXT("UBmrPawnReadySubsystem must NOT be abstract — the world subsystem collection "
			 "filters out abstract classes and would silently skip instantiation. An abstract "
			 "subsystem leaves Get() / GetPawnReadySubsystem() returning null, which the "
			 "former checkf-asserts on."),
		Class->HasAnyClassFlags(CLASS_Abstract));

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — Runtime null-safety + Reset idempotency, gated on a source-scan
//           confirmation that the IsReady / Reset bodies are non-trivial.
//
// Why a hybrid runtime + source-scan test:
//   The runtime probes below (IsReady(nullptr) false-returns and repeated
//   Reset() crash-safety) are valuable defensive-layer regression checks but
//   are NOT sufficient on their own to distinguish a working impl from a
//   stub. The stripped stub in start/ is literally:
//     bool IsReady(const ABmrPawn*) const        { return false; }
//     bool IsReady(const ABmrPlayerState*) const { return false; }
//     void Reset()                               { }
//   That stub happily satisfies every "must return false for null" and
//   "must not crash on repeated Reset" assertion — IsReady is a constant
//   false-return so nullity literally cannot affect the answer, and Reset
//   has no body so it cannot crash. The runtime probes are real preconditions
//   for the implementation (a re-impl that writes the obvious naïve form,
//   `return IsReady(PlayerState->GetPawn<ABmrPawn>())` without the
//   short-circuit, would crash here), but they cannot by themselves push
//   back on the empty stub.
//
//   The behaviors that most need direct observation — three-signal arbitration
//   (G1), the six-row IsPawnPossessed truth table (G2), the dual broadcast
//   gate (B10), and the already-ready skip (B8) — all require an ABmrPawn
//   instance. But ABmrPawn is declared `UCLASS(Abstract)` and has no concrete
//   C++ subclass in the module (only Blueprint subclasses like
//   BP_PlayerCharacter / BP_BotCharacter, which need a hydrated content
//   package). Beyond construction, exercising IsPawnPossessed's truth table
//   needs a multi-client PIE session where this client's world is non-
//   authoritative while a separate authority world holds the server state —
//   none of which is set up as a reusable harness in this project. And
//   OnReadyHandles is a private member with no UPROPERTY reflection, so
//   there is no way to populate or observe it from a free-standing instance.
//
//   What we CAN do is gate the runtime probes on a strict source-scan
//   precondition: the two IsReady overloads and Reset() must each have
//   non-trivial bodies that match the canonical behavior — specifically,
//   IsReady(const ABmrPawn*) must do an OnReadyHandles lookup, IsReady(const
//   ABmrPlayerState*) must forward through GetPawn (not return a constant),
//   and Reset() must actually mutate OnReadyHandles via Empty()/Reset().
//   Without those, the runtime probes degenerate to "two constants equal
//   false and an empty function did not crash" — which is true of the stub.
//   With them, the runtime probes test what they claim to test: that the
//   REAL IsReady's null guard works, that the REAL forwarding overload's
//   short-circuit works, and that the REAL Reset is idempotent.
//
// What this catches that the rest of the file does not:
//   * The free-standing-instance crash-safety probes (NewObject + repeated
//     Reset() + IsReady(nullptr)) are unique to this test — every other
//     test in the file is a static source scan that never instantiates the
//     subsystem at runtime, so a re-impl that introduces a non-trivial
//     constructor or default state that crashes on Reset without records
//     would slip past every other test.
//   * The IsReady(const ABmrPlayerState*) forwarding overload is not pinned
//     by any other test in this file — test 5 inspects the pawn-pointer
//     overload only. A re-impl that writes the pawn overload correctly
//     but leaves the playerstate overload as `return false;` would still
//     pass test 5; this test catches that gap by requiring the playerstate
//     overload to invoke GetPawn AND IsReady.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_RuntimeNullSafetyAndResetIdempotency,
	"Bomber.PawnReadySubsystem.RuntimeNullSafetyAndResetIdempotency",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_RuntimeNullSafetyAndResetIdempotency::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	// --- Source-scan precondition: the bodies must be non-trivial ---
	//
	// Without this gate, the runtime probes below pass against the stub
	// (where IsReady is `return false` and Reset is empty). The gate
	// rejects the stub by requiring each method body to contain the
	// load-bearing tokens that the canonical implementation must use.
	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// IsReady(const ABmrPawn*) must walk OnReadyHandles via FindByPredicate.
	// The stub is `return false;` — neither token appears. The canonical body
	// has both: `OnReadyHandles.FindByPredicate(...)`. A re-impl that keeps
	// IsReady as a constant false-return would let the runtime null-safety
	// probe pass vacuously (every input returns false), so this gate is what
	// makes that probe meaningful.
	{
		const FString Body = ExtractMemberBody(
			Source, TEXT("UBmrPawnReadySubsystem::IsReady(const ABmrPawn"));
		if (!TestTrue(
				TEXT("G3/B12 (gate): BmrPawnReadySubsystem.cpp must define "
					 "IsReady(const ABmrPawn*) with a body — the start/ stub is a constant "
					 "return that makes the null-safety runtime probe vacuous."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);
		if (!TestTrue(
				TEXT("G3/B12 (gate): IsReady(const ABmrPawn*) must reference OnReadyHandles and "
					 "call FindByPredicate to look up the per-pawn record. The stub `return "
					 "false;` body satisfies the null-safety runtime probe trivially (every "
					 "input returns false); without the OnReadyHandles lookup, the runtime "
					 "probe is not actually testing the IsReady null guard, it is testing a "
					 "constant. Pinning the FindByPredicate call ensures the runtime probe "
					 "exercises real branching logic."),
				CodeOnly.Contains(TEXT("OnReadyHandles"), ESearchCase::CaseSensitive)
					&& CodeOnly.Contains(TEXT("FindByPredicate"), ESearchCase::CaseSensitive)))
		{
			return false;
		}
	}

	// IsReady(const ABmrPlayerState*) must forward through GetPawn AND
	// recursively call IsReady on the pawn. The stub returns false; the
	// canonical body is `return PlayerState && IsReady(PlayerState->
	// GetPawn<ABmrPawn>());`. Without this gate, the playerstate-overload
	// null probe is also vacuous.
	{
		const FString Body = ExtractMemberBody(
			Source, TEXT("UBmrPawnReadySubsystem::IsReady(const ABmrPlayerState"));
		if (!TestTrue(
				TEXT("G3/B12 (gate): BmrPawnReadySubsystem.cpp must define "
					 "IsReady(const ABmrPlayerState*) with a body — the start/ stub is a "
					 "constant return that makes the playerstate-overload null-safety probe "
					 "vacuous."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);
		if (!TestTrue(
				TEXT("G3/B12 (gate): IsReady(const ABmrPlayerState*) must invoke GetPawn and "
					 "recursively call IsReady to forward the query to the pawn overload. The "
					 "stub `return false;` body trivially returns false for any input, so the "
					 "runtime null-safety probe below would pass against the stub. Pinning "
					 "the GetPawn forwarding ensures the runtime probe exercises the actual "
					 "short-circuit guard against null PlayerState."),
				CodeOnly.Contains(TEXT("GetPawn"), ESearchCase::CaseSensitive)
					&& CodeOnly.Contains(TEXT("IsReady"), ESearchCase::CaseSensitive)))
		{
			return false;
		}
	}

	// Reset() must clear OnReadyHandles via Empty() or Reset() on the TArray.
	// The stub is an empty body; this gate requires the actual array-clearing
	// call so the idempotency runtime probe below tests real clearing rather
	// than a no-op (an empty function cannot crash on repeat invocation, so
	// without this gate the idempotency probe is vacuous).
	{
		const FString Body = ExtractMemberBody(Source, TEXT("UBmrPawnReadySubsystem::Reset"));
		if (!TestTrue(
				TEXT("G3/B12 (gate): BmrPawnReadySubsystem.cpp must define Reset with a body — "
					 "the start/ stub is empty, which makes the idempotency runtime probe "
					 "vacuous (an empty function cannot crash on repeat invocation)."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);
		if (!TestTrue(
				TEXT("G3/B12 (gate): Reset() must call OnReadyHandles.Empty() or "
					 "OnReadyHandles.Reset() to actually clear the per-pawn records. The "
					 "stub's empty body satisfies the idempotency runtime probe trivially "
					 "because an empty function cannot crash on repeat invocation; without "
					 "the array-clearing call the runtime probe is not testing what its name "
					 "claims. The named failure mode 'Not resetting records on "
					 "deinitialization' depends on Reset actually clearing the array, not "
					 "just being safe to call."),
				CodeOnly.Contains(TEXT("OnReadyHandles"), ESearchCase::CaseSensitive)
					&& (CodeOnly.Contains(TEXT("Empty("), ESearchCase::CaseSensitive)
						|| CodeOnly.Contains(TEXT("Empty ("), ESearchCase::CaseSensitive)
						|| CodeOnly.Contains(TEXT(".Reset("), ESearchCase::CaseSensitive))))
		{
			return false;
		}
	}

	// --- Runtime probes: now meaningful because the gate has confirmed the
	//                     implementation is non-trivial ---
	//
	// Free-standing instance — no UWorld outer, no Initialize/Deinitialize
	// lifecycle. NewObject succeeds because the class is not abstract (test 9
	// pins this) and has no constructor that depends on world context. The
	// public methods exercised below (IsReady overloads, Reset) all live
	// above the world-context layer — they read/mutate the OnReadyHandles
	// TArray directly without touching GetWorld(), so a free-standing
	// instance is a valid probe surface for the defensive layer.
	UBmrPawnReadySubsystem* Subsystem = NewObject<UBmrPawnReadySubsystem>(GetTransientPackage());
	if (!TestNotNull(
			TEXT("G3 (runtime): UBmrPawnReadySubsystem must be constructible via "
				 "NewObject<>(GetTransientPackage()). The world subsystem framework instantiates "
				 "UCLASS-declared subclasses automatically; the same allocator path must accept "
				 "standalone test instantiation so a free-standing instance can exercise "
				 "null-safety paths without a hydrated PIE world. A null here means either the "
				 "UCLASS declaration was dropped (no reflection) or the class was incorrectly "
				 "marked Abstract — both regressions that disable the subsystem at runtime."),
			Subsystem))
	{
		return false;
	}

	// --- Direct runtime probe 1: IsReady(const ABmrPawn*) null-guard ---
	//
	// The canonical body opens with `if (!Pawn) return false;`. Omitting this
	// guard lets the FindByPredicate lambda compare `It.Pawn == nullptr`
	// against every record in OnReadyHandles — and if the record's weak-ptr
	// happens to be in a default-constructed (null) state, the predicate
	// matches and the function walks downstream into IsPawnPossessed with a
	// stale handle. IsPawnPossessed dereferences FoundHandle.Pawn to read
	// HasAuthority(), IsLocallyControlled() — both crash on null.
	TestFalse(
		TEXT("G3/B12 (runtime): IsReady((const ABmrPawn*)nullptr) must return false. The "
			 "canonical opening guard `if (!Pawn) return false` keeps the FindByPredicate "
			 "lambda from being called with a sentinel value and keeps IsPawnPossessed from "
			 "dereferencing a null pawn (HasAuthority / IsLocallyControlled would crash). "
			 "Downstream listeners depend on IsReady never reporting ready for a non-existent "
			 "pawn, and a re-impl that omits the guard would either crash on null deref or "
			 "(worse) silently match a stale weak-ptr-default record and report it ready."),
		Subsystem->IsReady(static_cast<const ABmrPawn*>(nullptr)));

	// --- Direct runtime probe 2: IsReady(const ABmrPlayerState*) null-guard ---
	//
	// The canonical body is `return PlayerState && IsReady(PlayerState->
	// GetPawn<ABmrPawn>());` — the short-circuit AND keeps the GetPawn call
	// from dereferencing a null player state. A re-impl that writes
	// `return IsReady(PlayerState->GetPawn<ABmrPawn>())` without the
	// short-circuit crashes on null.
	TestFalse(
		TEXT("G3/B12 (runtime): IsReady((const ABmrPlayerState*)nullptr) must return false. The "
			 "overload forwards to IsReady(PlayerState->GetPawn<ABmrPawn>()); a missing null "
			 "guard would dereference the null player state and crash before reaching the "
			 "pawn-overload's own guard. The cheat manager and global event subsystem both "
			 "invoke this surface from contexts where the player state may not yet have "
			 "replicated, so the null short-circuit is the difference between a safe early "
			 "false-return and a null-deref crash on every fresh client connection."),
		Subsystem->IsReady(static_cast<const ABmrPlayerState*>(nullptr)));

	// --- Direct runtime probe 3: Reset() crash-safety on empty state ---
	//
	// The canonical body is `OnReadyHandles.Empty();` — a no-op on an empty
	// array. A re-impl that walks the array unbinding delegates without a
	// length guard, or that does any post-empty bookkeeping that touches the
	// now-empty array, would crash here. The empty state IS the steady state
	// during Deinitialize after a fresh-boot world tear-down where no signal
	// ever arrived, so this case fires on every PIE session that ends before
	// any pawn becomes ready.
	Subsystem->Reset();

	TestFalse(
		TEXT("G3/B12 (runtime): After Reset() on an empty subsystem, IsReady(nullptr) must "
			 "still return false. The Reset path must not put OnReadyHandles into a state that "
			 "breaks subsequent null-pawn queries — the named failure mode 'Not resetting "
			 "records on deinitialization' depends on Reset being a safe, idempotent cleanup "
			 "on ANY state the subsystem can be in, including the empty fresh-boot state where "
			 "Deinitialize fires before any signal has arrived. A re-impl that leaves "
			 "OnReadyHandles in a half-cleared state would let the next IsReady call walk a "
			 "freed buffer."),
		Subsystem->IsReady(static_cast<const ABmrPawn*>(nullptr)));

	// --- Direct runtime probe 4: Reset() is idempotent ---
	//
	// Reset is both the Deinitialize cleanup hook AND a public callable.
	// Map transitions can trigger overlapping tear-down paths (world
	// destruction + game-instance shutdown, or a level-streaming purge
	// followed by a regular tear-down), and Reset must remain safe under
	// repeat invocation. A re-impl that, say, calls Empty() then sets a
	// "dirty" flag and asserts on the second pass — or that frees memory
	// without nulling pointers — would fail here.
	Subsystem->Reset();
	Subsystem->Reset();

	TestFalse(
		TEXT("G3/B12 (runtime): Multiple Reset() calls in succession must remain safe and "
			 "leave the subsystem in a consistent state where IsReady(nullptr) returns false. "
			 "Reset is both the Deinitialize hook and a public callable; map transitions can "
			 "fire from overlapping tear-down paths and the public API lets callers explicitly "
			 "re-arm. A re-impl that isn't idempotent (e.g. asserts on a second-empty call, or "
			 "corrupts the TArray's allocator state by double-freeing) would crash here or "
			 "leave IsReady walking freed memory."),
		Subsystem->IsReady(static_cast<const ABmrPawn*>(nullptr)));

	TestFalse(
		TEXT("G3/B12 (runtime): IsReady((const ABmrPlayerState*)nullptr) must continue to "
			 "return false after multiple Reset() calls. The PlayerState overload chains into "
			 "the pawn overload; both guards must remain stable across cleanup cycles so "
			 "downstream listeners that poll IsReady on a player state between map transitions "
			 "(global event subsystem rebinds, viewmodel reconnects) get a consistent false "
			 "rather than a crash or a stale-true."),
		Subsystem->IsReady(static_cast<const ABmrPlayerState*>(nullptr)));

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — DIRECT runtime probe of IsReady's "no matching record" branch.
//
// Test 10 covers the null-guard path of IsReady (`if (!Pawn) return false`).
// This test covers a different and equally load-bearing branch: when the
// pawn pointer is non-null but no FOnReadyData record matches, the
// FindByPredicate scan returns nullptr and IsReady must fall through to
// return false. This is the *common* runtime path — every IsReady call
// against a never-registered pawn lands here — and it's what makes the
// "Skip if already ready" guard at the top of each Broadcast_* entry point
// correctly differentiate "first signal for this pawn" (no record → not
// ready → proceed) from "late signal after broadcast" (record exists,
// all three flags set → ready → skip).
//
// Anti-patterns this catches that Test 10 does not:
//   * A re-impl that, on "no record found," constructs a temporary stack
//     FOnReadyData and reads its default fields would see bIsPossessed=false
//     && bIsAddedOnGeneratedMap=false → IsPawnPossessed→false → IsReady→false
//     by accident. But IsReady's contract calls for *short-circuit* on
//     no-record, not "evaluate default record." A re-impl that defaults
//     bIsAddedOnGeneratedMap=true (a plausible typo if someone copies the
//     "ready" sentinel from the broadcast path) would return true for any
//     non-null pointer.
//   * A re-impl that mistakenly compares `It.Pawn.Get() != nullptr` instead
//     of `It.Pawn == Pawn` would match any non-null record against any
//     non-null query — also caught here because the empty array means
//     FindByPredicate returns null regardless.
//   * A re-impl that initializes OnReadyHandles with a sentinel "default
//     ready" entry on construction would have a record present at startup,
//     and IsReady against any pointer might false-match.
//
// Why this counts as direct runtime coverage:
//   It calls the real IsReady at runtime, passes a typed pointer that the
//   real lookup must compare against real records, and asserts on the real
//   return value. The pointer is a sentinel (the subsystem's own address
//   reinterpret-cast to ABmrPawn*) — safe because IsReady's FindByPredicate
//   lambda only does weak-ptr-vs-raw-pointer equality comparison, never
//   dereferences the parameter.
//
// Source-scan gate (preserved from Test 10's pattern):
//   The probe is preceded by a source-scan precondition that the IsReady
//   body actually walks OnReadyHandles via FindByPredicate. Without this
//   gate, the stub `return false;` body would trivially pass the runtime
//   probe (any input → false), so the gate is what makes the probe
//   meaningful against the stripped start/ workspace.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPawnReadySubsystem_IsReadyRejectsUnknownPawnAtRuntime,
	"Bomber.PawnReadySubsystem.IsReadyRejectsUnknownPawnAtRuntime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPawnReadySubsystem_IsReadyRejectsUnknownPawnAtRuntime::RunTest(const FString& Parameters)
{
	using namespace PawnReadySubsystemTests;

	// --- Source-scan gate: IsReady must walk OnReadyHandles via FindByPredicate ---
	//
	// Without this gate, the stripped stub `return false;` would pass the
	// runtime probe trivially (every input returns false). The gate rejects
	// the stub by requiring the canonical FindByPredicate-based lookup.
	const FString Source = LoadProjectFile(this, SubsystemCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}
	{
		const FString Body = ExtractMemberBody(
			Source, TEXT("UBmrPawnReadySubsystem::IsReady(const ABmrPawn"));
		if (!TestTrue(
				TEXT("Gate: IsReady(const ABmrPawn*) must have a body — the start/ stub is a "
					 "constant return that would make the FindByPredicate-not-found runtime "
					 "probe vacuous (every input returns false trivially)."),
				!Body.IsEmpty()))
		{
			return false;
		}
		const FString CodeOnly = StripComments(Body);
		if (!TestTrue(
				TEXT("Gate: IsReady(const ABmrPawn*) must walk OnReadyHandles via "
					 "FindByPredicate to look up the per-pawn record by pointer identity. "
					 "Without the FindByPredicate scan, the runtime probe below is not "
					 "exercising the canonical lookup branch — it would just be re-testing a "
					 "constant return path. Pinning FindByPredicate ensures the probe lands "
					 "on the actual no-record-found fall-through."),
				CodeOnly.Contains(TEXT("OnReadyHandles"), ESearchCase::CaseSensitive)
					&& CodeOnly.Contains(TEXT("FindByPredicate"), ESearchCase::CaseSensitive)))
		{
			return false;
		}
	}

	// --- Free-standing instance — fresh OnReadyHandles is empty ---
	UBmrPawnReadySubsystem* Subsystem = NewObject<UBmrPawnReadySubsystem>(GetTransientPackage());
	if (!TestNotNull(
			TEXT("G3 (runtime): UBmrPawnReadySubsystem must be NewObject-constructible — "
				 "required for the no-record-found IsReady probe."),
			Subsystem))
	{
		return false;
	}

	// --- Direct runtime probe: IsReady against a non-null unknown pawn ---
	//
	// The subsystem's own address (typed as ABmrPawn*) is a sentinel that
	// IsReady will never find a record for. The FindByPredicate scan
	// iterates zero records, returns null, and IsReady must fall through
	// to return false. Safe because IsReady only does pointer-identity
	// comparison via TWeakObjectPtr's operator==(const T*), which calls
	// Get() == Other — Get() returns the weak-ptr's resolved pointer (or
	// null), never dereferences `Other`. So passing a non-pawn pointer
	// reinterpret-cast to ABmrPawn* is safe in this branch.
	const ABmrPawn* UnknownPawn = reinterpret_cast<const ABmrPawn*>(Subsystem);
	TestFalse(
		TEXT("G3/B12 (runtime): IsReady against an unknown non-null pawn must return false on "
			 "a fresh subsystem. The canonical FindByPredicate scan iterates zero records and "
			 "returns null, and IsReady's no-record-found branch must yield false. This is the "
			 "*common* runtime path — every IsReady call against a never-registered pawn lands "
			 "here, and it's what makes the 'Skip if already ready' guard at the top of each "
			 "Broadcast_* entry point correctly differentiate 'first signal for this pawn' (no "
			 "record → not ready → proceed) from 'late signal after broadcast' (record exists, "
			 "all three flags set → ready → skip). A re-impl that on no-record-found reads a "
			 "default-constructed temporary FOnReadyData with bIsAddedOnGeneratedMap=true (a "
			 "plausible typo from copying the ready sentinel) would return true here."),
		Subsystem->IsReady(UnknownPawn));

	// --- Post-Reset: same answer — Reset must not corrupt the lookup ---
	//
	// Reset's contract is 'empty the records'. The FindByPredicate scan
	// post-Reset must still iterate zero records and return null. A re-impl
	// that swaps the TArray's allocator for a moved-from buffer (instead of
	// calling Empty()) would either leave the array in an invalid state or
	// would set Num()=0 with stale backing memory the scan might still walk.
	Subsystem->Reset();
	TestFalse(
		TEXT("G3/B12 (runtime): IsReady against the same unknown non-null pawn must continue "
			 "to return false after Reset(). Reset must leave OnReadyHandles in a queryable "
			 "empty state — a re-impl that Move-assigns the array (leaving the array in a "
			 "valid-but-empty state) is fine; a re-impl that std::memset-zeroes the array or "
			 "leaks backing memory would corrupt the FindByPredicate scan. Combined with "
			 "test 10's nullptr probes, this pins both code paths of IsReady (null-guard and "
			 "no-record-found) across the Reset boundary."),
		Subsystem->IsReady(UnknownPawn));

	return true;
}
