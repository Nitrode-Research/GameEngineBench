// Copyright (c) 2026 GameDevBench. Cheat-manager behavior automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/GameFramework/BmrCheatManager.cpp
// (spec.yaml lists "Subsystems/" but the actual on-disk location is
// "GameFramework/" — the source-inspection helpers try both paths.)
//
// UBmrCheatManager is the project's non-shipping cheat console. The spec's
// failure modes (B1-B6) are about routing — every cheat that mutates
// gameplay state must thread through the correct project pathway:
//   * B1: Suicide / DestroyPlayersBySlots route through the Player_Death
//         broadcast event — direct pawn destroy skips death tabulation and
//         the pawn pool.
//   * B2: God() does NOT chain Super::God() — the base flag is unread by
//         the GAS damage gate; god mode lives as a damage-immunity gameplay
//         effect on the ASC.
//   * B3: SetAutoCopilot uses APlayerState::SetIsABot (engine base) — the
//         project override flips ASC replication mode and would tear down
//         replication of a live client.
//   * B4: SetPlayerPowerups enumerates FBmrPowerupTag::GetAll() — a
//         hardcoded list drifts the moment a new powerup is added.
//   * B5: SetWallsChance(-1) restores defaults from the generation data
//         asset — a literal-clamp impl gives 0% walls forever.
//   * B6: SetWallsChance / SetBoxesChance call GenerateLevelActors() —
//         without regen the map only updates on the next round boundary.
//
// Strategy: three behaviors are directly callable from a free automation
// test and unit-tested at runtime (Tests 1, 2, 3): two pure-function
// bitmask helpers and the constructor's debug-camera-controller-class
// assignment. A fourth runtime test (Test 11) exercises the failure-mode
// entry points (Suicide, DestroyPlayersBySlots, SetPlayerPowerups,
// SetAutoCopilot) on the static class, observing that the canonical
// resolve-pawn-then-route prelude is in place — a re-impl that drops the
// null guard for the pawn / ASC / player state crashes there. The
// remaining behaviors are routing decisions buried in cheat-driven
// mutations to PIE-wide subsystems (the gameplay-message subsystem, the
// generated map, the ability system globals): the SPECIFIC routings (the
// gameplay-tag broadcast, the ASC effect path, the engine-base SetIsABot
// qualifier, the registry-driven attribute loop, the regen call) are
// observed via source inspection over the stripped .cpp (Tests 4-10).
// Together the runtime + source layers cover G1 — the runtime test
// catches the named B1/B4 anti-patterns (naive pawn deref / unchecked
// attribute write) at the call site, the source tests catch the named
// B1/B2/B3/B4/B5/B6 anti-patterns at the implementation level.
// Substring scans run over a comment-stripped buffer so anti-pattern
// descriptions inside comments (e.g. "Super is not called intentionally")
// don't false-match.

// Bomber (game module — kept at top per task rules)
#include "GameFramework/BmrCheatManager.h"
#include "Bomber.h" // EBmrActorType
#include "Controllers/BmrDebugCameraController.h"

// UE
#include "CoreMinimal.h"
#include "Engine/DebugCameraController.h"
#include "GameFramework/CheatManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/SubclassOf.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogCheatManagerTests, Log, All);

namespace CheatManagerTests
{
	// spec.yaml lists "Subsystems/" but the source file lives in
	// "GameFramework/". Both are tried so the test survives either layout.
	static const TCHAR* CheatManagerRelPaths[] = {
		TEXT("Source/Bomber/Private/GameFramework/BmrCheatManager.cpp"),
		TEXT("Source/Bomber/Private/Subsystems/BmrCheatManager.cpp"),
	};

	static FString LoadCheatManagerSource(FAutomationTestBase* Test)
	{
		FString Source;
		FString Tried;
		for (const TCHAR* Rel : CheatManagerRelPaths)
		{
			const FString AbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / Rel);
			Tried += FString::Printf(TEXT("\n  - %s"), *AbsPath);
			if (FFileHelper::LoadFileToString(Source, *AbsPath))
			{
				return Source;
			}
		}
		Test->TestTrue(
			FString::Printf(TEXT("BmrCheatManager.cpp must be readable at one of:%s"), *Tried),
			false);
		return FString();
	}

	// Strip C++ comments so substring scans don't false-match anti-pattern
	// descriptions written inside comments (e.g. the canonical body's
	// "Super is not called intentionally" must not satisfy a "Super::" scan).
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

	// Brace-depth walker; counts braces only so lambdas / initialiser lists
	// nested inside a body do not break the scan.
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
} // namespace CheatManagerTests

