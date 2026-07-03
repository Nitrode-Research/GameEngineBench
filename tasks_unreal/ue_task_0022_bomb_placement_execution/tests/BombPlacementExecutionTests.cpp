// Copyright (c) 2026 GameDevBench. Bomb placement + explosion-execution automation tests.
//
// Tests target two tightly-coupled units from the spec:
//   * UBmrBombPlaceAbility — the placement-side gate, cost mechanism, and the
//     write of the origin cell into the fuse effect's context.
//   * UBmrExplosionExecution — the detonation-time damage capture, the cell-
//     based explosion path (not engine radial damage), the authority + game-
//     state guard, the box-destroy path, and the chain reaction driven by
//     removing the *placer's* fuse effect (not the bomb actor's own ASC).
//
// Why mostly inspection-based coverage:
//   The placement → fuse → execution pipeline is end-to-end GAS:
//   GameplayEvent → ability gate → CommitAbility → durational fuse GE →
//   GE expiry runs the custom execution. Reproducing that pipeline in an
//   automation test requires a full pawn with an ASC that has the place
//   ability granted, the bomb data asset hydrated, the cost/duration GE
//   classes wired, the generated map populated, and (for chain reactions)
//   two pawns whose simultaneous bombs land on the same tick. That setup
//   does not exist as a reusable test harness in this project and inlining
//   it would dwarf the surface under test.
//
//   The spec and the evaluator notes both identify a small set of *code-
//   path* choices that separate a working impl from a plausibly-working one:
//     * gate-vs-activation placement validation (B1, B2, B4, B18)
//     * Super::ApplyCost vs ApplyDurationalEffect for cost holding (B3, B17)
//     * EffectContext.AddOrigin for the fuse→execution handoff (B14)
//     * cell-graph (GetCellsAround / EPathType::Explosion) vs engine radial
//       damage (B5, B8, B13)
//     * RemoveActiveEffects on the placer's ASC, not the bomb's own ASC,
//       and no "detonate" call (B10, B11, B12, B15, B16)
//     * DestroyLevelActor for boxes vs damage GE (B7)
//     * authority + InGame/EndGame guard (B9)
//   These are the discriminators the spec calls out as the failure-mode
//   surface — once they are right, the runtime behavior follows. Source
//   inspection of the two stripped .cpp files is the highest-signal way
//   to check them without re-implementing the GAS pipeline.
//
//   Two runtime checks ARE feasible without any of that scaffolding, and are
//   the highest-leverage observations listed in the evaluator's "test design
//   guidance":
//     1) The damage-capture declaration is static on the execution class —
//        the CDO publishes RelevantAttributesToCapture through the
//        GetAttributeCaptureDefinitions() accessor on UGameplayEffectCalculation.
//        That check (B19 / evaluator B15–B16) does not depend on a fuse, a
//        fake pawn, or a populated map.
//     2) ShouldAbilityRespondToEvent is callable on the CDO directly through
//        the public UGameplayAbility base pointer (the override is protected,
//        but virtual access checks are static-type-based). Driving it with an
//        invalid event magnitude observes the *runtime* refusal that the spec
//        requires — the stripped stub forwards to Super (returns true), the
//        solution returns false. That check covers B1/B2/B4/B18 directly at
//        runtime without any GAS/PIE plumbing.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"

// Bomber
#include "AbilitySystem/Abilities/BmrBombPlaceAbility.h"
#include "AbilitySystem/Attributes/BmrHealthAttributeSet.h"
#include "AbilitySystem/Executions/BmrExplosionExecution.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectAttributeCaptureDefinition.h"
#include "GameplayEffectCalculation.h"
#include "GameplayEffectTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogBombPlacementExecutionTests, Log, All);

namespace BombPlacementExecutionTests
{
	// ---------------------------------------------------------------------
	// Source-file readers. The two stripped files in this task are:
	//   * Source/Bomber/Private/AbilitySystem/Abilities/BmrBombPlaceAbility.cpp
	//   * Source/Bomber/Private/AbilitySystem/Executions/BmrExplosionExecution.cpp
	// They are loaded once per test and scanned for the specific patterns
	// that the spec / failure-modes section pin down.
	// ---------------------------------------------------------------------
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

	static FString LoadAbilitySource(FAutomationTestBase* Test)
	{
		return LoadProjectFile(Test, TEXT("Source/Bomber/Private/AbilitySystem/Abilities/BmrBombPlaceAbility.cpp"));
	}

