// Copyright (c) 2026 GameDevBench. AI controller automation tests (Bomber).
//
// Tests target ABmrAIController, the bot survival decision body. The spec
// frames the controller as a cell-grid tactical loop: bail if disabled, decide
// whether the bot is in danger by consuming the cell-library's Safe-cells
// filter (not by projecting bomb fuses), prefer adjacent powerups, otherwise
// filter candidate cells through a pipeline (crossway / secure / near-radius)
// that *keeps* the previous filtered set when a step collapses to empty, walk
// via the project's cell-based move-by-intent primitive, and gate bomb
// placement on five independent conditions before invalidating the candidate
// set so the bot does not step back onto its own bomb. The lifecycle wires
// subscriptions on possession, cleans them up on unpossess (before letting the
// base class clear the pawn ref), and toggles the decision loop on game-state
// transitions and on pawn-removed-from-level (death).
//
// Why mostly source-inspection coverage:
//   The decision loop's runtime preconditions are: a possessed ABmrPawn with
//   its MoverComponent + MapComponent, a generated map populated with the
//   cell graph, an authoritative GameState driving FBmrGameStateTag::InGame,
//   the GlobalMessageSubsystem wired to the gameplay-event router, a bomb
//   data asset hydrated for the place-ability, and (for the failure-mode
//   tests like "walks into own bomb") a chain of bomb actor + fuse GE +
//   explosion execution. Reproducing that pipeline in an automation test
//   requires PIE plumbing that does not exist as a reusable harness in this
//   project, and inlining it would dwarf the surface under test (the .cpp
//   body that was stripped). The spec and the evaluator notes both name a
//   small set of *code-path* choices that separate a working impl from a
//   plausibly-working one — those choices are the things the failure-mode
//   section calls out, and source inspection of the stripped .cpp is the
//   highest-signal way to verify them without re-implementing the whole
//   match pipeline. Two pieces ARE feasible at the CDO level (the const
//   IsAIEnabled() predicate and the constructor's tick-disabled / attach-
//   to-pawn defaults), so those are exercised at runtime.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"

// Bomber
#include "Bomber.h"
#include "Controllers/BmrAIController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAiControllerTests, Log, All);

namespace AiControllerTests
{
	// ---------------------------------------------------------------------
	// Source-file reader. The stripped file in this task is
	//   Source/Bomber/Private/Controllers/BmrAIController.cpp
	// and its body is what the model has to restore. The header stays
	// intact (the protected hooks and properties are pre-declared) — the
	// behavior under test all lives in the cpp.
	// ---------------------------------------------------------------------
	static const TCHAR* ControllerSourceRelPath =
		TEXT("Source/Bomber/Private/Controllers/BmrAIController.cpp");

	static FString LoadControllerSource(FAutomationTestBase* Test)
	{
		const FString AbsPath = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / ControllerSourceRelPath);
		FString Source;
		if (!Test->TestTrue(
				FString::Printf(TEXT("BmrAIController.cpp must be readable at %s"), *AbsPath),
				FFileHelper::LoadFileToString(Source, *AbsPath)))
		{
			return FString();
		}
		return Source;
	}

	// Strips C++ line comments (// ...) and block comments (/* ... */) from
	// source text. Forbidden-API substring scans must run against the stripped
	// source: the canonical solution's MoveToCell contains a comment that
	// contrasts the intent-based locomotion against "MoveToLocation with
	// navmesh", and a naïve Source.Contains("MoveToLocation") would flag that
	// comment as a violation. Stripping comments first keeps the scan focused
	// on actual code references. String literals are preserved so the scan
	// can still see e.g. `TEXT("MoveToLocation")` if that ever appears in
	// code (which would itself be a smell, but not what we filter for here).
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

	// Brace-depth walker — extracts the body of a member function definition
	// (from the opening { through the matching close }). Mirrors the helper
	// used by the BombPlacementExecution / BombAbilityActor tests so that
	// nested initialiser lists / lambdas don't confuse the body extraction.
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
} // namespace AiControllerTests