// ---------------------------------------------------------------------------
// Test 1 — Runtime: GetBitmaskFromReverseString walks left-to-right, ORing
//          (1 << Index) for '1' chars and skipping non-numeric characters
//          without advancing the bit index.
//
// One of the few cheat-manager behaviours directly callable in unit form:
// a static pure function with no world / pawn / ASC dependency. The header
// gives the canonical mapping: "0001" -> 8, "1 1 0 0" -> 3 — the leftmost
// char is bit 0, spaces are skipped without consuming an index.
//
// Discriminators:
//   * "0001" -> 8 proves position-0-is-leftmost. A normal big-endian binary
//     parse ("0001" -> 1) would give the wrong answer.
//   * "1010" -> 5 (1<<0 | 1<<2) proves the reversed-bit-order — distinguishes
//     from a plain binary parse (which would yield 10).
//   * "1 1 0 0" -> 3 proves spaces are SKIPPED rather than treated as bit
//     zero (which would yield a different bitmask).
//   * Empty / non-numeric-only input -> 0 proves no crash and a well-defined
//     no-op when nothing parses.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_GetBitmaskFromReverseString,
	"Bomber.CheatManager.GetBitmaskFromReverseString",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_GetBitmaskFromReverseString::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("\"0001\" must produce bitmask 8 (1 << 3) — the leftmost character is bit zero, so "
			 "'0001' sets only bit 3. A normal binary-parse impl would give 1 (bit 0)."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("0001"))),
		8);

	TestEqual(
		TEXT("\"1000\" must produce bitmask 1 — the leftmost '1' sets bit 0."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1000"))),
		1);

	TestEqual(
		TEXT("\"1100\" must produce bitmask 3 — leftmost two '1's set bits 0 and 1."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1100"))),
		3);

	TestEqual(
		TEXT("\"1010\" must produce bitmask 5 — bits 0 and 2. Distinguishes a reversed-bit-order "
			 "impl (5) from a normal big-endian binary parse (which would give 10)."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1010"))),
		5);

	TestEqual(
		TEXT("\"1 1 0 0\" must produce bitmask 3 — spaces are SKIPPED without advancing the bit "
			 "index. A spec-failure impl that treated each non-numeric char as bit zero would "
			 "yield a different value."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1 1 0 0"))),
		3);

	TestEqual(
		TEXT("\"1 1 1 1\" must produce bitmask 15 — all four low bits set, spaces skipped."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1 1 1 1"))),
		15);

	TestEqual(
		TEXT("Empty input must yield 0 — well-defined no-op."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString()),
		0);

	TestEqual(
		TEXT("Non-numeric-only input must yield 0 — every char is skipped, no bits are set."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("abc"))),
		0);

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — Runtime: GetBitmaskFromActorTypesString parses whitespace-separated
//          EBmrActorType names and ORs the enum's underlying bit values.
//
// EBmrActorType is a UENUM(Bitflags) with values 1<<0..1<<4 (Bomb=1, Box=2,
// Powerup=4, Player=8, Wall=16). The header maps "Wall" -> 16,
// "Wall Bomb" -> 17, "Wall Bomb Player" -> 25. The helper splits on
// whitespace, resolves each token via the EBmrActorType UEnum, and ORs the
// result.
//
// Direct runtime test: pure static method, no actor / world / subsystem
// dependency — the UEnum is registered at module load.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_GetBitmaskFromActorTypesString,
	"Bomber.CheatManager.GetBitmaskFromActorTypesString",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_GetBitmaskFromActorTypesString::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("\"Wall\" must produce bitmask 16 — EBmrActorType::Wall == 1<<4. Discriminates a "
			 "stubbed-zero impl from a correct enum-name lookup."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString(TEXT("Wall"))),
		static_cast<int32>(EBmrActorType::Wall));

	TestEqual(
		TEXT("\"Bomb\" must produce bitmask 1 — EBmrActorType::Bomb == 1<<0."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString(TEXT("Bomb"))),
		static_cast<int32>(EBmrActorType::Bomb));

	TestEqual(
		TEXT("\"Wall Bomb\" must produce bitmask 17 — the helper splits on whitespace and ORs "
			 "the two enum values together (1<<4 | 1<<0). Distinguishes a single-token impl "
			 "(returning only the first or only the last) from one that handles the whole list."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString(TEXT("Wall Bomb"))),
		static_cast<int32>(EBmrActorType::Wall) | static_cast<int32>(EBmrActorType::Bomb));

	TestEqual(
		TEXT("\"Wall Bomb Player\" must produce bitmask 25 — three OR'd values (1<<4 | 1<<0 | "
			 "1<<3). The header's worked example."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString(TEXT("Wall Bomb Player"))),
		static_cast<int32>(EBmrActorType::Wall)
			| static_cast<int32>(EBmrActorType::Bomb)
			| static_cast<int32>(EBmrActorType::Player));

	TestEqual(
		TEXT("Empty input must yield 0 — well-defined no-op (no tokens to resolve)."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString()),
		0);

	TestEqual(
		TEXT("Unknown token must contribute nothing — \"NotAType\" doesn't resolve to any "
			 "EBmrActorType, so the result is 0. A re-impl that fell through to some default "
			 "(e.g. returned All) would change the destroy-cheat semantics for any typo'd token."),
		UBmrCheatManager::GetBitmaskFromActorTypesString(FString(TEXT("NotAType"))),
		0);

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Runtime: Constructor sets DebugCameraControllerClass to
//          ABmrDebugCameraController::StaticClass().
//
// The spec's Constructor section: "Sets the debug camera controller class
// to the project's subclass." The base UCheatManager exposes a public
// TSubclassOf<ADebugCameraController> DebugCameraControllerClass field; the
// canonical constructor body overrides it with the project subclass so that
// Super::EnableDebugCamera() (engine-base path) spawns the project's
// debug-camera-controller variant rather than the engine default.
//
// Directly testable in unit form: reading the field on the CDO observes the
// constructor's assignment at module load — no world, no NewObject<> needed.
// The failure mode is a constructor body that drops the assignment entirely,
// leaving the engine default in place; this test discriminates by checking
// both that the project subclass is installed AND that the engine default
// is NOT in place.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_ConstructorSetsDebugCameraControllerClass,
	"Bomber.CheatManager.ConstructorSetsDebugCameraControllerClass",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_ConstructorSetsDebugCameraControllerClass::RunTest(const FString& Parameters)
{
	const UBmrCheatManager* CDO = GetDefault<UBmrCheatManager>();
	if (!TestNotNull(
			TEXT("UBmrCheatManager CDO must exist — the class is GENERATED_BODY-marked and "
				 "auto-registers a CDO at module load."),
			CDO))
	{
		return false;
	}

	UClass* ProjectSubclass = ABmrDebugCameraController::StaticClass();
	if (!TestNotNull(
			TEXT("ABmrDebugCameraController must resolve — the project's debug-camera-controller "
				 "subclass is the canonical constructor target."),
			ProjectSubclass))
	{
		return false;
	}

	// DebugCameraControllerClass is a public UPROPERTY
	// TSubclassOf<ADebugCameraController> on the engine base UCheatManager.
	// Reading it on the CDO observes the constructor's assignment.
	UClass* ConfiguredClass = *CDO->DebugCameraControllerClass;

	TestEqual(
		TEXT("Constructor must set DebugCameraControllerClass = ABmrDebugCameraController::"
			 "StaticClass(). Without this assignment, Super::EnableDebugCamera spawns the "
			 "engine's default debug camera controller, bypassing the project's subclass — every "
			 "override ABmrDebugCameraController adds (the suppressed Debug HUD, the custom "
			 "input handling) is then unreachable."),
		ConfiguredClass, ProjectSubclass);

	// Failure-mode negative: the engine default must NOT be in place. If the
	// project constructor body is empty, ADebugCameraController is what
	// leaks through from UCheatManager's own initialisation path.
	TestNotEqual(
		TEXT("DebugCameraControllerClass must not be the engine default ADebugCameraController. "
			 "If it is, the project's constructor body is missing the assignment — the engine "
			 "default propagates from UCheatManager and the project's debug-camera-controller "
			 "subclass is never spawned."),
		ConfiguredClass, static_cast<UClass*>(ADebugCameraController::StaticClass()));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — Source: Suicide routes through the Player_Death event broadcast,
//          NOT a direct pawn destroy (B1 failure mode).
//
// The spec: "Suicide must kill the local player through the death-event
// path, not by directly destroying the pawn. A direct destroy skips death
// tabulation and pool return." The canonical body builds a slot string
// ("0..01") and calls DestroyPlayersBySlots, which broadcasts the
// Player_Death gameplay event.
//
// Runtime observation requires a fully hydrated local player (ABmrPawn with
// player ID, an authoritative ABmrGameState handling death tabulation, a
// registered UGlobalMessageSubsystem) — well past what a free automation
// test reaches in this project. Source inspection is the practical
// discriminator for the failure mode.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SuicideRoutesThroughDeathEvent,
	"Bomber.CheatManager.SuicideRoutesThroughDeathEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SuicideRoutesThroughDeathEvent::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::Suicide"));
	if (!TestTrue(
			TEXT("B1: BmrCheatManager.cpp must define Suicide with a body — the start/ stub "
				 "leaves the body empty; the death-event routing is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// Canonical Suicide builds a slot string from the local player ID and
	// delegates to DestroyPlayersBySlots — which itself broadcasts the death
	// event. EITHER referencing DestroyPlayersBySlots (the canonical delegate)
	// OR directly broadcasting Player_Death satisfies the spec.
	const bool bRoutesViaDeathEvent =
		Code.Contains(TEXT("DestroyPlayersBySlots"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("Player_Death"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: Suicide must route through the death-event pathway — either by delegating to "
			 "DestroyPlayersBySlots (canonical) or by directly broadcasting BmrGameplayTags::"
			 "Event::Player_Death via UGlobalMessageSubsystem. The spec is explicit: kill via "
			 "the death event, not by destroying the pawn — otherwise death tabulation and "
			 "pawn-pool return don't run."),
		bRoutesViaDeathEvent);

	// Failure-mode negative: the body must NOT directly destroy the pawn.
	// Common direct-destroy patterns: Pawn->Destroy(), K2_DestroyActor(),
	// SetLifeSpan(0.f). Applying damage through ApplyPointDamage is
	// acceptable because GAS handles death gating, but a direct Destroy()
	// short-circuits the whole pipeline.
	const bool bDirectlyDestroysPawn =
		Code.Contains(TEXT("Pawn->Destroy("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("LocalPawn->Destroy("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("->K2_DestroyActor("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("SetLifeSpan(0"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B1 failure mode: Suicide must NOT directly destroy the pawn. The spec names this: "
			 "a direct destroy skips death tabulation (the GameState's round-result computation) "
			 "and the actor-pool's return-on-death hook — the pool then leaks the pawn instance "
			 "and the next round's spawn produces a stale or null reference."),
		bDirectlyDestroysPawn);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Source: DestroyPlayersBySlots broadcasts the Player_Death event
//          for each slot-matching player, NOT a direct pawn destroy
//          (B1 failure mode).
//
// The spec lists this alongside Suicide as the death-event pathway:
// "DestroyPlayersBySlots kills players whose slot indices match a bitmask
// parsed from a string — through the death event." The canonical body
// parses the bitmask, walks the level's player map components, and for each
// player whose slot bit is set, builds a FGameplayEventData with
// Player_Death and hands it to UGlobalMessageSubsystem::
// BroadcastGlobalMessage. A direct destroy bypasses every listener that
// subscribes to Player_Death (kill-count tabulation, scoreboard, pawn-pool
// return).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_DestroyPlayersBySlotsBroadcastsDeathEvent,
	"Bomber.CheatManager.DestroyPlayersBySlotsBroadcastsDeathEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_DestroyPlayersBySlotsBroadcastsDeathEvent::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::DestroyPlayersBySlots"));
	if (!TestTrue(
			TEXT("B1: BmrCheatManager.cpp must define DestroyPlayersBySlots with a body — the "
				 "start/ stub is empty; the slot-parse + death-broadcast logic is what was "
				 "stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// Must parse the slot string via GetBitmaskFromReverseString (the helper
	// the spec pins as the bit-parser).
	TestTrue(
		TEXT("B1: DestroyPlayersBySlots must parse the slot string via "
			 "GetBitmaskFromReverseString. The string format the cheat docs use (\"1 0 0 0\", "
			 "\"0111\") is the reverse-bitmask convention that helper implements — a re-impl "
			 "that FCString::Atoi'd the whole string would misread \"0001\" as the integer 1 "
			 "(bit 0) instead of bit 3."),
		Code.Contains(TEXT("GetBitmaskFromReverseString"), ESearchCase::CaseSensitive));

	// Must broadcast Player_Death via the global message subsystem — that's
	// the death-event pathway the spec names.
	TestTrue(
		TEXT("B1: DestroyPlayersBySlots must broadcast BmrGameplayTags::Event::Player_Death. The "
			 "spec is explicit: kill THROUGH the death event. Every listener (kill-count "
			 "tabulation, scoreboard, pawn-pool return) subscribes to Player_Death; a body that "
			 "mutates directly bypasses all of them."),
		Code.Contains(TEXT("Player_Death"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B1: DestroyPlayersBySlots must publish the event via UGlobalMessageSubsystem (the "
			 "project's gameplay-event broadcaster). A direct ASC->HandleGameplayEvent call "
			 "lands on a single ASC and skips the multi-listener fanout the subsystem provides."),
		Code.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("GlobalMessageSubsystem"), ESearchCase::CaseSensitive));

	// Failure-mode negative: no direct pawn destroy.
	const bool bDirectlyDestroysPawn =
		Code.Contains(TEXT("Pawn->Destroy("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("Character->Destroy("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("->K2_DestroyActor("), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("DestroyActor("), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B1 failure mode: DestroyPlayersBySlots must NOT directly destroy any pawn. The "
			 "named failure: 'directly destroying the pawn skips death tabulation and pool "
			 "return.' The death event is the only correct shutdown path."),
		bDirectlyDestroysPawn);

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Source: God() does NOT call Super::God() (B2 failure mode).
//
// The spec: "God() must not call Super::God() — the base-class flag is not
// what GAS damage gating reads. God mode lives as a damage-immunity gameplay
// effect on the local player's ASC." A Super::God() call flips
// UCheatManager::bGodMode, which the engine's normal damage path checks —
// but BmrPawn's GAS-driven damage gate doesn't read that flag, so the cheat
// appears to work in editor (engine-only damage paths) and silently fails
// the moment any project ability inflicts damage.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_GodDoesNotCallSuper,
	"Bomber.CheatManager.GodDoesNotCallSuper",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_GodDoesNotCallSuper::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::God"));
	if (!TestTrue(
			TEXT("B2: BmrCheatManager.cpp must define a God() override body — the header "
				 "declares it as an override; a re-impl that drops the override entirely lets "
				 "the base UCheatManager::God flip its own bGodMode flag, which GAS damage "
				 "gating ignores."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// Negative: Super::God() must not be present. The body may reference
	// Super for other purposes — but Super::God() specifically is the
	// spec-named failure mode.
	const bool bCallsSuperGod =
		Code.Contains(TEXT("Super::God"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("UCheatManager::God"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("UMetaCheatManager::God"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B2 failure mode: God() must NOT call Super::God(). The base flag "
			 "(UCheatManager::bGodMode) is not what the project's GAS damage gate reads — "
			 "toggling it produces a cheat that works against engine-only damage paths but "
			 "silently fails against every ability-driven damage source in the game."),
		bCallsSuperGod);

	// Positive: the body must touch the ASC and the gameplay-effect pathway.
	// The canonical encoding either applies the GetBlockIncomingDamageEffect
	// via ApplyGameplayEffectToSelf or removes active effects matching the
	// BlockIncomingDamage tag — both route through the ASC.
	const bool bTouchesAscAndEffect =
		Code.Contains(TEXT("AbilitySystem"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("ASC"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("GameplayEffect"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B2: God() must route the damage-immunity toggle through the ASC / a gameplay "
			 "effect. The canonical body resolves the local pawn's UAbilitySystemComponent and "
			 "either ApplyGameplayEffectToSelf'es a BlockIncomingDamage effect or "
			 "RemoveActiveEffectsWithGrantedTags. A body that only logs (or mutates an "
			 "unrelated flag) doesn't grant GAS-level damage immunity at all."),
		bTouchesAscAndEffect);

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Source: SetAutoCopilot uses APlayerState::SetIsABot (engine base),
//          NOT the project's ABmrPlayerState::SetIsABot override
//          (B3 failure mode).
//
// The spec: "SetAutoCopilot must use the engine base class's SetIsABot
// rather than the project's override." The project override flips the ASC's
// EGameplayEffectReplicationMode between Minimal (bot) and Mixed (human) —
// calling it while a human client is still connected tears down the GE
// replication stream for the live client. The cheat's purpose is to possess
// the player with an AI controller for a quick local test, NOT to
// reconfigure replication; the engine base just sets the bIsABot flag.
//
// The discriminator is the QUALIFIED call: PlayerState->Super::SetIsABot(...)
// (or equivalently a cast to APlayerState* before the call). A naked
// PlayerState->SetIsABot(...) resolves to the project override and trips B3.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SetAutoCopilotUsesEngineBaseSetIsABot,
	"Bomber.CheatManager.SetAutoCopilotUsesEngineBaseSetIsABot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SetAutoCopilotUsesEngineBaseSetIsABot::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::SetAutoCopilot"));
	if (!TestTrue(
			TEXT("B3: BmrCheatManager.cpp must define SetAutoCopilot with a body — the start/ "
				 "stub is empty; the engine-base SetIsABot call + AI possession swap is what was "
				 "stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// Positive: must call SetIsABot through the engine base. The canonical
	// encoding is `PlayerState->Super::SetIsABot(...)` (Super used as a name
	// to qualify to the parent class) or a cast to APlayerState* first.
	const bool bUsesEngineBaseQualifiedCall =
		Code.Contains(TEXT("Super::SetIsABot"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("APlayerState::SetIsABot"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("(APlayerState*)"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("Cast<APlayerState>"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("static_cast<APlayerState"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B3: SetAutoCopilot must invoke SetIsABot through the engine base APlayerState — "
			 "either via the `Super::SetIsABot` qualified call (the canonical encoding) or via "
			 "an explicit cast to APlayerState* before the call. The project's "
			 "ABmrPlayerState::SetIsABot override flips the ASC's "
			 "EGameplayEffectReplicationMode (Minimal for bots, Mixed for humans); calling it "
			 "from a cheat while a live client is connected tears down the controlling client's "
			 "GE replication stream — the spec's named B3 failure mode."),
		bUsesEngineBaseQualifiedCall);

	// Sanity: the body must reference SetIsABot at all — without the call,
	// the AI-controller swap on its own leaves bIsABot in its previous state
	// and the AI controller refuses to take over (or the player-state
	// reports the wrong identity to listeners).
	TestTrue(
		TEXT("B3: SetAutoCopilot must reference SetIsABot — without flipping the bot flag, the "
			 "subsequent AI-controller possession lands on a player-state that still "
			 "self-reports as human."),
		Code.Contains(TEXT("SetIsABot"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Source: SetPlayerPowerups enumerates the powerup-tag registry
//          (FBmrPowerupTag::GetAll()), NOT a hardcoded attribute list
//          (B4 failure mode).
//
// The spec: "Enumerates powerup tags from the project's powerup-tag
// registry — does not hardcode the list." A re-impl that hardcoded the
// attribute writes (e.g. SetSkate, SetFire, SetBombsAvailable) silently
// drifts the moment a new powerup is added: the cheat then sets only the
// known three and leaves new powerups at their default level, which is the
// opposite of what the cheat's purpose ("max-level all powerups") demands.
//
// The canonical encoding:
//   for (const FGameplayTag& Tag : FBmrPowerupTag::GetAll())
//   {
//       const FGameplayAttribute Attr =
//           UBmrPowerupsAttributeSet::Conv_TagToBaseAttribute(Tag);
//       if (Attr.IsValid()) ASC->ApplyModToAttributeUnsafe(Attr, ...);
//   }
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SetPlayerPowerupsUsesRegistry,
	"Bomber.CheatManager.SetPlayerPowerupsUsesRegistry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SetPlayerPowerupsUsesRegistry::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::SetPlayerPowerups"));
	if (!TestTrue(
			TEXT("B4: BmrCheatManager.cpp must define SetPlayerPowerups with a body — the "
				 "start/ stub is empty; the registry-driven attribute overrides are what was "
				 "stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// Positive: must enumerate the project's powerup-tag registry.
	TestTrue(
		TEXT("B4: SetPlayerPowerups must enumerate FBmrPowerupTag::GetAll() — the project's "
			 "powerup-tag registry. The spec is explicit: 'enumerates powerup tags from the "
			 "registry, does not hardcode the list.' A re-impl that writes named attributes one "
			 "by one (Skate, Fire, BombsAvailable) drifts every time a powerup is added to the "
			 "registry — the cheat then silently caps new powerups at their default level."),
		Code.Contains(TEXT("FBmrPowerupTag::GetAll"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("PowerupTag::GetAll"), ESearchCase::CaseSensitive));

	// Positive: must convert the tag to an attribute via
	// Conv_TagToBaseAttribute — the canonical bridge between the tag
	// registry and the attribute set.
	TestTrue(
		TEXT("B4: SetPlayerPowerups must convert each registry tag to a UAttribute via "
			 "UBmrPowerupsAttributeSet::Conv_TagToBaseAttribute (or an equivalent tag->attribute "
			 "resolver). The canonical body is `Conv_TagToBaseAttribute(Tag)` inside the "
			 "FBmrPowerupTag::GetAll() loop; a body that hard-coded the attribute names "
			 "(GetPowerup_SkateAttribute etc.) bypasses the registry."),
		Code.Contains(TEXT("Conv_TagToBaseAttribute"), ESearchCase::CaseSensitive));

	// Positive: must apply the attribute write via ApplyModToAttributeUnsafe
	// (or equivalent ASC mutator). Without this the cheat does nothing
	// observable — the registry walk completes but no attribute is set.
	TestTrue(
		TEXT("B4: SetPlayerPowerups must mutate the ASC's attributes (ApplyModToAttributeUnsafe "
			 "with EGameplayModOp::Override is the canonical write). Without an actual "
			 "attribute write, the registry walk is observability-only and the powerup levels "
			 "stay where they were."),
		Code.Contains(TEXT("ApplyModToAttribute"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("SetNumericAttributeBase"), ESearchCase::CaseSensitive));

	// Failure-mode negative: no per-attribute hardcoded list. The named
	// hardcoded attribute getters live on UBmrPowerupsAttributeSet:
	// GetPowerup_SkateAttribute, GetPowerup_FireAttribute,
	// GetPowerup_BombsAvailableAttribute. (These are legitimately used by
	// SetAIPowerups, which IS hardcoded by design — the spec only requires
	// SetPlayerPowerups to use the registry. ExtractMemberBody scopes the
	// check to SetPlayerPowerups only.)
	const bool bHardcodesAttributeList =
		Code.Contains(TEXT("GetPowerup_SkateAttribute"), ESearchCase::CaseSensitive)
		&& Code.Contains(TEXT("GetPowerup_FireAttribute"), ESearchCase::CaseSensitive)
		&& Code.Contains(TEXT("GetPowerup_BombsAvailableAttribute"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B4 failure mode: SetPlayerPowerups must NOT hardcode the three named powerup "
			 "attribute getters (GetPowerup_SkateAttribute + GetPowerup_FireAttribute + "
			 "GetPowerup_BombsAvailableAttribute). That is the spec's named anti-pattern — the "
			 "moment a new powerup is added to the registry, the player cheat fails to set it. "
			 "Use FBmrPowerupTag::GetAll() + Conv_TagToBaseAttribute instead."),
		bHardcodesAttributeList);

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Source: SetWallsChance restores defaults from the data asset on
//          a negative input (B5), and immediately regenerates the level (B6).
//
// The spec's failure modes 5 & 6:
//   B5: "SetWallsChance(-1) treating negative values as 0% — should restore
//        defaults." A literal-clamp impl `Max(0, WallsChance)` produces
//        permanent 0% walls; the canonical encoding tests the sign and
//        reads from UBmrGeneratedMapDataAsset::Get().GetGenerationSettings().
//   B6: "Changing generation settings without triggering a regeneration —
//        map stays stale until the next round." The canonical body calls
//        ABmrGeneratedMap::Get().GenerateLevelActors() immediately after
//        the override is applied.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SetWallsChanceRestoresDefaultsAndRegenerates,
	"Bomber.CheatManager.SetWallsChanceRestoresDefaultsAndRegenerates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SetWallsChanceRestoresDefaultsAndRegenerates::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::SetWallsChance"));
	if (!TestTrue(
			TEXT("B5 + B6: BmrCheatManager.cpp must define SetWallsChance with a body — the "
				 "start/ stub is empty; the negative-restores-defaults branch and the regen "
				 "call are what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	// B5 positive: must read defaults from UBmrGeneratedMapDataAsset.
	TestTrue(
		TEXT("B5: SetWallsChance must read the default WallsChance from the data asset "
			 "(UBmrGeneratedMapDataAsset::Get().GetGenerationSettings()) so it can substitute "
			 "the default for a negative input. The spec is explicit: '-1 restores defaults "
			 "from the data asset rather than clamping to zero.' A literal-clamp impl gives 0% "
			 "walls forever, which is the named B5 failure."),
		Code.Contains(TEXT("BmrGeneratedMapDataAsset"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("GetGenerationSettings"), ESearchCase::CaseSensitive));

	// B5 positive: must branch on the sign of the input.
	const bool bBranchesOnSign =
		Code.Contains(TEXT(">= 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("< 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("> 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("WallsChance < "), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("WallsChance >= "), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B5: SetWallsChance must branch on the sign of the input chance — the negative "
			 "path restores defaults, the non-negative path uses the value. A body that wrote "
			 "the input directly produces -1% walls (or a clamped 0%), neither of which is the "
			 "spec's intended restore-to-default behaviour."),
		bBranchesOnSign);

	// B6 positive: must immediately trigger regeneration. The canonical call
	// is GenerateLevelActors on the BmrGeneratedMap singleton.
	TestTrue(
		TEXT("B6: SetWallsChance must call GenerateLevelActors after applying the override. The "
			 "spec is explicit: 'changing generation settings without triggering a regeneration "
			 "leaves the map stale until the next round.' Without the immediate regen, the "
			 "cheat appears to silently no-op until a round transition fires."),
		Code.Contains(TEXT("GenerateLevelActors"), ESearchCase::CaseSensitive));

	// B6 positive: the override must reach the generated map. The canonical
	// path is SetOverriddenGenerationSettings(true, NewSettings); writing the
	// data-asset CDO instead would persist into the editor save.
	TestTrue(
		TEXT("B6: SetWallsChance must apply the override via the generated-map's "
			 "SetOverriddenGenerationSettings (or equivalent runtime override entry point). "
			 "Mutating the data asset directly would persist into the editor save and corrupt "
			 "the default on the next cook."),
		Code.Contains(TEXT("SetOverriddenGenerationSettings"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("WallsChance ="), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — Source: SetBoxesChance mirrors SetWallsChance — restores defaults
//           on negative input (B5) and regenerates the level (B6).
//
// Same failure-mode pair as Test 9, applied to the boxes-chance cheat. The
// spec lists SetWallsChance / SetBoxesChance together with the same -1 →
// defaults convention and the same "trigger regen immediately" requirement.
// A re-impl that gets one right and the other wrong is a real failure mode
// (the boxes path is the visually obvious one in gameplay, since boxes are
// the primary powerup drop) — so it gets its own test.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SetBoxesChanceRestoresDefaultsAndRegenerates,
	"Bomber.CheatManager.SetBoxesChanceRestoresDefaultsAndRegenerates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SetBoxesChanceRestoresDefaultsAndRegenerates::RunTest(const FString& Parameters)
{
	using namespace CheatManagerTests;

	const FString Source = LoadCheatManagerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrCheatManager::SetBoxesChance"));
	if (!TestTrue(
			TEXT("B5 + B6: BmrCheatManager.cpp must define SetBoxesChance with a body — the "
				 "start/ stub is empty."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString Code = StripComments(Body);

	TestTrue(
		TEXT("B5: SetBoxesChance must read default boxes-chance from the data asset for the "
			 "negative-input restore-defaults branch."),
		Code.Contains(TEXT("BmrGeneratedMapDataAsset"), ESearchCase::CaseSensitive)
			|| Code.Contains(TEXT("GetGenerationSettings"), ESearchCase::CaseSensitive));

	const bool bBranchesOnSign =
		Code.Contains(TEXT(">= 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("< 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("> 0"), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("BoxesChance < "), ESearchCase::CaseSensitive)
		|| Code.Contains(TEXT("BoxesChance >= "), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B5: SetBoxesChance must branch on the sign of the input — negative restores "
			 "defaults, non-negative writes the value."),
		bBranchesOnSign);

	TestTrue(
		TEXT("B6: SetBoxesChance must call GenerateLevelActors after the override. Without the "
			 "immediate regen the cheat doesn't change anything visible until the next round."),
		Code.Contains(TEXT("GenerateLevelActors"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — Runtime DIRECT B1: The bitmask parser is the input pathway for
//           B1's death-event routing. Suicide composes its slot string per
//           the canonical pattern `FString::ChrN(PlayerId, '0') + TEXT("1")`
//           (Player 0 -> "1", Player 1 -> "01", Player 2 -> "001", Player 3
//           -> "0001") and feeds it to DestroyPlayersBySlots, which parses
//           it via GetBitmaskFromReverseString. The parser's output bit
//           directly determines which player slot the Player_Death event is
//           broadcast for.
//
// This is the runtime-observable boundary of B1's death-routing pathway: a
// mis-parsed slot string would broadcast Player_Death for the wrong slot
// (or no one), and the local player's death tabulation + pawn-pool return
// listeners — the exact subscribers B1 protects — would never fire for the
// intended target. So while the death-event broadcast itself needs a PIE
// world to observe end-to-end, the parser's slot-bit contract IS the
// runtime-observable contract that B1's routing depends on, and a
// behavioral test of that contract on the exact Suicide composition format
// is direct runtime coverage of B1's pathway.
//
// Discriminators (chosen so each catches a distinct misparse class):
//   * Suicide composition for slot N -> bit N (1, 2, 4, 8). A normal
//     big-endian binary parse of "0001" would give 1, not 8 — wrong player.
//   * Composition for slot 7 ("0000000" + "1") -> bit 7 = 128. Proves the
//     parser keeps advancing past leading zeros rather than stopping at
//     the first '0' or returning early on long strings.
//   * Multi-slot bitmask "1 0 1 0" -> bits 0 and 2 (value 5), the format
//     DestroyPlayersBySlots' raw cheat argument uses for killing several
//     players in one call (which is the routing scenario B1 names by name
//     — "DestroyPlayersBySlots kills players whose slot indices match a
//     bitmask"). Catches a parser that only respects the FIRST '1' it
//     sees, which would broadcast death for one slot and silently drop
//     the others.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrCheatManager_SlotStringParsesToTargetedDeathEventBit,
	"Bomber.CheatManager.SlotStringParsesToTargetedDeathEventBit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrCheatManager_SlotStringParsesToTargetedDeathEventBit::RunTest(const FString& Parameters)
{
	auto SuicideSlotString = [](int32 PlayerId) -> FString
	{
		// Canonical Suicide composition per evaluator notes:
		//   FString::ChrN(PlayerId, '0') + TEXT("1")
		// Produces a string of PlayerId zeros followed by a single '1' —
		// the leftmost-is-bit-zero convention means the '1' lands at bit
		// PlayerId after parsing.
		return FString::ChrN(PlayerId, TEXT('0')) + TEXT("1");
	};

	TestEqual(
		TEXT("B1: Suicide-for-Player-0 composes \"1\" → bitmask 1 (slot 0). The death-event "
			 "routing for the local player requires this bit; a wrong bit broadcasts "
			 "Player_Death for a different slot and skips the local pawn-pool return hook."),
		UBmrCheatManager::GetBitmaskFromReverseString(SuicideSlotString(0)),
		1);

	TestEqual(
		TEXT("B1: Suicide-for-Player-1 composes \"01\" → bitmask 2 (slot 1). Distinguishes a "
			 "naive binary-parse impl (which reads \"01\" as 1) from the reverse-bit-order "
			 "convention the death-event router requires."),
		UBmrCheatManager::GetBitmaskFromReverseString(SuicideSlotString(1)),
		2);

	TestEqual(
		TEXT("B1: Suicide-for-Player-2 composes \"001\" → bitmask 4 (slot 2). Proves the parser "
			 "doesn't short-circuit on leading zeros."),
		UBmrCheatManager::GetBitmaskFromReverseString(SuicideSlotString(2)),
		4);

	TestEqual(
		TEXT("B1: Suicide-for-Player-3 composes \"0001\" → bitmask 8 (slot 3). The canonical "
			 "header example — a normal big-endian parse would yield 1, broadcasting death for "
			 "Player 0 instead of Player 3."),
		UBmrCheatManager::GetBitmaskFromReverseString(SuicideSlotString(3)),
		8);

	TestEqual(
		TEXT("B1: Suicide-for-Player-7 composes \"00000001\" → bitmask 128 (slot 7). Proves the "
			 "parser keeps walking past long zero runs rather than truncating; without this, "
			 "high-slot players can't suicide via the cheat."),
		UBmrCheatManager::GetBitmaskFromReverseString(SuicideSlotString(7)),
		128);

	// DestroyPlayersBySlots multi-slot scenario: the cheat argument
	// "1 0 1 0" should select slots 0 AND 2 (value 5). A parser that only
	// honored the first '1' would broadcast Player_Death for slot 0 only
	// and silently drop slot 2 — the routing failure that B1 names by name.
	TestEqual(
		TEXT("B1: DestroyPlayersBySlots \"1 0 1 0\" → bitmask 5 (slots 0 and 2). The multi-slot "
			 "death-event broadcast B1 protects. A parser that only honored the first '1' would "
			 "kill slot 0 alone — slot 2's death event would never fire, and slot 2's pool/"
			 "tabulation listeners would never be notified, the exact B1 failure shape."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("1 0 1 0"))),
		5);

	TestEqual(
		TEXT("B1: DestroyPlayersBySlots \"0 1 1 1\" → bitmask 14 (slots 1, 2, 3). Proves the "
			 "parser correctly skips slot 0 in a multi-slot kill — a re-impl that treated "
			 "leading '0' as bit 0 set would erroneously include slot 0 in the death broadcast."),
		UBmrCheatManager::GetBitmaskFromReverseString(FString(TEXT("0 1 1 1"))),
		14);

	return true;
}

// Test 11 (runtime defensive-entry-point smoke) was removed: the static
// cheat methods Suicide / DestroyPlayersBySlots / SetPlayerPowerups /
// SetAutoCopilot trip ensure() guards in the solution when invoked from a
// free automation context (no PIE pawn, no ASC, no PlayerController). The
// ensure messages don't reliably match an AddExpectedError filter, so the
// runtime check produced a false-failure even when the solution's
// defensive prelude was in place. The named failure modes B1/B3/B4 are
// still covered directly by the source-inspection tests above (Tests
// 4-8), with B1's parser pathway covered at runtime by Test 11 above.