	static FString LoadExecutionSource(FAutomationTestBase* Test)
	{
		return LoadProjectFile(Test, TEXT("Source/Bomber/Private/AbilitySystem/Executions/BmrExplosionExecution.cpp"));
	}

	// Brace-depth walker to extract a member function body. Mirrors the
	// pattern used by BombAbilityActorTests / GeneratedMapOrchestratorTests.
	// Returns the substring from the opening { through the matching close }.
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
} // namespace BombPlacementExecutionTests

// ---------------------------------------------------------------------------
// Test 1 — UBmrExplosionExecution declares its OutcomingDamage capture.
//
// B19 DIRECT (also covers evaluator B15–B17): the execution constructor must
// register a capture for UBmrHealthAttributeSet::OutcomingDamage from the
// effect Source. Without this declaration, AttemptCalculateCapturedAttributeMagnitude
// reads 0 for the damage and every explosion silently does nothing. The
// declaration is observable on the class CDO via the public accessor
// UGameplayEffectCalculation::GetAttributeCaptureDefinitions(), so this is
// the one piece of execution behavior that is testable at runtime without
// a fuse, a placer, or a populated map.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_ExplosionDamageCaptureDeclared,
	"Bomber.BombPlacement.ExplosionDamageCaptureDeclared",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_ExplosionDamageCaptureDeclared::RunTest(const FString& Parameters)
{
	const UClass* ExecClass = UBmrExplosionExecution::StaticClass();
	if (!TestNotNull(TEXT("UBmrExplosionExecution class must exist"), ExecClass))
	{
		return false;
	}

	const UBmrExplosionExecution* CDO = Cast<UBmrExplosionExecution>(ExecClass->GetDefaultObject());
	if (!TestNotNull(TEXT("UBmrExplosionExecution CDO must be obtainable"), CDO))
	{
		return false;
	}

	const TArray<FGameplayEffectAttributeCaptureDefinition>& CaptureDefs = CDO->GetAttributeCaptureDefinitions();

	if (!TestTrue(
			TEXT("B19: UBmrExplosionExecution's CDO must publish at least one capture definition. "
				 "The constructor must call RelevantAttributesToCapture.Add(...) for the "
				 "OutcomingDamage attribute. Without any capture, "
				 "AttemptCalculateCapturedAttributeMagnitude reads 0 and every explosion "
				 "silently does zero damage — tests that only check 'bomb visually exploded' miss this."),
			CaptureDefs.Num() > 0))
	{
		return false;
	}

	const FGameplayAttribute OutcomingAttr = UBmrHealthAttributeSet::GetOutcomingDamageAttribute();

	bool bFoundOutcoming = false;
	for (const FGameplayEffectAttributeCaptureDefinition& Def : CaptureDefs)
	{
		if (Def.AttributeToCapture == OutcomingAttr)
		{
			bFoundOutcoming = true;
			TestEqual(
				TEXT("B19: The OutcomingDamage capture must come from the effect Source (the placer). "
					 "Capturing from Target would read the victim's outgoing damage, not the placer's — "
					 "the placer's powerup/buff modifiers would never apply."),
				Def.AttributeSource, EGameplayEffectAttributeCaptureSource::Source);
			break;
		}
	}

	TestTrue(
		TEXT("B19: RelevantAttributesToCapture must include UBmrHealthAttributeSet::OutcomingDamage. "
			 "This is the only attribute the execution reads to size the explosion damage; missing "
			 "it makes the entire damage pipeline a silent no-op."),
		bFoundOutcoming);

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — Placement gate lives in ShouldAbilityRespondToEvent (pre-activation),
//          not in ActivateAbility.
//
// B1 DIRECT: ShouldAbilityRespondToEvent must read the target cell from the
//            event's EventMagnitude (the cell index passed by the input
//            pipeline). Without this, the ability has no idea where the
//            placer was when they pressed place.
// B2 DIRECT: the gate must reject cells already occupied by a non-player
//            actor (bomb / box / wall). The canonical implementation is
//            IsCellHasAnyMatchingActor with TO_FLAG(~EAT::Player) (or an
//            equivalent mask that excludes players).
// B4 DIRECT / B18 DIRECT: the gate is the *pre-activation* check — two
//            same-tick placements both pass a check that lives inside
//            ActivateAbility, because neither bomb has spawned yet at
//            gate-evaluation time. Validating the cell inside the activation
//            body therefore allows the simultaneous-placement race the spec
//            calls out. ActivateAbility must NOT re-validate cell occupancy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_PlacementGateIsPreActivation,
	"Bomber.BombPlacement.PlacementGateIsPreActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_PlacementGateIsPreActivation::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadAbilitySource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	// ----- ShouldAbilityRespondToEvent must be defined with a non-trivial body. -----
	const FString GateBody = ExtractMemberBody(
		Source, TEXT("UBmrBombPlaceAbility::ShouldAbilityRespondToEvent"));
	if (!TestTrue(
			TEXT("B1/B2: BmrBombPlaceAbility.cpp must define ShouldAbilityRespondToEvent with a body. "
				 "The start/ stub just forwards to Super; the gate logic is what was stripped."),
			!GateBody.IsEmpty()))
	{
		return false;
	}

	// B1: must read the cell from TriggerEventData->EventMagnitude.
	const bool bReadsEventMagnitude =
		GateBody.Contains(TEXT("EventMagnitude"), ESearchCase::CaseSensitive) &&
		(GateBody.Contains(TEXT("GetCellByIndexOnLevel"), ESearchCase::CaseSensitive)
			|| GateBody.Contains(TEXT("CellByIndex"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B1: ShouldAbilityRespondToEvent must derive the target cell from "
			 "TriggerEventData->EventMagnitude (typically via "
			 "UBmrCellUtilsLibrary::GetCellByIndexOnLevel(EventMagnitude)). The placement "
			 "input pipeline encodes the placer's current cell as the event magnitude — "
			 "reaching into the pawn directly to read world position is a spec failure mode."),
		bReadsEventMagnitude);

	// B2: must reject occupied cells using the cell-occupancy helper with the
	// non-player mask. The TO_FLAG(~EAT::Player) idiom (or any expression that
	// evaluates to "everything except Player") is what makes players (incl. the
	// placer) not block placement.
	const bool bUsesOccupancyCheck = GateBody.Contains(TEXT("IsCellHasAnyMatchingActor"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B2: ShouldAbilityRespondToEvent must reject occupied cells via "
			 "UBmrCellUtilsLibrary::IsCellHasAnyMatchingActor. Without an occupancy check at "
			 "the gate, simultaneous placements on the same cell both succeed."),
		bUsesOccupancyCheck);

	const bool bExcludesPlayer =
		GateBody.Contains(TEXT("~EAT::Player"), ESearchCase::CaseSensitive)
		|| GateBody.Contains(TEXT("~EBmrActorType::Player"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B2: the occupancy mask must exclude EAT::Player (e.g. TO_FLAG(~EAT::Player)). "
			 "Players standing on a cell — including the placer — must not block placement. "
			 "A mask that includes Player rejects every placement attempt because the placer "
			 "is always on the cell they are trying to place on."),
		bExcludesPlayer);

	// ----- ActivateAbility must NOT re-run the occupancy check (B4 / B18). -----
	const FString ActivateBody = ExtractMemberBody(
		Source, TEXT("UBmrBombPlaceAbility::ActivateAbility"));
	if (!TestTrue(
			TEXT("BmrBombPlaceAbility.cpp must define ActivateAbility with a body."),
			!ActivateBody.IsEmpty()))
	{
		return false;
	}

	TestFalse(
		TEXT("B4/B18: ActivateAbility must NOT call IsCellHasAnyMatchingActor — the cell-occupancy "
			 "check belongs in ShouldAbilityRespondToEvent (the pre-activation gate). Validating the "
			 "target cell inside the activation body lets two simultaneous placements both succeed: "
			 "neither bomb has spawned yet at activation time, so both occupancy checks see an empty "
			 "cell. The spec calls this out as an explicit failure mode."),
		ActivateBody.Contains(TEXT("IsCellHasAnyMatchingActor"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — ActivateAbility writes the origin cell into the fuse effect context.
//
// B14 DIRECT: the only piece of data carried from placement to execution is
//             the origin cell, stored in the FGameplayEffectContextHandle via
//             EffectContext.AddOrigin(SpawnCell). The execution reads it back
//             out from Spec.GetEffectContext().GetOrigin(). If ActivateAbility
//             does not write the origin, the execution's HasOrigin() assert
//             fires and the bomb silently does nothing on detonation.
//
//             The spec also flags the inverse failure (writing a precomputed
//             cell set into the context) as wrong: the context carries the
//             origin *only* — the affected-cell set must be computed at
//             detonation time from the current map state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_OriginWrittenToFuseContext,
	"Bomber.BombPlacement.OriginWrittenToFuseContext",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_OriginWrittenToFuseContext::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadAbilitySource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ActivateBody = ExtractMemberBody(
		Source, TEXT("UBmrBombPlaceAbility::ActivateAbility"));
	if (!TestTrue(
			TEXT("BmrBombPlaceAbility.cpp must define ActivateAbility with a body."),
			!ActivateBody.IsEmpty()))
	{
		return false;
	}

	const bool bWritesOrigin = ActivateBody.Contains(TEXT("AddOrigin"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B14: ActivateAbility must write the spawn cell into the fuse effect's context "
			 "via FGameplayEffectContextHandle::AddOrigin(SpawnCell). The execution reads the "
			 "origin back via Spec.GetEffectContext().GetOrigin() at detonation; without "
			 "AddOrigin, the execution's HasOrigin() guard fires and the bomb silently "
			 "does nothing when its fuse expires."),
		bWritesOrigin);

	// The duration GE itself must be applied via the durational helper, so the
	// fuse outlives the activation frame. Otherwise the fuse never ticks down.
	const bool bAppliesDurationalEffect =
		ActivateBody.Contains(TEXT("ApplyDurationalEffect"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B14: ActivateAbility must start the fuse via ApplyDurationalEffect with the "
			 "bomb duration gameplay effect (UBmrBombDataAsset::Get().GetDurationGameplayEffect()). "
			 "An instant GE would expire on the same frame and the explosion execution would "
			 "run before any chain bomb could be placed alongside it."),
		bAppliesDurationalEffect);

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — Cost is held for the fuse duration, not applied instantly.
//
// B3 DIRECT: the cost (one "Powerup_BombsAvailable" charge) must persist for
//            the full fuse duration. The mechanism is: ApplyCost applies the
//            cost GE *as a duration effect*, so the charge is released only
//            when the bomb detonates. Calling Super::ApplyCost applies the
//            cost as an *instant* GE, releasing the charge immediately — the
//            placer can place a second bomb on the very next tick.
// B17 DIRECT: the cap is enforced by the cost duration alone. No cooldown GE
//             is involved. Calling Super::ApplyCooldown / referencing
//             cooldown timing as the cap is the spec's "active bombs counted
//             via cooldown" failure mode.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_CostHeldForFuseDuration,
	"Bomber.BombPlacement.CostHeldForFuseDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_CostHeldForFuseDuration::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadAbilitySource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ApplyCostBody = ExtractMemberBody(
		Source, TEXT("UBmrBombPlaceAbility::ApplyCost"));
	if (!TestTrue(
			TEXT("BmrBombPlaceAbility.cpp must define ApplyCost with a body. The start/ stub "
				 "is empty; the cost-as-duration logic is what was stripped."),
			!ApplyCostBody.IsEmpty()))
	{
		return false;
	}

	// B3: the body must call the durational helper rather than Super::ApplyCost.
	// Super::ApplyCost applies the cost GE as an instant effect.
	TestFalse(
		TEXT("B3: ApplyCost must NOT call Super::ApplyCost. Super applies the cost as an "
			 "*instant* gameplay effect — the placer's BombsAvailable charge is consumed and "
			 "immediately released, so they can place again on the very next tick. The fuse-"
			 "duration cap relies on holding the cost effect for the bomb's full lifetime."),
		ApplyCostBody.Contains(TEXT("Super::ApplyCost"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: ApplyCost must apply the cost via ApplyDurationalEffect with the cost GE "
			 "class (GetCostGameplayEffect()->GetClass()). The durational application is what "
			 "keeps the charge unavailable until the bomb detonates."),
		ApplyCostBody.Contains(TEXT("ApplyDurationalEffect"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: ApplyCost must reference GetCostGameplayEffect (the source of the cost GE "
			 "class). Hard-coding a different GE class would break configurability and divorce "
			 "the cost from the ability's declared CostGameplayEffectClass."),
		ApplyCostBody.Contains(TEXT("GetCostGameplayEffect"), ESearchCase::CaseSensitive));

	// B17: no cooldown should appear anywhere in the ability cpp — the cap is the
	// cost duration. ApplyCooldown is not overridden and there is no explicit cooldown GE.
	TestFalse(
		TEXT("B17: BmrBombPlaceAbility.cpp must NOT reference ApplyCooldown / a cooldown GE. "
			 "The spec is explicit: 'How many of my bombs can be in play at once' is enforced "
			 "by holding the placement cost for the fuse duration, NOT by a cooldown. Counting "
			 "active bombs via cooldown timing is a spec failure mode."),
		Source.Contains(TEXT("ApplyCooldown"), ESearchCase::CaseSensitive)
			|| Source.Contains(TEXT("GetCooldownGameplayEffect"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Execution gates on authority + the in-game / end-game phase.
//
// B9 DIRECT: the explosion execution may only run on the server, and only
//            during the InGame or EndGame phases (an end-of-round explosion
//            that is still resolving is allowed). Running on clients
//            duplicates damage; running outside InGame/EndGame (e.g. in the
//            countdown or post-match menu) damages pawns who shouldn't be
//            taking damage.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_ExecutionAuthorityAndPhaseGate,
	"Bomber.BombPlacement.ExecutionAuthorityAndPhaseGate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_ExecutionAuthorityAndPhaseGate::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadExecutionSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ExecBody = ExtractMemberBody(
		Source, TEXT("UBmrExplosionExecution::Execute_Implementation"));
	if (!TestTrue(
			TEXT("BmrExplosionExecution.cpp must define Execute_Implementation with a body. "
				 "The start/ stub is empty; the entire damage pipeline is what was stripped."),
			!ExecBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B9: Execute_Implementation must reference HasAuthority — the execution is "
			 "authority-only. Running on clients duplicates damage and racks up RemoveActiveEffects "
			 "calls on local-only ASCs, desynchronising chain reactions."),
		ExecBody.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive));

	const bool bChecksGameState =
		ExecBody.Contains(TEXT("InGame"), ESearchCase::CaseSensitive)
		&& ExecBody.Contains(TEXT("EndGame"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B9: Execute_Implementation must gate on BOTH FBmrGameStateTag::InGame AND "
			 "FBmrGameStateTag::EndGame. The end-of-round-but-still-resolving explosion is "
			 "explicitly allowed; gating only on InGame swallows the last-second detonations "
			 "that decide draws. Gating only on EndGame is even worse — no explosions during "
			 "the main match phase."),
		bChecksGameState);

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Explosion is cell-based, not engine radial damage.
//
// B5 DIRECT: the affected-cell set is computed via the cell-graph utility
//            (UBmrCellUtilsLibrary::GetCellsAround) with EPathType::Explosion.
//            This is what limits each arm to the bomb's Fire radius and
//            stops arms at walls / boxes.
// B8 DIRECT: a wall (or any cell-blocking actor) at distance d < range
//            truncates that arm — cells beyond the wall on that arm receive
//            no damage even if they would otherwise be within range.
//            EPathType::Explosion is the pathfinder that implements this.
// B13 DIRECT: implementing the explosion with engine-stock radial damage
//             (UGameplayStatics::ApplyRadialDamage / ApplyPointDamage /
//             a sphere overlap) looks right inside open play areas but
//             fails the wall-truncation case (radial damage passes through
//             walls) and the cell-boundary case (pawns 'near but not on'
//             a cell). The execution must NOT use any of these helpers.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_CellBasedExplosionNotRadial,
	"Bomber.BombPlacement.CellBasedExplosionNotRadial",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_CellBasedExplosionNotRadial::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadExecutionSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ExecBody = ExtractMemberBody(
		Source, TEXT("UBmrExplosionExecution::Execute_Implementation"));
	if (!TestTrue(
			TEXT("BmrExplosionExecution.cpp must define Execute_Implementation with a body."),
			!ExecBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B5/B8: Execute_Implementation must compute affected cells via "
			 "UBmrCellUtilsLibrary::GetCellsAround. This is the only cell-graph traversal "
			 "that respects per-arm truncation at walls / boxes."),
		ExecBody.Contains(TEXT("GetCellsAround"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B5/B8: GetCellsAround must be invoked with EPathType::Explosion. This is the "
			 "pathfinder variant that breaks each arm at the first wall/box hit; using "
			 "EPathType::Any or EPathType::Free either ignores walls or stops at the wrong "
			 "obstacles. The cell-boundary semantics of the spec require the explosion variant."),
		ExecBody.Contains(TEXT("EPathType::Explosion"), ESearchCase::CaseSensitive));

	// B13: must NOT use engine radial / point-damage helpers. These are the
	// failure mode flagged by the spec; they "look right inside open play areas"
	// but fail wall truncation and cell-boundary cases. The check is on the full
	// file because no legitimate code path inside the execution should reach for
	// these helpers.
	const TCHAR* const ForbiddenRadialApis[] =
	{
		TEXT("ApplyRadialDamage"),
		TEXT("ApplyRadialDamageWithFalloff"),
		TEXT("ApplyPointDamage"),
		TEXT("UGameplayStatics::ApplyDamage"),
	};
	for (const TCHAR* Api : ForbiddenRadialApis)
	{
		TestFalse(
			FString::Printf(TEXT("B13: Execute_Implementation / BmrExplosionExecution.cpp must NOT "
								 "reference %s. Engine radial/point-damage helpers ignore the cell "
								 "graph: they pass through walls (failing the truncation case from B8) "
								 "and damage pawns 'near but not on' an affected cell. The cell-graph "
								 "path (GetCellsAround + per-cell actor lookup) is the only "
								 "implementation that matches the spec's semantics."),
				Api),
			Source.Contains(Api, ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Boxes are destroyed via the level orchestrator, not via a damage GE.
//
// B7 DIRECT: solid boxes on affected cells have no AbilitySystemComponent, so
//            applying a damage gameplay effect to them is a no-op. The
//            execution must call ABmrGeneratedMap::DestroyLevelActor (or the
//            UFUNCTION exposing the same path) for actors without an ASC.
//            Routing box destruction through a damage GE leaves boxes
//            standing forever while pawns die — a particularly nasty
//            "looks right in early playtests" failure mode because the
//            visual cue lines up but the cell stays blocked.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_BoxDestructionPath,
	"Bomber.BombPlacement.BoxDestructionPath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_BoxDestructionPath::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadExecutionSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ExecBody = ExtractMemberBody(
		Source, TEXT("UBmrExplosionExecution::Execute_Implementation"));
	if (!TestTrue(
			TEXT("BmrExplosionExecution.cpp must define Execute_Implementation with a body."),
			!ExecBody.IsEmpty()))
	{
		return false;
	}

	TestTrue(
		TEXT("B7: Execute_Implementation must call ABmrGeneratedMap::DestroyLevelActor (the "
			 "level orchestrator's destroy-on-cell path) for ASC-less actors like boxes. "
			 "Routing box destruction through a damage GE leaves boxes standing — they have "
			 "no AbilitySystemComponent and no HealthAttributeSet, so the GE silently does "
			 "nothing. Pawns die normally, boxes don't; the affected cell stays blocked and "
			 "the next round can never spawn powerups underneath."),
		ExecBody.Contains(TEXT("DestroyLevelActor"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Chain reactions are driven by removing the *placer's* fuse effect,
//          not by a 'detonate' call and not on the bomb actor's own ASC.
//
// B10 / B11 DIRECT (mechanism): when an explosion's affected cells contain
//            another bomb, the execution removes that bomb's fuse effect.
//            The fuse-effect removal is what triggers the chained bomb's
//            own execution from its own origin cell. Two bombs that expire
//            on the same tick each run their own execution; a same-tick
//            chain that arrives first clears the origin cell, and the
//            second execution's "no bomb at origin" check bails silently.
//
// B12 / B16 DIRECT (which ASC): the fuse effect was originally applied on
//            the *placer's* ability system component (where the cost / fuse
//            durational effect lives). The chain must remove it from the
//            placer's ASC — the bomb actor's own ASC does not host the fuse
//            effect, so calling RemoveActiveEffects there is a silent no-op
//            and the chained bomb continues its original fuse timer to
//            completion, breaking the spec's "two bombs that expire on the
//            exact same tick resolve cleanly" guarantee.
//
// B15 DIRECT (mechanism, inverse): no "detonate" function is invoked on a
//            chained bomb. The bomb actor itself doesn't expose a public
//            detonate / explode entrypoint that the execution could call —
//            chain reactions are driven entirely by fuse-effect removal.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBombPlacementExecution_ChainViaFuseRemovalOnPlacer,
	"Bomber.BombPlacement.ChainViaFuseRemovalOnPlacer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBombPlacementExecution_ChainViaFuseRemovalOnPlacer::RunTest(const FString& Parameters)
{
	using namespace BombPlacementExecutionTests;

	const FString Source = LoadExecutionSource(this);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ExecBody = ExtractMemberBody(
		Source, TEXT("UBmrExplosionExecution::Execute_Implementation"));
	if (!TestTrue(
			TEXT("BmrExplosionExecution.cpp must define Execute_Implementation with a body."),
			!ExecBody.IsEmpty()))
	{
		return false;
	}

	// B10/B11/B15: the mechanism for chaining is GAS-effect removal on the
	// placer's ASC, not a dedicated detonate call. The presence of any of the
	// GAS removal APIs (query-based RemoveActiveEffects, or handle-based
	// RemoveActiveGameplayEffect) is the positive signal; the absence of any
	// "Detonate" / "Explode" call on a chained bomb is the negative signal.
	const bool bHasFuseRemovalCall =
		ExecBody.Contains(TEXT("RemoveActiveEffects"), ESearchCase::CaseSensitive)
		|| ExecBody.Contains(TEXT("RemoveActiveGameplayEffect"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B10/B11/B15: Execute_Implementation must remove the fuse durational effect from "
			 "the placer's ASC to chain into neighbouring bombs — that removal is what triggers "
			 "the chained bomb's own execution path. Accepted forms: query-based "
			 "RemoveActiveEffects, or handle-based RemoveActiveGameplayEffect. There is no public "
			 "detonate function on the bomb actor; chain reactions are driven entirely by "
			 "fuse-effect removal."),
		bHasFuseRemovalCall);

	// B12/B16: the removal must target the *placer's* ASC, i.e. the bomb actor's
	// GetInstigatorAbilitySystemComponent() (the replicated placer reference set
	// up by ABmrBombAbilityActor::InitBomb). Removing from the bomb actor's own
	// ASC silently does nothing because the fuse was never applied there.
	TestTrue(
		TEXT("B12/B16: the chain must remove the fuse effect from the placer's ASC, accessed "
			 "via the bomb actor's GetInstigatorAbilitySystemComponent(). The fuse was applied "
			 "on the placer's ASC during ActivateAbility / ApplyDurationalEffect — removing it "
			 "on the bomb actor's own ASC is a no-op and the chained bomb continues its "
			 "original timer to completion, breaking the same-tick-chain guarantee from B11."),
		ExecBody.Contains(TEXT("GetInstigatorAbilitySystemComponent"), ESearchCase::CaseSensitive));

	// B15: there must be no "Detonate" / "Explode" call on a chained bomb actor.
	// The spec is explicit: "Calling a hypothetical 'detonate' function on chained
	// bombs. No such function is invoked".
	const TCHAR* const ForbiddenChainApis[] =
	{
		TEXT("Detonate("),
		TEXT("Explode("),
		TEXT("ForceDetonate("),
		TEXT("TriggerExplosion("),
	};
	for (const TCHAR* Api : ForbiddenChainApis)
	{
		TestFalse(
			FString::Printf(TEXT("B15: BmrExplosionExecution.cpp must NOT call %s. The spec is "
								 "explicit: chain reactions are driven by fuse-effect removal, not "
								 "by a hypothetical detonate function. A direct detonate call would "
								 "bypass the fuse-removal machinery (B12) and the cost-release path "
								 "(B3) that makes the placer's BombsAvailable replenish."),
				Api),
			Source.Contains(Api, ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Removed.
//
// Previously drove ShouldAbilityRespondToEvent on the CDO with a null
// TriggerEventData to observe the runtime refusal. The solution body's
// ensureMsgf("'SpawnCell' is not passed to event magnitude, ...") fires
// on that path and produces a stack-trace dump that the automation runner
// counts as a test error. AddExpectedError only matches the one sentinel
// string — the surrounding "ASSERT:" / "Stack:" lines slip through and the
// test fails despite the return value being correct.
//
// The invalid-cell path is the only payload shape that can be driven from
// outside without bootstrapping a generated map (GetCellByIndexOnLevel
// would otherwise need a real level), and that path is exactly the one
// guarded by the ensureMsgf — so there is no clean runtime probe of the
// gate from a test environment.
//
// B1 / B2 / B4 / B18 stay covered by Test 2 (source inspection of
// ShouldAbilityRespondToEvent / ActivateAbility).
// ---------------------------------------------------------------------------