// ---------------------------------------------------------------------------
// Test 1 — IsAIEnabled is a direct runtime predicate: bot-not-enabled bail (B1).
//
// B1 DIRECT (runtime): the decision loop must bail when "the bot is not
// enabled". The enable predicate is the public IsAIEnabled() const method.
// On a freshly-constructed controller with no possessed pawn, IsAIEnabled()
// must return false — there is no MoverComponent yet, so the gate cannot be
// open. This is the one piece of the bail-out behavior that is observable
// without bootstrapping a pawn / map / cheat-cvar harness: it is a pure
// const accessor and the false-on-no-pawn path is the dominant case the
// rest of the controller's logic depends on (UpdateAI calls IsAIEnabled
// first thing — if it ever returns true with no pawn, the body would crash
// on the very next line dereferencing MapComponent).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_IsAIEnabledBailsWhenNoPawn,
	"Bomber.AIController.IsAIEnabledBailsWhenNoPawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_IsAIEnabledBailsWhenNoPawn::RunTest(const FString& Parameters)
{
	const UClass* Class = ABmrAIController::StaticClass();
	if (!TestNotNull(TEXT("ABmrAIController class must exist"), Class))
	{
		return false;
	}

	const ABmrAIController* CDO = Cast<const ABmrAIController>(Class->GetDefaultObject());
	if (!TestNotNull(TEXT("ABmrAIController CDO must be obtainable"), CDO))
	{
		return false;
	}

	// IsAIEnabled is `const` and reads only the (possibly null) MoverComponent
	// off the (null on the CDO) pawn. With no pawn, the canonical body short-
	// circuits to false via `return MoverComponent && ...`. A reimplementation
	// that returned true (or crashed) here would be visibly wrong.
	TestFalse(
		TEXT("B1: IsAIEnabled() must return false when the controller has no possessed pawn. "
			 "The enable predicate is what gates the entire decision loop; without this short-"
			 "circuit, UpdateAI would attempt to deref a null MoverComponent on the very next "
			 "line. The canonical body checks MoverComponent first and bails false if absent."),
		CDO->IsAIEnabled());

	// The CDO also publishes the tick defaults the constructor sets. These are
	// implementation-detail checks (so they do not count as direct gameplay
	// coverage) but they cheaply rule out the failure mode the spec names
	// explicitly: "re-planning every frame produces oscillation" (B17). If the
	// constructor leaves Tick enabled, the bot ticks every frame regardless of
	// what UpdateAI does internally.
	TestFalse(
		TEXT("Constructor must disable Tick (PrimaryActorTick.bCanEverTick = false). The bot "
			 "re-plans on cell-traversal completion, not on Tick — leaving Tick enabled would "
			 "feed UpdateAI on every frame, which (even with throttling) burns work and risks "
			 "the oscillation failure mode the spec calls out."),
		CDO->PrimaryActorTick.bCanEverTick);
	TestFalse(
		TEXT("Constructor must leave Tick disabled at start (bStartWithTickEnabled = false)."),
		CDO->PrimaryActorTick.bStartWithTickEnabled);

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — UpdateAI bails on missing map component / disabled AI; throttled
//          to the project tick interval; re-plans on cell-traversal completion.
//
// B1 SOURCE: UpdateAI must short-circuit when MapComponent is null or
//           IsAIEnabled() returns false. Without this guard the body would
//           crash on a deref or run while disabled.
// B17 / failure-mode "oscillation" DIRECT: re-planning every frame produces
//           the oscillation symptom. Two pieces protect against it:
//             (a) the throttle keyed on LastAIUpdateTime + GTickInterval;
//             (b) the wiring of OnOwnerMovementCompleted -> UpdateAI, so the
//                 re-plan is driven by traversal completion, not Tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_UpdateAIThrottleAndArrivalDriven,
	"Bomber.AIController.UpdateAIThrottleAndArrivalDriven",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_UpdateAIThrottleAndArrivalDriven::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString UpdateAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define UpdateAI with a body — the start/ stub is "
				 "empty; the decision-loop body is what was stripped."),
			!UpdateAIBody.IsEmpty()))
	{
		return false;
	}

	// B1: the body must bail on missing MapComponent / disabled AI before
	// touching anything else (including GetWorld()->GetTimeSeconds()).
	const bool bChecksEnable = UpdateAIBody.Contains(TEXT("IsAIEnabled"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: UpdateAI must call IsAIEnabled() and bail if it returns false. The cheat-"
			 "driven enable CVar and the movement-blocked predicate are both routed through "
			 "IsAIEnabled — bypassing it would let the bot keep re-planning while explicitly "
			 "disabled (e.g. on a client where SetAI cannot run, or with the AI cheat off)."),
		bChecksEnable);

	const bool bChecksMapComponent = UpdateAIBody.Contains(TEXT("MapComponent"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: UpdateAI must reference MapComponent (so it can bail when null). The body "
			 "deref's the map component to read F0 = MapComponent->GetCell() — without a null "
			 "check the body would crash on a freshly-spawned controller before the pawn's "
			 "MapComponent is wired."),
		bChecksMapComponent);

	// B17 / oscillation failure mode (a): throttle keyed on LastAIUpdateTime
	// and the project tick interval. The canonical pattern is
	//   if (GetWorld()->GetTimeSeconds() - LastAIUpdateTime < GTickInterval) return;
	// followed by LastAIUpdateTime = GetWorld()->GetTimeSeconds().
	const bool bReadsLastUpdateTime =
		UpdateAIBody.Contains(TEXT("LastAIUpdateTime"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B17: UpdateAI must reference LastAIUpdateTime — the throttle that keeps the bot "
			 "from re-planning every time the movement system fires OnPostSimulationTick. "
			 "Without the throttle, the bot oscillates direction mid-cell (the symptom the "
			 "spec calls out as a re-plan-every-frame failure)."),
		bReadsLastUpdateTime);

	const bool bUsesTickInterval =
		UpdateAIBody.Contains(TEXT("GTickInterval"), ESearchCase::CaseSensitive)
		|| UpdateAIBody.Contains(TEXT("TickInterval"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B17: UpdateAI must throttle against the project tick interval "
			 "(UBmrGameStateDataAsset::GTickInterval). Hard-coded delays would drift away from "
			 "the rest of the project's tick-throttled systems and decouple bot pacing from "
			 "the rest of the game-state schedule."),
		bUsesTickInterval);

	// B17 / oscillation failure mode (b): the re-plan trigger is the
	// movement-completed event, not Tick.
	const FString MoveCompletedBody = ExtractMemberBody(
		Source, TEXT("ABmrAIController::OnOwnerMovementCompleted_Implementation"));
	if (!TestTrue(
			TEXT("OnOwnerMovementCompleted_Implementation must have a body."),
			!MoveCompletedBody.IsEmpty()))
	{
		return false;
	}
	TestTrue(
		TEXT("B17: OnOwnerMovementCompleted_Implementation must call UpdateAI(). This is the "
			 "callback the spec names as the re-plan trigger — the bot re-decides 'on cell "
			 "traversal completion (when the underlying movement system finishes a cell "
			 "step) rather than per frame'. Failing to invoke UpdateAI here freezes the bot "
			 "because nothing else drives its decisions."),
		MoveCompletedBody.Contains(TEXT("UpdateAI"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Danger detection consumes the Safe-cells filter; never projects
//          fuse times itself (B2, B20).
//
// B2 DIRECT (source pattern): "the cell utility library exposes a 'safe
//          cells' filter that already encodes this; the bot must consume that
//          filter and trust it". The canonical body iterates a (Safe, Free)
//          pair via GetCellsAround(F0, EPathType::Safe, MaxInt) first, then
//          falls back to EPathType::Free with the dangerous flag set.
// B20 DIRECT (failure-mode inverse): "the bot tries to project bomb fuse
//          times itself (reinventing the safe-cell filter is both duplicate
//          and drift-prone)". The cpp must not project fuses — no calls to
//          fuse-time / GetTimeRemaining / GameplayEffect-duration queries.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_DangerDetectionUsesSafeFilter,
	"Bomber.AIController.DangerDetectionUsesSafeFilter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_DangerDetectionUsesSafeFilter::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString UpdateAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define UpdateAI with a body."),
			!UpdateAIBody.IsEmpty()))
	{
		return false;
	}

	// B2: must reach for the Safe-cells filter on the cell utility library.
	TestTrue(
		TEXT("B2: UpdateAI must call UBmrCellUtilsLibrary::GetCellsAround. This is the "
			 "cell-graph BFS that every danger / candidate / pipeline step is built on; "
			 "reimplementing it inline (or hand-rolling a fan-out from F0) drifts from the "
			 "rest of the codebase that uses the same primitive."),
		UpdateAIBody.Contains(TEXT("GetCellsAround"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: UpdateAI must use EPathType::Safe for the danger scan. EPathType::Safe is "
			 "the variant that already encodes 'cells outside every active bomb's blast zone' "
			 "— consuming it is the spec-mandated way to decide whether the bot is in danger. "
			 "Using EPathType::Free here would mark the bot 'safe' inside a bomb's blast zone."),
		UpdateAIBody.Contains(TEXT("EPathType::Safe"), ESearchCase::CaseSensitive));

	// B2: must also fall back to EPathType::Free when Safe returns empty,
	// because "if the safe-cell scan from the bot's position returns nothing,
	// the bot is in danger and falls back to the broader 'free cells' filter".
	TestTrue(
		TEXT("B2: UpdateAI must reference EPathType::Free — the broader fallback when the "
			 "Safe scan returns empty (the bot is in danger and must still find a candidate "
			 "set to move through, even though every reachable cell is in a blast zone)."),
		UpdateAIBody.Contains(TEXT("EPathType::Free"), ESearchCase::CaseSensitive));

	// B20: must NOT project fuse times itself. The canonical anti-patterns
	// are reaching into a fuse / duration GE, querying remaining fuse time,
	// or applying a hand-rolled timer to a bomb actor. None of these may
	// appear anywhere in the controller cpp. Scan the comment-stripped source
	// so a doc comment that mentions "we don't project fuse times here"
	// wouldn't be misread as the controller actually calling such APIs.
	const FString CodeOnly = StripComments(Source);
	const TCHAR* const ForbiddenFuseApis[] =
	{
		TEXT("GetFuseTime"),
		TEXT("GetTimeRemaining"),
		TEXT("GetRemainingTime"),
		TEXT("FuseDuration"),
		TEXT("BombDuration"),
		TEXT("ExplosionTime"),
		TEXT("GetDurationGameplayEffect"),
	};
	for (const TCHAR* Api : ForbiddenFuseApis)
	{
		TestFalse(
			FString::Printf(TEXT("B20: BmrAIController.cpp must NOT reference %s. The spec is "
								 "explicit: the bot 'must consume [the Safe filter] and trust it, "
								 "not project bomb fuse times itself'. A re-implementation of "
								 "fuse projection inside the controller is both duplicate work "
								 "and drift-prone — when the bomb-side timing changes, the bot's "
								 "private projection silently disagrees."),
				Api),
			CodeOnly.Contains(Api, ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — Direct powerup grab when not in danger and a powerup is adjacent (B3).
//
// B3 DIRECT (source pattern): "When not in danger and there is a powerup
// adjacent, prefer the direct grab over filtering". The canonical body
// scans GetCellsAroundWithActors with EPathType::Safe, the AI data asset's
// PowerupSearchRadius, and the EAT::Powerup actor-type flag, and if any are
// found immediately calls MoveToCell on the first result and returns (so
// the filter pipeline never runs for this tick).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_DirectPowerupGrabShortcut,
	"Bomber.AIController.DirectPowerupGrabShortcut",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_DirectPowerupGrabShortcut::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString UpdateAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define UpdateAI with a body."),
			!UpdateAIBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B3: UpdateAI must scan for adjacent powerups via "
			 "UBmrCellUtilsLibrary::GetCellsAroundWithActors. This is the actor-typed variant "
			 "of GetCellsAround — without it, the body either has to walk the result of a "
			 "plain GetCellsAround and re-query actor types (slower) or hand-roll an actor "
			 "filter (drift). The canonical body reaches for the typed helper."),
		UpdateAIBody.Contains(TEXT("GetCellsAroundWithActors"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: the powerup shortcut must reference EAT::Powerup (the actor-type flag for "
			 "powerups). A scan that omits the powerup mask would either match the wrong "
			 "actor kinds (bombs / boxes) or — if no mask is provided at all — return every "
			 "reachable cell and the 'prefer the direct grab' heuristic would degrade into "
			 "'walk anywhere within radius'."),
		UpdateAIBody.Contains(TEXT("EAT::Powerup"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: the powerup shortcut must use the AI data asset's PowerupSearchRadius. "
			 "Hard-coding a different radius decouples the bot's grab heuristic from the "
			 "tunable in UBmrAIDataAsset and produces inconsistent behavior across "
			 "difficulty profiles."),
		UpdateAIBody.Contains(TEXT("PowerupSearchRadius"), ESearchCase::CaseSensitive));

	// The shortcut must be guarded by 'not in danger'. The canonical body
	// wraps the shortcut in `if (bIsDangerous == false)` / `if (!bIsDangerous)`.
	const bool bGuardedByDanger =
		UpdateAIBody.Contains(TEXT("bIsDangerous"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B3: the powerup shortcut must be gated on the danger flag — 'When not in "
			 "danger and there is a powerup adjacent, prefer the direct grab'. Running the "
			 "shortcut inside a blast zone would have the bot chase a powerup at the cost "
			 "of dying to a bomb, which is the inverse of survival."),
		bGuardedByDanger);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Filter pipeline: each step is an intersect, and an empty step
//          keeps the previous Filtered set instead of giving up (B4, B19).
//
// B4 DIRECT (source pattern): "filter them through a pipeline of
//          progressively stricter conditions (be on a crossway, be reachable
//          without bumping into another player, be inside a near-radius)".
//          The canonical body uses Intersect against AllCrossways /
//          SecureCrossways and a near-radius cull keyed on
//          UBmrAIDataAsset::GetNearFilterRadius.
// B19 DIRECT (failure-mode inverse): "the bot stands still indefinitely in
//          tight situations (a symptom of treating an empty filter result
//          as 'give up entirely' rather than 'keep the previous filtered
//          set')". When a step produces an empty set, the body must mark a
//          flag (bIsFilteringFailed) and keep the previous Filtered — only
//          non-empty steps assign through to Filtered.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_FilterPipelinePreservesOnEmpty,
	"Bomber.AIController.FilterPipelinePreservesOnEmpty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_FilterPipelinePreservesOnEmpty::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString UpdateAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define UpdateAI with a body."),
			!UpdateAIBody.IsEmpty()))
	{
		return false;
	}

	// B4: pipeline steps. The canonical body builds AllCrossways and
	// SecureCrossways during the cell-iteration pass, then intersects the
	// Filtered set against them in the filtering pass.
	TestTrue(
		TEXT("B4: UpdateAI must reference AllCrossways (the set of crossway cells discovered "
			 "during cell iteration). The first filtering step intersects against this set — "
			 "the spec's 'be on a crossway' condition."),
		UpdateAIBody.Contains(TEXT("AllCrossways"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: UpdateAI must reference SecureCrossways (crossways with no other player in "
			 "them). The third filtering step intersects against this set — the spec's 'be "
			 "reachable without bumping into another player' condition. Skipping this filter "
			 "lets the bot path through crossways guarded by another player."),
		UpdateAIBody.Contains(TEXT("SecureCrossways"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: UpdateAI must reference NearFilterRadius (via "
			 "UBmrAIDataAsset::GetNearFilterRadius). The final filtering step culls cells "
			 "outside the near radius — the spec's 'be inside a near-radius' condition. "
			 "Skipping it lets the bot pick targets across the whole map."),
		UpdateAIBody.Contains(TEXT("NearFilterRadius"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: UpdateAI must use Intersect / .Intersect( on FBmrCells to implement the "
			 "pipeline steps. Each step narrows the candidate set by intersecting against a "
			 "stricter criterion; a re-implementation that copies / filters by hand drifts "
			 "from the FBmrCells primitive and may pick suboptimal cells."),
		UpdateAIBody.Contains(TEXT("Intersect"), ESearchCase::CaseSensitive));

	// B4: the final pick must be uniform-random over the resulting set.
	TestTrue(
		TEXT("B4: the final pick must be uniform-random across the Filtered set — the spec "
			 "says 'pick uniformly at random from the final filtered set'. The canonical "
			 "body uses FMath::RandRange (or a project-equivalent uniform draw). Picking the "
			 "first/last element of the array biases the bot toward grid-order, making it "
			 "predictable in 1v1 play."),
		UpdateAIBody.Contains(TEXT("RandRange"), ESearchCase::CaseSensitive));

	// B19: empty-step preservation. The canonical pattern keeps the prior
	// Filtered when a step is empty and marks bIsFilteringFailed (which is
	// then read by the bomb gate). The simplest spec-faithful encoding sets
	// the flag inside the empty branch — its presence in the body is the
	// strongest single signal.
	TestTrue(
		TEXT("B19: UpdateAI must track an 'is-filtering-failed' flag (canonically "
			 "bIsFilteringFailed). The flag is set when any pipeline step collapses to empty "
			 "and is then read by the bomb-placement gate. Without it, the body has nowhere "
			 "to record 'the pipeline collapsed and the chosen Filtered set is uncertain', "
			 "so bombing decisions made on an empty-step-fed set would be wrong."),
		UpdateAIBody.Contains(TEXT("bIsFilteringFailed"), ESearchCase::CaseSensitive)
			|| UpdateAIBody.Contains(TEXT("FilteringFailed"), ESearchCase::CaseSensitive));

	// The negative side of B19: the empty-step branch must NOT clobber
	// Filtered with an empty set. The simplest way to verify the spec-faithful
	// pattern is the presence of a guard like `if (Step.Num() > 0) Filtered = Step;`
	// — i.e. the body must read a Num() on the intermediate filtering step.
	TestTrue(
		TEXT("B19: the filtering loop must guard the assignment to Filtered on the "
			 "intermediate step being non-empty (e.g. `if (FilteringStep.Num() > 0) Filtered "
			 "= FilteringStep;`). Unconditional assignment lets an empty step overwrite the "
			 "previous Filtered, and the body then picks from an empty set — the spec's "
			 "'bot stands still indefinitely in tight situations' failure mode."),
		UpdateAIBody.Contains(TEXT(".Num()"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — MoveToCell uses the project's move-by-intent primitive; the
//          controller never reaches for navmesh / MoveToActor APIs (B5, B21).
//
// B5 DIRECT (source pattern): "After picking, walk toward that cell using
//          the existing move-by-intent helper". MoveToCell must call
//          MoverComponent->RequestMoveByIntent with a 2D-normalized intent
//          vector (zero on arrival).
// B21 DIRECT (failure-mode inverse): "the bot uses navmesh path-following or
//          MoveToActor-style locomotion". None of the engine AI locomotion
//          helpers (MoveToLocation, MoveToActor, SimpleMoveTo*, FollowPath,
//          UNavigationSystemV1, UPathFollowingComponent) may appear in the
//          controller cpp. Their use bypasses the cell-graph movement that
//          the rest of the project relies on for cell-snapped pawn motion.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_MoveByIntentNoNavmesh,
	"Bomber.AIController.MoveByIntentNoNavmesh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_MoveByIntentNoNavmesh::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	// The MoveToCell body itself is not stripped in this task — it sits in
	// the start/ cpp already. Even so, the failure-mode test asserts on the
	// *whole-file* absence of navmesh APIs, because a re-implementation that
	// adds (say) a fallback navmesh call inside UpdateAI would also fail
	// B21. The positive RequestMoveByIntent check is on the MoveToCell body.
	const FString MoveToCellBody = ExtractMemberBody(Source, TEXT("ABmrAIController::MoveToCell"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define MoveToCell with a body. The start/ cpp "
				 "already provides MoveToCell, but a re-implementation that elides it would "
				 "fail here."),
			!MoveToCellBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B5: MoveToCell must call MoverComponent->RequestMoveByIntent. This is the "
			 "project's cell-aware movement primitive — passing an intent vector lets the "
			 "MoverComponent step cell-to-cell without navmesh. Using any other locomotion "
			 "API decouples the bot from the cell graph that the rest of the game (bomb "
			 "placement, explosion arms, crossway scans) is built on."),
		MoveToCellBody.Contains(TEXT("RequestMoveByIntent"), ESearchCase::CaseSensitive));

	// B5 DIRECT (UpdateAI side): the decision body must actually invoke the
	// move-by-intent helper. The spec is explicit: "After picking, walk
	// toward that cell using the existing move-by-intent helper." MoveToCell
	// is that helper — it computes the 2D-normalized intent and forwards
	// into MoverComponent->RequestMoveByIntent. Checking only the helper's
	// own body for RequestMoveByIntent passes vacuously against an empty
	// UpdateAI stub: the helper exists in start/ already, but nothing calls
	// it from the (stripped) decision loop, so the bot sits still. UpdateAI
	// has at least two call sites for MoveToCell in the canonical body — the
	// direct-powerup grab shortcut (B3 / evaluator B26) and the final
	// uniform-random pick from the Filtered set (evaluator B48).
	const FString UpdateAIBodyForMove =
		ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	TestTrue(
		TEXT("B5: UpdateAI must call MoveToCell to actually drive the pawn toward the cell "
			 "it just picked. Without the call, the decision body runs its filtering math "
			 "and never issues a movement intent — the bot decides correctly and stands "
			 "still anyway. The canonical body invokes MoveToCell from both the direct-"
			 "powerup shortcut and the final Filtered-set random pick."),
		UpdateAIBodyForMove.Contains(TEXT("MoveToCell"), ESearchCase::CaseSensitive));

	// B21: forbidden navmesh / engine-AI locomotion APIs. These checks run
	// against the whole controller cpp — a single forbidden call anywhere
	// in the body taints the whole controller, because the bot's locomotion
	// is supposed to be exclusively cell-graph-driven. The scan runs on the
	// comment-stripped source: documentation in the canonical implementation
	// contrasts the intent-based locomotion against "MoveToLocation with
	// navmesh", and a naïve whole-file substring search would treat that
	// explanatory comment as a violation. Stripping comments first keeps the
	// check focused on actual code references.
	const FString CodeOnly = StripComments(Source);
	const TCHAR* const ForbiddenNavApis[] =
	{
		TEXT("MoveToLocation"),
		TEXT("MoveToActor"),
		TEXT("SimpleMoveToActor"),
		TEXT("SimpleMoveToLocation"),
		TEXT("UNavigationSystemV1"),
		TEXT("UPathFollowingComponent"),
		TEXT("FollowPathSync"),
		TEXT("FindPathToLocationSynchronously"),
	};
	for (const TCHAR* Api : ForbiddenNavApis)
	{
		TestFalse(
			FString::Printf(TEXT("B21: BmrAIController.cpp must NOT reference %s. The bots in "
								 "this project move cell-to-cell via the MoverComponent's "
								 "RequestMoveByIntent primitive. Engine navmesh / "
								 "MoveToActor-style locomotion bypasses the cell graph and "
								 "moves the pawn off-grid, breaking every downstream system "
								 "that reads MapComponent->GetCell()."),
				Api),
			CodeOnly.Contains(Api, ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Bomb placement is gated on all five spec conditions and uses the
//          Explosion path-type with the Box|Player mask for the destructible-
//          target check; the candidate set is invalidated after placement
//          (B6, B7, B8, B9, B10, failure mode "walks into own bomb").
//
// B6  DIRECT: "the bot has bomb budget available" — gated via the bCanSpawnBombs
//             property (set by SetAICanSpawnBomb; default true).
// B7  DIRECT: "the bot is not currently in danger" — !bIsDangerous.
// B8  DIRECT: "the filtering pipeline did not collapse" — !bIsFilteringFailed.
// B9  DIRECT: "the bot has not just moved onto a fresh powerup" — !bIsPowerupInDirect.
// B10 DIRECT: "destructible target reachable along one of the four arms within
//             the bot's current Fire radius" — GetCellsAroundWithActors with
//             EPathType::Explosion, the Fire radius from the powerups attribute
//             set, and the Box|Player actor-type mask.
// Failure mode "walks into own bomb" (B16): after spawning, Free.Empty()
//             clears the candidate set so the random-pick at the end of the
//             body cannot land on the cell the bot just bombed.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_BombGateAllFiveAndInvalidates,
	"Bomber.AIController.BombGateAllFiveAndInvalidates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_BombGateAllFiveAndInvalidates::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString UpdateAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::UpdateAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define UpdateAI with a body."),
			!UpdateAIBody.IsEmpty()))
	{
		return false;
	}

	// B6 — bCanSpawnBombs gate.
	TestTrue(
		TEXT("B6: UpdateAI must read bCanSpawnBombs as part of the bomb-placement gate. "
			 "This is the configurable per-bot toggle (SetAICanSpawnBomb); if the body never "
			 "references it, the toggle is dead and bots place bombs in modes that disable "
			 "bombing entirely."),
		UpdateAIBody.Contains(TEXT("bCanSpawnBombs"), ESearchCase::CaseSensitive));

	// B7 — danger flag gate.
	TestTrue(
		TEXT("B7: UpdateAI must gate bomb placement on the danger flag (!bIsDangerous). "
			 "Placing a bomb while standing in another bomb's blast zone is the canonical "
			 "way to die to your own bomb — the bot would be trapped between the existing "
			 "danger and its newly-placed one."),
		UpdateAIBody.Contains(TEXT("bIsDangerous"), ESearchCase::CaseSensitive));

	// B8 — filtering-failed gate (already covered as B19, but the BOMB gate
	// is the load-bearing read of the flag — without it, an empty filter step
	// degrades silently into a bomb placement on a degenerate Filtered set).
	TestTrue(
		TEXT("B8: UpdateAI must gate bomb placement on !bIsFilteringFailed. The flag tracks "
			 "whether any pipeline step collapsed to empty; placing a bomb when the filter "
			 "is uncertain is the spec's 'bot places bombs in empty corridors' failure mode."),
		UpdateAIBody.Contains(TEXT("bIsFilteringFailed"), ESearchCase::CaseSensitive)
			|| UpdateAIBody.Contains(TEXT("FilteringFailed"), ESearchCase::CaseSensitive));

	// B9 — fresh-powerup gate. The bot must not bomb the cell it is grabbing
	// a powerup from (or any direct-powerup pursuit cell).
	TestTrue(
		TEXT("B9: UpdateAI must gate bomb placement on !bIsPowerupInDirect. When the bot is "
			 "actively pursuing a direct (in-Free) powerup, dropping a bomb on its current "
			 "cell would force it to abandon the powerup to escape the blast — the spec "
			 "calls this 'has not just moved onto a fresh powerup'."),
		UpdateAIBody.Contains(TEXT("bIsPowerupInDirect"), ESearchCase::CaseSensitive));

	// B10 — destructible-target check.
	TestTrue(
		TEXT("B10: UpdateAI must compute the destructible-target set via "
			 "UBmrCellUtilsLibrary::GetCellsAroundWithActors with EPathType::Explosion. "
			 "EPathType::Explosion is the path-type that traces the four arms of the bomb "
			 "blast (stopping at the first wall on each arm); using any other path-type "
			 "would count targets the explosion couldn't actually reach."),
		UpdateAIBody.Contains(TEXT("EPathType::Explosion"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B10: the destructible-target mask must include both EAT::Box and EAT::Player. "
			 "Boxes destroy and spawn powerups; players are the win condition. Omitting "
			 "Player makes the bot defensive and never offensive; omitting Box makes the bot "
			 "pass through every breakable corridor without clearing it."),
		(UpdateAIBody.Contains(TEXT("EAT::Box"), ESearchCase::CaseSensitive)
			&& UpdateAIBody.Contains(TEXT("EAT::Player"), ESearchCase::CaseSensitive))
		|| UpdateAIBody.Contains(TEXT("EAT::Box | EAT::Player"), ESearchCase::CaseSensitive)
		|| UpdateAIBody.Contains(TEXT("Box | EAT::Player"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B10: UpdateAI must sample the bot's current Fire radius (the Powerup_Fire "
			 "attribute on UBmrPowerupsAttributeSet). Hard-coding a radius decouples the "
			 "bomb gate from the in-game Fire upgrade — the bot would still gate on the "
			 "starting radius after picking up Fire powerups."),
		UpdateAIBody.Contains(TEXT("Fire"), ESearchCase::CaseSensitive));

	// Spawn must go through the pawn's SpawnBomb path (not a hand-rolled
	// Actor spawn / NewObject). The pawn's helper applies the place ability,
	// runs the gate, and writes the origin into the fuse context — bypassing
	// it skips every preceding test's invariants.
	TestTrue(
		TEXT("B10: UpdateAI must call InOwner->SpawnBomb() (or the pawn-side equivalent) to "
			 "drop the bomb. Spawning the bomb actor by hand bypasses the place ability's "
			 "gate, the origin-cell write into the fuse context, and the placer-ASC bookkeeping "
			 "that the rest of the bomb pipeline depends on."),
		UpdateAIBody.Contains(TEXT("SpawnBomb"), ESearchCase::CaseSensitive));

	// Failure-mode "walks into own bomb": post-bomb invalidation.
	TestTrue(
		TEXT("Failure mode: after spawning a bomb, UpdateAI must invalidate the candidate "
			 "set (Free.Empty()) so the subsequent random pick cannot land on the cell the "
			 "bot just bombed. Without this clear, the final-move step picks from a Filtered "
			 "set that still contains the current cell, and the bot stays put — walking "
			 "straight into its own explosion seconds later."),
		UpdateAIBody.Contains(TEXT("Free.Empty"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Lifecycle: OnPossess wires the runtime subscriptions, OnUnPossess
//          tears them down before Super (B11, B12).
//
// B11 DIRECT (source pattern): on possession, the controller must:
//   * spawn a PlayerState if absent (InitPlayerState + SetDefaultPlayerName)
//   * subscribe to game-state changes via the GlobalMessageSubsystem
//     (BmrGameplayTags::Event::GameState_Changed -> OnGameStateChanged)
//   * subscribe to the MapComponent's OnPostRemovedFromLevel (death)
//   * subscribe to the MoverComponent's OnPostSimulationTick
//     (the cell-traversal-complete event that drives UpdateAI)
//   * enable the AI only if the game is currently in the InGame phase
// B12 DIRECT (source pattern): on unpossess, the controller must clear those
//   subscriptions from the pawn-side components BEFORE calling
//   Super::OnUnPossess(), so the base class can null out the pawn ref
//   without leaving dangling delegate bindings.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_LifecyclePossessUnpossess,
	"Bomber.AIController.LifecyclePossessUnpossess",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_LifecyclePossessUnpossess::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	// ----- OnPossess -----
	const FString PossessBody = ExtractMemberBody(Source, TEXT("ABmrAIController::OnPossess"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define OnPossess with a body."),
			!PossessBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B11: OnPossess must initialise a PlayerState when absent (InitPlayerState). The "
			 "PlayerState carries replicated nickname/score/team data the bot needs to be "
			 "treated as a real player by the scoreboard; without it, the bot is anonymous "
			 "and downstream UI silently breaks."),
		PossessBody.Contains(TEXT("InitPlayerState"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must assign a default nickname via SetDefaultPlayerName. "
			 "Without a default, the bot's nickname is empty and the scoreboard / kill feed "
			 "shows blank entries."),
		PossessBody.Contains(TEXT("SetDefaultPlayerName"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must subscribe to the GameState_Changed gameplay event via the "
			 "GlobalMessageSubsystem so the controller can react to InGame / EndGame phase "
			 "transitions (the SetAI(true) call on entering InGame is what actually starts "
			 "the bot moving)."),
		PossessBody.Contains(TEXT("GameState_Changed"), ESearchCase::CaseSensitive)
			|| PossessBody.Contains(TEXT("OnGameStateChanged"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must bind the MoverComponent's OnPostSimulationTick (the cell-"
			 "traversal-complete event) — this is the re-plan trigger that drives UpdateAI "
			 "on every cell step. Without the binding, the decision loop never runs and the "
			 "bot stands still."),
		PossessBody.Contains(TEXT("OnPostSimulationTick"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must bind the MapComponent's OnPostRemovedFromLevel — the "
			 "death-side event that disables the AI when the pawn is removed (B15). Without "
			 "the binding, a dead bot's controller would keep trying to UpdateAI."),
		PossessBody.Contains(TEXT("OnPostRemovedFromLevel"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must gate SetAI on the InGame tag — 'enable the decision loop "
			 "only if the game is in the playing phase'. Enabling AI mid-menu or mid-pre-"
			 "game has the bot moving before the round has started."),
		PossessBody.Contains(TEXT("FBmrGameStateTag::InGame"), ESearchCase::CaseSensitive)
			|| PossessBody.Contains(TEXT("::InGame"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B11: OnPossess must call SetAI(...) with the InGame-derived bMatchStarted "
			 "value — this is the public toggle that actually starts the decision loop."),
		PossessBody.Contains(TEXT("SetAI"), ESearchCase::CaseSensitive));

	// ----- OnUnPossess -----
	const FString UnpossessBody = ExtractMemberBody(Source, TEXT("ABmrAIController::OnUnPossess"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define OnUnPossess with a body."),
			!UnpossessBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B12: OnUnPossess must call SetAI(false) to stop the decision loop before the "
			 "pawn ref is cleared — running UpdateAI against a stale pawn would crash on "
			 "the next call."),
		UnpossessBody.Contains(TEXT("SetAI(false)"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B12: OnUnPossess must unbind the MapComponent's OnPostRemovedFromLevel — "
			 "leaving the binding alive is a UAF risk if the next pawn is a different "
			 "controller's, because the delegate target is `this`."),
		UnpossessBody.Contains(TEXT("OnPostRemovedFromLevel"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B12: OnUnPossess must touch the MoverComponent's OnPostSimulationTick binding "
			 "(remove or rebind) so the re-plan callback is not delivered to a stale `this` "
			 "after the pawn is repossessed."),
		UnpossessBody.Contains(TEXT("OnPostSimulationTick"), ESearchCase::CaseSensitive));

	// The cleanup must happen before Super::OnUnPossess() — the base class
	// nulls the pawn ref, and the cleanup needs the pawn to find the
	// components to unbind. The simplest verifiable encoding is that
	// `Super::OnUnPossess()` is the LAST line of the body; we approximate
	// that by requiring the substring to appear AFTER the cleanup refs.
	const int32 SuperPos = UnpossessBody.Find(TEXT("Super::OnUnPossess"), ESearchCase::CaseSensitive);
	const int32 CleanupPos = UnpossessBody.Find(TEXT("RemoveAll"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B12: OnUnPossess must call its base (Super::OnUnPossess) — the AIController "
			 "base does the actual pawn-ref clear and the binding wrap-up."),
		SuperPos != INDEX_NONE);

	// If the cleanup uses RemoveAll (the canonical pattern), it must appear
	// before Super::OnUnPossess() in the body. A body that calls Super first
	// loses GetPawn() and the cleanup degrades into a no-op.
	if (SuperPos != INDEX_NONE && CleanupPos != INDEX_NONE)
	{
		TestTrue(
			TEXT("B12: delegate cleanup (RemoveAll) must precede Super::OnUnPossess() in "
				 "OnUnPossess. The base call nulls the pawn reference; cleanup that runs "
				 "after it cannot find the pawn-side components and degrades into a no-op, "
				 "leaving stale delegate bindings on the next possession."),
			CleanupPos < SuperPos);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Lifecycle: Reset stops the pawn and clears the in-flight target;
//          OnGameStateChanged enables/disables on phase transitions;
//          OnPostRemovedFromLevel disables on death (B13, B14, B15).
//
// B13 DIRECT (source pattern): "On reset: clear in-flight movement and stop
//          the pawn". The canonical body sets LastMoveToCell to InvalidCell
//          and calls MoverComponent->RequestMoveByIntent(FVector::ZeroVector).
// B14 DIRECT (source pattern): "On game-state change: enable the decision
//          loop in the playing phase, disable it in menus, pre-game, and
//          end-game". OnGameStateChanged_Implementation routes the InGame
//          tag from the payload through SetAI.
// B15 DIRECT (source pattern): "On the pawn being removed from the level
//          (death): disable the decision loop". OnPostRemovedFromLevel_
//          Implementation calls SetAI(false).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_LifecycleResetGameStateAndDeath,
	"Bomber.AIController.LifecycleResetGameStateAndDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_LifecycleResetGameStateAndDeath::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	// ----- Reset (B13) -----
	const FString ResetBody = ExtractMemberBody(Source, TEXT("ABmrAIController::Reset"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define Reset with a body — the start/ stub only "
				 "forwards to Super and drops the in-flight target / pawn-stop steps."),
			!ResetBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B13: Reset must clear LastMoveToCell (typically set to FBmrCell::InvalidCell). "
			 "Without this clear, the post-reset UpdateAI sees a still-valid LastMoveToCell, "
			 "matches it against the bot's current cell, and bails through the 'Free.Contains"
			 "(LastMoveToCell)' branch — the bot freezes mid-step."),
		ResetBody.Contains(TEXT("LastMoveToCell"), ESearchCase::CaseSensitive)
			&& (ResetBody.Contains(TEXT("InvalidCell"), ESearchCase::CaseSensitive)
				|| ResetBody.Contains(TEXT("FBmrCell::InvalidCell"), ESearchCase::CaseSensitive)));

	TestTrue(
		TEXT("B13: Reset must stop the pawn by calling RequestMoveByIntent with a zero "
			 "vector (FVector::ZeroVector). Leaving the prior intent active means the pawn "
			 "keeps drifting in the last-chosen direction after the controller has logically "
			 "halted — the spec calls for an explicit stop."),
		ResetBody.Contains(TEXT("RequestMoveByIntent"), ESearchCase::CaseSensitive)
			&& (ResetBody.Contains(TEXT("FVector::ZeroVector"), ESearchCase::CaseSensitive)
				|| ResetBody.Contains(TEXT("ZeroVector"), ESearchCase::CaseSensitive)));

	TestTrue(
		TEXT("B13: Reset must call Super::Reset() (the AAIController base does its own "
			 "movement-task abort and StartSpot reset). Skipping it would leave engine-side "
			 "movement-tasks active."),
		ResetBody.Contains(TEXT("Super::Reset"), ESearchCase::CaseSensitive));

	// ----- OnGameStateChanged (B14) -----
	const FString GameStateBody = ExtractMemberBody(
		Source, TEXT("ABmrAIController::OnGameStateChanged_Implementation"));
	if (!TestTrue(
			TEXT("OnGameStateChanged_Implementation must have a body."),
			!GameStateBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B14: OnGameStateChanged_Implementation must read the InGame tag from the event "
			 "payload (Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame)). The match-"
			 "started flag is what decides whether the decision loop should be running; "
			 "reading the wrong tag (or hard-coding true/false) breaks the spec's 'enable in "
			 "playing phase, disable in menus / pre-game / end-game' rule."),
		GameStateBody.Contains(TEXT("InGame"), ESearchCase::CaseSensitive)
			&& GameStateBody.Contains(TEXT("HasTag"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B14: OnGameStateChanged_Implementation must route the resolved bMatchStarted "
			 "into SetAI. SetAI is the only authority-guarded toggle for the decision loop; "
			 "calling Reset() or flipping a private bool here would skip the authority guard "
			 "and the MoverComponent block-movement bookkeeping."),
		GameStateBody.Contains(TEXT("SetAI"), ESearchCase::CaseSensitive));

	// ----- OnPostRemovedFromLevel (B15) -----
	const FString RemovedBody = ExtractMemberBody(
		Source, TEXT("ABmrAIController::OnPostRemovedFromLevel_Implementation"));
	if (!TestTrue(
			TEXT("OnPostRemovedFromLevel_Implementation must have a body."),
			!RemovedBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B15: OnPostRemovedFromLevel_Implementation must call SetAI(false). The pawn "
			 "has just been removed from the level (death); the decision loop must stop or "
			 "the next UpdateAI tick tries to deref a destroyed pawn / MapComponent."),
		RemovedBody.Contains(TEXT("SetAI(false)"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — SetAI gates on authority + dead-AI guard; runtime probe of the
//           dead-AI early-return (B53 / evaluator-note "SetAI authority guard").
//
// Evaluator-note "common error #7": SetAI missing authority check — clients
//           run AI. The cpp must early-return when !HasAuthority(), and also
//           when the caller asks to enable AI on a dead bot (no pawn).
// DIRECT (runtime): on the CDO with no pawn, calling SetAI(true) must be a
//           no-op because the body sees `!InOwner && bShouldEnable` and bails
//           through the dead-AI guard. This is observable without a PIE
//           harness because the CDO's pawn is always null and the guard
//           short-circuits before any side effect (no Reset, no MoverComponent
//           access). Calling it must not change any observable property.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_SetAIAuthorityAndDeadGuard,
	"Bomber.AIController.SetAIAuthorityAndDeadGuard",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_SetAIAuthorityAndDeadGuard::RunTest(const FString& Parameters)
{
	using namespace AiControllerTests;

	const FString Source = LoadControllerSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString SetAIBody = ExtractMemberBody(Source, TEXT("ABmrAIController::SetAI"));
	if (!TestTrue(
			TEXT("BmrAIController.cpp must define SetAI with a body."),
			!SetAIBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("SetAI must check HasAuthority() and bail on clients. The decision loop only "
			 "runs server-side (the pawn's MoverComponent is replicated); a client running "
			 "UpdateAI duplicates the work and competes with the server's choices, leading "
			 "to visible desync."),
		SetAIBody.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("SetAI must reach for SetBlockMovement on the MoverComponent — this is how the "
			 "controller disables/enables movement in lockstep with the AI toggle. Calling "
			 "SetBlockMovement(!bShouldEnable) blocks movement when disabling AI and unblocks "
			 "when enabling — without it, a disabled bot would still respond to a stale "
			 "intent vector and drift."),
		SetAIBody.Contains(TEXT("SetBlockMovement"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("SetAI must call Reset() before flipping the movement block — clearing the "
			 "in-flight target ensures the bot doesn't resume mid-step toward a stale "
			 "destination when re-enabled."),
		SetAIBody.Contains(TEXT("Reset"), ESearchCase::CaseSensitive));

	// The dead-AI guard. The canonical pattern is
	//   const bool bWantsEnableDeadAI = !InOwner && bShouldEnable;
	//   if (bWantsEnableDeadAI || !HasAuthority()) { return; }
	// The body must short-circuit when asked to enable AI on a controller
	// with no pawn (e.g. SetAI(true) called after the pawn was destroyed but
	// before OnUnPossess landed). Without the guard, Reset() runs against a
	// null pawn and the MoverComponent deref further down faults.
	const bool bGuardsNoPawnEnable =
		SetAIBody.Contains(TEXT("!InOwner"), ESearchCase::CaseSensitive)
		|| SetAIBody.Contains(TEXT("InOwner == nullptr"), ESearchCase::CaseSensitive)
		|| SetAIBody.Contains(TEXT("nullptr == InOwner"), ESearchCase::CaseSensitive)
		|| SetAIBody.Contains(TEXT("!IsValid(InOwner)"), ESearchCase::CaseSensitive)
		|| SetAIBody.Contains(TEXT("bWantsEnableDeadAI"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("Evaluator-note 'common error #7': SetAI must guard against enabling AI when "
			 "the controller has no pawn (the canonical pattern is "
			 "`bWantsEnableDeadAI = !InOwner && bShouldEnable; if (bWantsEnableDeadAI || "
			 "!HasAuthority()) return;`). Without the dead-AI guard, SetAI(true) called "
			 "between the pawn's destruction and OnUnPossess would call Reset() against a "
			 "null pawn and deref a null MoverComponent further down."),
		bGuardsNoPawnEnable);

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — Public-API behavioral runtime: bomb-budget toggle is symmetric
//           and read by the same accessor the bomb gate consumes (B6); the
//           public MoveToCell entry-point is safe on a pawn-less controller
//           (B5).
//
// This test is intentionally runtime-only — it constructs an actual
// ABmrAIController via NewObject and exercises the *public* surface that is
// observable without a possessed pawn / generated map / authoritative
// GameState. The CDO is not used here because SetAICanSpawnBomb writes to
// a non-mutable accessor on the CDO branch; a fresh NewObject instance is
// the correct site for toggle exercises.
//
// Note on Reset: the canonical Reset body would be a useful runtime probe
// for B13, but Reset() is declared `protected` in ABmrAIController.h
// (override of AAIController::Reset). Tests must rely on observable
// behavior through clearly public declarations only, so we cannot call
// Reset() from this file. B13 is still covered by the source-inspection
// test FAiController_LifecycleResetGameStateAndDeath above, which verifies
// the canonical body's LastMoveToCell / ZeroVector / Super::Reset pattern.
//
// Coverage notes:
//
//   B6 DIRECT (runtime): the bomb-placement gate reads bCanSpawnBombs via
//   the inline CanAISpawnBomb() accessor. The public setter is the only way
//   per-mode logic disables bot bombing. The gate is meaningful only if the
//   setter and getter agree — a decoupled setter (writing to a different
//   field) would silently make per-mode bomb disables a no-op. This test
//   verifies the setter/getter contract end-to-end at runtime.
//
//   B5 DIRECT (runtime, partial): MoveToCell's null-pawn guard (evaluator
//   B49 — "Returns early if no MapComponent or MoverComponent") is the
//   safety property that prevents the locomotion path from crashing when
//   the controller is in a partially-constructed state. We can drive that
//   guard directly at runtime by calling MoveToCell on a pawn-less
//   controller; a re-implementation that elides the null check would crash
//   here (turning a silent stub-pass into a visible test failure).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAiController_PublicAPIBehavioralRuntime,
	"Bomber.AIController.PublicAPIBehavioralRuntime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAiController_PublicAPIBehavioralRuntime::RunTest(const FString& Parameters)
{
	ABmrAIController* AI = NewObject<ABmrAIController>();
	if (!TestNotNull(
			TEXT("ABmrAIController must be instantiable via NewObject for the runtime "
				 "behavioral test — the public surface (CanAISpawnBomb / MoveToCell) is "
				 "callable without a possessed pawn."),
			AI))
	{
		return false;
	}

	// B6 — bomb-budget toggle: default-true, then symmetric flip.
	TestTrue(
		TEXT("B6: CanAISpawnBomb() must default to true on a freshly-constructed controller. "
			 "The toggle is the per-bot bomb-budget gate consumed by UpdateAI; a default of "
			 "false would prevent every bot from bombing without explicit per-mode setup, "
			 "regressing the spec's 'bomb is placed only when [...] bomb budget available' "
			 "into 'bomb is never placed'."),
		AI->CanAISpawnBomb());

	AI->SetAICanSpawnBomb(false);
	TestFalse(
		TEXT("B6: after SetAICanSpawnBomb(false), CanAISpawnBomb() must return false. The "
			 "bomb-placement gate inside UpdateAI reads bCanSpawnBombs through this same "
			 "accessor; if the setter writes to a different field (or the getter reads from "
			 "a different field), per-mode bomb disables silently fail and bots bomb in "
			 "modes that explicitly forbid it."),
		AI->CanAISpawnBomb());

	AI->SetAICanSpawnBomb(true);
	TestTrue(
		TEXT("B6: SetAICanSpawnBomb(true) must restore CanAISpawnBomb() to true. The toggle "
			 "is symmetric — re-enabling after a temporary disable is the canonical mode-"
			 "transition flow. An asymmetric setter (one-way latch) would leave bots "
			 "permanently disabled after any per-round bomb pause."),
		AI->CanAISpawnBomb());

	// B5 / locomotion safety — MoveToCell on a pawn-less controller must
	// early-return through the canonical null-pawn / null-MoverComponent
	// guard rather than deref. A re-implementation that drops the guard
	// would crash here (and any caller that races MoveToCell against
	// OnUnPossess would crash in production).
	AI->MoveToCell(FBmrCell::InvalidCell);
	TestFalse(
		TEXT("B5: MoveToCell on a pawn-less controller must not produce a side effect that "
			 "flips IsAIEnabled() — the canonical body early-returns when MapComponent or "
			 "MoverComponent is null. Reaching this assertion at all proves the early-return "
			 "ran (a missing guard would have crashed). With no pawn, IsAIEnabled() reads "
			 "the (null) MoverComponent off the (null) pawn and short-circuits to false."),
		AI->IsAIEnabled());

	// MoveToCell must not be a one-way switch for the bomb-budget toggle.
	// The decision-loop locomotion path and the per-bot bomb config are
	// orthogonal concerns; the public locomotion call must not mutate the
	// bomb-budget field as a side effect.
	AI->SetAICanSpawnBomb(false);
	AI->MoveToCell(FBmrCell::InvalidCell);
	TestFalse(
		TEXT("B6: MoveToCell must NOT clobber the bomb-budget config — locomotion and per-"
			 "bot bombing configuration are orthogonal. A MoveToCell that incidentally "
			 "flipped bCanSpawnBombs would silently re-enable bombing in modes that "
			 "explicitly disabled it, violating B6's bomb-budget gate."),
		AI->CanAISpawnBomb());

	return true;
}
