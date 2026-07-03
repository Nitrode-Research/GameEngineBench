// Copyright (c) 2026 GameDevBench. UBmrMoverComponent automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/Components/BmrMoverComponent.cpp
//
// UBmrMoverComponent is the project's UMoverComponent subclass — the
// Mover-2.0 bridge sitting on every ABmrPawn. It owns two per-tick
// contracts (ProduceInput → inject directional input; OnPostMove →
// write sync state and update the pawn's grid cell), a GAS-tag-driven
// movement-block, a cached Skate-attribute, a teleport path that must
// route through Mover's instant-movement-effect mechanism, and a
// BeginPlay binding fan-out across five separate event sources.
//
// Why source-inspection coverage:
//   Direct runtime coverage needs a populated ABmrGeneratedMap (the cell
//   utility library asserts on it inside SnapActorOnLevel /
//   GetLevelGridRotation, both via the singleton ABmrGeneratedMap::Get()),
//   a possessed ABmrPawn whose ASC has the BmrPlayerDataAsset's
//   BlockMovementEffect granted, the GlobalMessageSubsystem wired to
//   broadcast Event::Player_PawnReady and Event::GameState_Changed, and
//   the Mover-2.0 BackendLiaisonComp registered against a network driver
//   that can accept QueueInstantMovementEffect. None of that exists as a
//   reusable PIE harness in this project; reproducing it would dwarf the
//   surface under test and the stripped helpers (SnapActorOnLevel,
//   GetLevelGridRotation) would assert on the empty map before any test
//   could observe the component's response.
//
//   The spec and the evaluator notes pin down a small set of code-path
//   choices that separate a working impl from a plausibly-working one —
//   one per failure-mode bullet in the brief:
//     B1: IsBlockedMovement reads the GAS tag, not a bool field, so
//         external systems (cheat manager, AI controller, game state)
//         all converge on the same authoritative state.
//     B2: ProduceInput rotates the move input by the level grid's yaw
//         before injecting it, so WASD follows the rotated map.
//     B3: TeleportToLocation routes through QueueInstantMovementEffect
//         when BackendLiaisonComp is valid; SetActorLocation appears
//         only on the editor-preview fallback.
//     B4: OnPostMove's cell-update split is mutually exclusive
//         (HasAuthority → server snap; else-if IsLocallyControlled →
//         client snap), so the cell never gets two writes per tick.
//     B5: OnPawnReady guards the Skate-attribute delegate bind with
//         IsBoundToObject(this), so the pawn pool's reuse cycle
//         doesn't stack duplicate subscribers.
//     B6: OnPostMove reads CachedSkatePowerupAttribute, not the live
//         ASC attribute, so per-tick attribute queries don't enter
//         the GAS hot path.
//     B7: OnPostMove early-returns when IsBlockedMovement() is true,
//         so a dead pawn whose grid cell is being torn down doesn't
//         scribble a stale position back into the map.
//
//   Each test below targets one of these failure modes directly,
//   scanning the comment-stripped body of the relevant member function
//   so that anti-pattern descriptions written in canonical comments
//   ("Pass a do nothing input", "Might be null in editor preview",
//   "On server, update a player location on the Generated Map") cannot
//   satisfy the assertions. Additional tests cover the broader behavior
//   the spec headings call out (BeginPlay's full binding fan-out,
//   OnPreRemovedFromLevel's block trigger) so a re-impl that moves a
//   failure mode out of one function and into another still fails. A
//   final test reaches the component's UClass via engine reflection to
//   catch UFUNCTION / UPROPERTY rename regressions that source scans
//   alone wouldn't notice.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

// Bomber
#include "Components/BmrMoverComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMoverComponentTests, Log, All);

namespace MoverComponentTests
{
	static const TCHAR* ComponentCppRelPath =
		TEXT("Source/Bomber/Private/Components/BmrMoverComponent.cpp");

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
	// own comments include "Pass a do nothing input", "Might be null in editor
	// preview", "On server, update a player location on the Generated Map",
	// "On local client, directly set a player location" — every one of these
	// would pass a naive substring test even against a stub body. String
	// literals are preserved so log-call text and TEXT() macros still scan.
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

	// Brace-depth walker. Counts braces only, not parens, so nested
	// initialiser lists / lambdas (BeginPlay's CallOrStartListeningForGlobalMessage
	// passes a lambda capturing the owning pawn) don't confuse the scan.
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
} // namespace MoverComponentTests

// ---------------------------------------------------------------------------
// Test 1 — B1: IsBlockedMovement reads the GAS tag, NOT a bool field.
//
// The spec's named failure mode: "Boolean movement block — external tag
// adders diverge from the bool." Three independent systems toggle the
// movement block: the cheat manager (for testing), the AI controller (for
// stunning), and game-state transitions (Menu / GameStarting / EndGame).
// Each adds or removes the Block::Movement gameplay tag on the pawn's ASC.
// If IsBlockedMovement reads a bool member, a cheat-manager apply of the
// tag-bearing GE through the GAS surface bypasses the bool entirely; the
// component then disagrees with the tag store on whether movement is
// blocked and the pawn either keeps moving despite a granted block or
// freezes despite a removed one.
//
// The canonical encoding is a stateless query:
//   ASC->HasMatchingGameplayTag(BmrGameplayTags::GameplayEffect::Block::Movement)
// — the tag store is the single source of truth.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_IsBlockedMovementQueriesTag,
	"Bomber.MoverComponent.IsBlockedMovementQueriesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_IsBlockedMovementQueriesTag::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::IsBlockedMovement"));
	if (!TestTrue(
			TEXT("B1: BmrMoverComponent.cpp must define IsBlockedMovement with a body — the start "
				 "stub returns false, which silently lies to every caller (cheat manager, AI "
				 "controller, game state transition all read 'not blocked' even when the GE is "
				 "applied)."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B1: IsBlockedMovement must query the ASC's gameplay-tag store via "
			 "HasMatchingGameplayTag — the spec's named failure mode is 'Boolean movement block — "
			 "external tag adders diverge from the bool.' Three independent systems (cheat manager, "
			 "AI controller, game-state) apply/remove the Block::Movement tag through the GAS "
			 "surface; a bool field doesn't see those mutations and the component disagrees with "
			 "the tag store on whether the pawn can move."),
		CodeOnly.Contains(TEXT("HasMatchingGameplayTag"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B1: IsBlockedMovement must reference the Block::Movement gameplay tag specifically. "
			 "Querying a different tag (or no tag) would let unrelated GE applications either "
			 "spuriously block movement or never block it. The canonical encoding names "
			 "BmrGameplayTags::GameplayEffect::Block::Movement."),
		CodeOnly.Contains(TEXT("Block::Movement"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("Block.Movement"), ESearchCase::CaseSensitive));

	// The query must run against the live ASC retrieved via
	// UAbilitySystemGlobals::GetAbilitySystemComponentFromActor — not via a
	// cached or member pointer that might go stale across pool reuse.
	TestTrue(
		TEXT("B1: IsBlockedMovement must fetch the ASC from the owner via "
			 "UAbilitySystemGlobals::GetAbilitySystemComponentFromActor (or an equivalent "
			 "GetAbilitySystemComponent call). The pawn is pool-reused; reading from a stale "
			 "cached ASC pointer would read the wrong store after the owner is recycled."),
		CodeOnly.Contains(TEXT("GetAbilitySystemComponentFromActor"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("GetAbilitySystemComponent"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B2: ProduceInput rotates the directional intent by the level
//              grid's yaw before injecting it.
//
// The spec: "The directional input must be rotated by the level grid's
// own yaw before being set — without this, WASD direction diverges from
// the cell grid when the map transform is rotated." The map can be placed
// into the world at any yaw (a designer may rotate it by 90° to align
// with a different camera angle); the cell grid is rotated with it, but
// the pawn's controller-relative WASD intent is in world space. Without
// the rotation step, pressing "right" in screen space moves the pawn in
// a direction that may not be parallel to any cell row, so the pawn
// drifts off the lattice and gets stuck at a corner.
//
// The canonical encoding is:
//   const FRotator LevelGridRotation = UBmrCellUtilsLibrary::GetLevelGridRotation();
//   const FVector FinalDirectionalIntent = LevelGridRotation.RotateVector(CurrentMoveInput);
//   CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, FinalDirectionalIntent);
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_ProduceInputRotatesByLevelGrid,
	"Bomber.MoverComponent.ProduceInputRotatesByLevelGrid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_ProduceInputRotatesByLevelGrid::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::ProduceInput"));
	if (!TestTrue(
			TEXT("B2: BmrMoverComponent.cpp must define ProduceInput with a body — the start stub "
				 "just calls Super and the entire input-injection pipeline is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B2: ProduceInput must read the level grid's yaw via "
			 "UBmrCellUtilsLibrary::GetLevelGridRotation. The named failure mode: 'Skipping level-"
			 "grid rotation in input production — WASD doesn't follow a rotated map.' Without the "
			 "level-grid yaw, controller-relative input stays in world space; on a rotated map the "
			 "pawn drifts off the cell lattice and gets stuck at corners."),
		CodeOnly.Contains(TEXT("GetLevelGridRotation"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: ProduceInput must actually rotate CurrentMoveInput by the level-grid rotation "
			 "(FRotator::RotateVector(CurrentMoveInput) or equivalent). Reading the rotation but "
			 "not applying it produces the same failure as not reading it at all — the input "
			 "stays in world space."),
		CodeOnly.Contains(TEXT("RotateVector"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("CurrentMoveInput"), ESearchCase::CaseSensitive));

	// The rotated intent must reach Mover via SetMoveInput on the
	// FCharacterDefaultInputs payload. Without this, the rotation is computed
	// but never injected and Mover keeps consuming the previous frame's input.
	TestTrue(
		TEXT("B2: ProduceInput must call SetMoveInput on the FCharacterDefaultInputs payload with "
			 "EMoveInputType::DirectionalIntent and the rotated vector. Without this final inject "
			 "the rotated value is computed and discarded; Mover reads stale input and the pawn's "
			 "direction lags by one frame on every change."),
		CodeOnly.Contains(TEXT("SetMoveInput"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("DirectionalIntent"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B3 (failure-mode 3): TeleportToLocation routes through Mover's
//              instant-movement-effect mechanism when BackendLiaisonComp
//              is available. The SetActorLocation path is the fallback
//              for the null-liaison case only.
//
// The spec: "Routes through Mover's instant-movement-effect mechanism
// when the backend liaison is available. Bypassing it and calling
// SetActorLocation directly causes Mover to rubber-band clients."
// Mover-2.0 owns the authoritative simulation state for the pawn; an
// out-of-band SetActorLocation moves the actor immediately but the
// Mover-managed sync state still holds the old position, and on the
// next replication tick the client's interpolation snaps back to the
// stale Mover state — the rubber-band the spec names.
//
// The fallback (BackendLiaisonComp == nullptr) is real and necessary:
// in editor preview / during component initialization the liaison may
// not yet be registered. In that mode SetActorLocation is the only
// option, but it is also harmless because there is no replicated client
// to rubber-band yet. The asymmetry — QueueInstantMovementEffect on the
// happy path, SetActorLocation only on the fallback — is what the test
// pins down.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_TeleportUsesInstantMovementEffect,
	"Bomber.MoverComponent.TeleportUsesInstantMovementEffect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_TeleportUsesInstantMovementEffect::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::TeleportToLocation"));
	if (!TestTrue(
			TEXT("B3: BmrMoverComponent.cpp must define TeleportToLocation with a body — the start "
				 "stub is empty; the height-offset + liaison routing is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Happy path: QueueInstantMovementEffect routes through Mover's sync state
	// so client interpolation lands on the same final position the authority
	// commits to.
	TestTrue(
		TEXT("B3: TeleportToLocation must route through QueueInstantMovementEffect on the happy "
			 "path. The named failure mode: 'Direct SetActorLocation in teleport — Mover rubber-"
			 "bands clients.' QueueInstantMovementEffect updates Mover's authoritative sync state "
			 "in lockstep with the visual move; SetActorLocation moves the actor out-of-band and "
			 "leaves Mover's sync state stale, so client interpolation snaps back on the next "
			 "replication tick."),
		CodeOnly.Contains(TEXT("QueueInstantMovementEffect"), ESearchCase::CaseSensitive));

	// FTeleportEffect is the Mover-2.0 payload type for an instant location
	// change. A re-impl that uses a different effect type wouldn't satisfy
	// the QueueInstantMovementEffect signature and would silently no-op.
	TestTrue(
		TEXT("B3: TeleportToLocation must construct an FTeleportEffect for the "
			 "QueueInstantMovementEffect call. That is the Mover-2.0 payload type for an instant "
			 "location change; other effect types either don't land the location or produce a "
			 "different sync-state mutation."),
		CodeOnly.Contains(TEXT("FTeleportEffect"), ESearchCase::CaseSensitive));

	// The height offset from the data asset must be applied — the spec calls
	// this out as its own requirement under "Teleport".
	TestTrue(
		TEXT("B3: TeleportToLocation must apply the height offset from "
			 "UBmrGeneratedMapDataAsset::Get().GetActorsHeightOffset(). The spec: 'Applies a "
			 "height offset from the data asset.' Without this, the pawn teleports embedded in "
			 "the floor (or floating above it, depending on the offset sign)."),
		CodeOnly.Contains(TEXT("GetActorsHeightOffset"), ESearchCase::CaseSensitive));

	// SetActorLocation may appear, but only on the BackendLiaisonComp-null
	// fallback. The check is structural: if both SetActorLocation and a
	// BackendLiaisonComp test appear, the SetActorLocation must follow the
	// liaison-null check (i.e. the SetActorLocation offset is greater than
	// the liaison-test offset).
	const int32 SetActorLocationIdx = FindOffsetIn(CodeOnly, TEXT("SetActorLocation"));
	const int32 LiaisonIdx = FindOffsetIn(CodeOnly, TEXT("BackendLiaisonComp"));
	if (SetActorLocationIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B3: If TeleportToLocation calls SetActorLocation, it must do so only on the "
				 "BackendLiaisonComp-null fallback path. The check on BackendLiaisonComp must "
				 "appear in the body and precede the SetActorLocation call. A SetActorLocation "
				 "that runs unconditionally — or on the happy path — is the named failure mode "
				 "and causes client rubber-banding."),
			LiaisonIdx != INDEX_NONE && LiaisonIdx < SetActorLocationIdx);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B4: OnPostMove's cell-update split is mutually exclusive
//              between authority and locally-controlled non-authority.
//
// The spec: "Which peer performs the cell update depends on network
// role: authority uses the generated map's snap-to-nearest-cell path;
// locally-controlled non-authority snaps client-side; remote proxies
// skip (cell arrives via replication). Both the authority and the
// local client must not update simultaneously."
//
// In listen-server mode the host pawn is BOTH authority AND locally
// controlled. If the two branches are if / if instead of if / else if,
// the host writes the cell twice per tick: once via
// ABmrGeneratedMap::Get().SetNearestCell(MapComponent) (the server snap,
// which rounds the actor's world position to the nearest cell index)
// and once via MapComponent->SetCell(SnapActorOnLevel(...)) (the local
// snap, which uses a different rounding helper). When the actor sits
// between two cells, the two snap helpers can return different cell
// indices on the same tick — the cell value flips between the two on
// every tick. Downstream systems (the bomb placement gate, the
// explosion path) read whichever cell happened to be written last and
// targeting becomes non-deterministic.
//
// The canonical encoding is `if (HasAuthority) { ... } else if
// (IsLocallyControlled) { ... }` — a single chained conditional, not
// two independent ones.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_OnPostMoveCellUpdateMutuallyExclusive,
	"Bomber.MoverComponent.OnPostMoveCellUpdateMutuallyExclusive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_OnPostMoveCellUpdateMutuallyExclusive::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove"));
	}
	if (!TestTrue(
			TEXT("B4: BmrMoverComponent.cpp must define OnPostMove (or its _Implementation BNE "
				 "body) with a body — the start stub is empty; the sync-state inject + cell-update "
				 "split is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Both branches' anchor calls must be present — authority uses the
	// generated-map's snap helper, locally-controlled non-authority uses the
	// MapComponent's direct SetCell driven by the cell-utility snap.
	TestTrue(
		TEXT("B4: OnPostMove must call ABmrGeneratedMap::...SetNearestCell on the authority "
			 "branch — that is the server-side snap path that walks the cell grid and writes the "
			 "rounded index. Without this, the authoritative cell never updates and replicated "
			 "systems (bomb placement, explosion path) read a stale position."),
		CodeOnly.Contains(TEXT("SetNearestCell"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: OnPostMove must call MapComponent->SetCell(...) on the locally-controlled non-"
			 "authority branch — that is the responsiveness path that lets the local client see "
			 "their pawn's cell update before the server replicates it back. The canonical input "
			 "is UBmrCellUtilsLibrary::SnapActorOnLevel(Pawn)."),
		CodeOnly.Contains(TEXT("SetCell"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("SnapActorOnLevel"), ESearchCase::CaseSensitive));

	// The two branches must be in a single chained if/else, not two separate
	// ifs. The structural signal: SetNearestCell and SetCell both appear, and
	// the body contains an `else if` between them OR an `else` clause near
	// the locally-controlled check.
	const int32 SetNearestIdx = FindOffsetIn(CodeOnly, TEXT("SetNearestCell"));
	const int32 LocallyControlledIdx = FindOffsetIn(CodeOnly, TEXT("IsLocallyControlled"));
	const int32 HasAuthorityIdx = FindOffsetIn(CodeOnly, TEXT("HasAuthority"));
	if (TestTrue(
			TEXT("B4: OnPostMove must consult HasAuthority and IsLocallyControlled to decide "
				 "which peer writes the cell. The spec is explicit: 'Which peer performs the cell "
				 "update depends on network role.' Without these checks both branches run on the "
				 "listen-server host and the cell flips between two snap helpers' results every "
				 "tick (the named race condition)."),
			HasAuthorityIdx != INDEX_NONE && LocallyControlledIdx != INDEX_NONE))
	{
		// The locally-controlled check must appear strictly after the
		// authority check — an `else if` chain. A re-impl that reverses the
		// order (locally-controlled first, then authority) would let the host
		// take the client branch and skip SetNearestCell entirely.
		TestTrue(
			TEXT("B4: The HasAuthority check must precede the IsLocallyControlled check — the "
				 "canonical encoding is `if (HasAuthority) { server snap } else if "
				 "(IsLocallyControlled) { client snap }`. Reversing the order would let a listen-"
				 "server host (which is both authority AND locally-controlled) take the client "
				 "branch and skip SetNearestCell, so the authoritative cell never updates."),
			HasAuthorityIdx < LocallyControlledIdx);

		// The else-if chain: look for an `else` between the authority branch
		// and the locally-controlled branch. The check is on the literal
		// `else` keyword appearing in the window between the two — a re-impl
		// with two independent ifs would have no `else` between SetNearestCell
		// and IsLocallyControlled, which is the named race condition.
		const FString WindowBetween = CodeOnly.Mid(
			HasAuthorityIdx, FMath::Max(0, LocallyControlledIdx - HasAuthorityIdx));
		TestTrue(
			TEXT("B4: The two cell-update branches must be chained via `else if`, not written as "
				 "two independent `if` blocks. The named failure mode: 'Both authority and local "
				 "client updating the cell — race condition, cell flips between values.' On a "
				 "listen-server host both predicates are true; without the `else`, both writers "
				 "fire and the cell oscillates between SetNearestCell's index and "
				 "SnapActorOnLevel's index every tick."),
			WindowBetween.Contains(TEXT("else"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B5: OnPawnReady guards the Skate-attribute delegate bind
//              with IsBoundToObject(this).
//
// The spec: "Guard the delegate bind against duplicate registration —
// the pawn is pool-reused across rounds." Pawns are recycled by the
// PoolManager subsystem: at round end the pawn returns to the pool;
// at round start it is reactivated, BeginPlay fires again, the global
// message subsystem re-broadcasts Player_PawnReady, and OnPawnReady is
// invoked again. Without the IsBoundToObject(this) guard, every reuse
// cycle adds another AddUObject subscription to the ASC's
// FOnGameplayAttributeValueChange delegate — by round N the
// OnSkateAttributeChanged handler runs N times per attribute change,
// each invocation writing CachedSkatePowerupAttribute to the same
// value (so the bug is silent in test) but burning N delegate calls
// per Skate pickup.
//
// The canonical encoding is:
//   if (!SkateAttributeDelegate.IsBoundToObject(this))
//   {
//       SkateAttributeDelegate.AddUObject(this, &ThisClass::OnSkateAttributeChanged);
//       ...
//   }
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_OnPawnReadyGuardsAgainstDuplicateBind,
	"Bomber.MoverComponent.OnPawnReadyGuardsAgainstDuplicateBind",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_OnPawnReadyGuardsAgainstDuplicateBind::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPawnReady_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPawnReady"));
	}
	if (!TestTrue(
			TEXT("B5: BmrMoverComponent.cpp must define OnPawnReady (or its _Implementation BNE "
				 "body) with a body — the start stub is empty; the Skate-attribute bind is what "
				 "was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The Skate attribute's value-change delegate must be retrieved from the
	// ASC. The canonical encoding is
	// ASC->GetGameplayAttributeValueChangeDelegate(GetPowerup_SkateAttribute()).
	TestTrue(
		TEXT("B5: OnPawnReady must fetch the Skate-attribute change delegate via "
			 "GetGameplayAttributeValueChangeDelegate (with the Powerup_Skate attribute). That is "
			 "the GAS hook that fires on every Skate-attribute mutation; the cached value lives "
			 "or dies by this subscription."),
		CodeOnly.Contains(TEXT("GetGameplayAttributeValueChangeDelegate"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B5: OnPawnReady's delegate fetch must target the Powerup_Skate attribute "
			 "specifically (GetPowerup_SkateAttribute or an equivalent attribute getter). Binding "
			 "to a different attribute would cache the wrong value and the walking-mode speed "
			 "multiplier would never update on Skate pickups."),
		CodeOnly.Contains(TEXT("Powerup_Skate"), ESearchCase::CaseSensitive));

	// The duplicate-bind guard. The spec's named failure mode: 'No duplicate-
	// bind guard on the Skate attribute delegate — pool reuse creates duplicate
	// subscribers.' IsBoundToObject(this) is the canonical guard.
	TestTrue(
		TEXT("B5: OnPawnReady must guard the AddUObject bind with IsBoundToObject(this). The "
			 "named failure mode: 'No duplicate-bind guard on the Skate attribute delegate — pool "
			 "reuse creates duplicate subscribers.' The pawn is pool-recycled across rounds; "
			 "every reuse triggers BeginPlay → Player_PawnReady → OnPawnReady, so each round "
			 "stacks another subscription without the guard."),
		CodeOnly.Contains(TEXT("IsBoundToObject"), ESearchCase::CaseSensitive));

	// The bind itself must be AddUObject pointing at OnSkateAttributeChanged.
	// AddRaw / AddLambda would either leak or fail to clean up on the pawn's
	// destruction; AddUObject is the only form that respects UObject lifetime.
	TestTrue(
		TEXT("B5: The Skate-attribute bind must use AddUObject(this, &ThisClass::"
			 "OnSkateAttributeChanged). AddUObject is the only form that respects UObject "
			 "lifetime and auto-unbinds when the component is destroyed; AddRaw / AddLambda would "
			 "leave a dangling subscriber that fires after the pawn returns to the pool."),
		CodeOnly.Contains(TEXT("AddUObject"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("OnSkateAttributeChanged"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B6: OnPostMove reads the CACHED Skate value, not the live ASC
//              attribute.
//
// The spec: "The Skate powerup value must be readable per-tick without
// querying the ASC on every frame. Cache the value when the pawn becomes
// ready, update it through an attribute-change delegate, and read only
// the cache in the post-move path." The named failure mode: "Reading the
// Skate attribute from the ASC per-tick — use the cached value."
//
// OnPostMove runs once per simulation tick on every pawn — at 60Hz with
// 4 pawns that is 240 GAS attribute reads per second on a path that has
// nothing else to do. Each read traverses the ASC's attribute-set list
// to find Powerup_Skate, locks the attribute-aggregator mutex, and
// allocates a transient FGameplayAttribute lookup. The cached-value
// approach reduces this to a single float read.
//
// The canonical encoding writes CachedSkatePowerupAttribute (the field)
// into the FBmrMoverSyncState; it does NOT call
// GetGameplayAttributeValue or any Powerup_Skate accessor inside the
// OnPostMove body.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_OnPostMoveUsesCachedSkate,
	"Bomber.MoverComponent.OnPostMoveUsesCachedSkate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_OnPostMoveUsesCachedSkate::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove"));
	}
	if (!TestTrue(
			TEXT("B6: BmrMoverComponent.cpp must define OnPostMove (or its _Implementation BNE "
				 "body) with a body."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Positive: OnPostMove must reference CachedSkatePowerupAttribute (the
	// cached float field). That is the read the cached path is built around.
	TestTrue(
		TEXT("B6: OnPostMove must read CachedSkatePowerupAttribute and inject it into the sync "
			 "state. The spec: 'After each move step, always inject the cached Skate powerup "
			 "value into Mover's sync state.' Writing a hard-coded value or skipping the inject "
			 "entirely breaks the walking-mode speed-multiplier path that consumes the sync "
			 "state."),
		CodeOnly.Contains(TEXT("CachedSkatePowerupAttribute"), ESearchCase::CaseSensitive));

	// The sync state collection must be touched — FindOrAddMutableDataByType<
	// FBmrMoverSyncState> is the canonical access. Without this the cached
	// value is read but never published.
	TestTrue(
		TEXT("B6: OnPostMove must mutate the SyncState collection via "
			 "FindOrAddMutableDataByType<FBmrMoverSyncState> (or an equivalent typed accessor). "
			 "That is the contract Mover-2.0 uses to surface custom per-tick state to other "
			 "movement modes and to clients via replication. A read that doesn't reach the sync "
			 "state never reaches the consumer."),
		CodeOnly.Contains(TEXT("FBmrMoverSyncState"), ESearchCase::CaseSensitive)
			&& (CodeOnly.Contains(TEXT("FindOrAddMutableDataByType"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("SyncStateCollection"), ESearchCase::CaseSensitive)));

	// Negative: OnPostMove must NOT read the live attribute. The named anti-
	// patterns are GetGameplayAttributeValue / a Powerup_Skate accessor call
	// inside this function — either bypasses the cache the spec mandates and
	// re-introduces the per-tick GAS query.
	const bool bReadsLiveAttribute =
		CodeOnly.Contains(TEXT("GetGameplayAttributeValue"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetPowerup_Skate("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetNumericAttribute"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B6: OnPostMove must NOT read the live Skate attribute from the ASC "
			 "(GetGameplayAttributeValue / GetPowerup_Skate / GetNumericAttribute). The named "
			 "failure mode: 'Reading the Skate attribute from the ASC per-tick — use the cached "
			 "value.' The whole point of the cache + delegate plumbing is to keep the post-move "
			 "hot path off the GAS attribute-aggregator mutex; a live read undoes the "
			 "optimisation and serialises every pawn's post-move tick behind the same lock."),
		bReadsLiveAttribute);

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — B7: OnPostMove early-returns when movement is blocked.
//
// The spec: "Then update the pawn's grid cell — but only if movement
// is not blocked." The named failure mode: "Cell update running when
// movement is blocked — dead pawn updates cell on an empty grid."
//
// OnPreRemovedFromLevel (Test 9) calls SetBlockMovement(true) before
// the pawn is unregistered from the generated map. The block lives
// for the brief window between the pre-remove broadcast and the
// actual map teardown. During that window the pawn's actor is still
// in the world (so its position still moves under any residual
// velocity) but its MapComponent is being removed from the
// ABmrGeneratedMap's actor list. If OnPostMove tries to write the
// cell during that window it either (a) writes to a cell index that
// has just been freed for another spawn — overwriting it — or (b) hits
// the checkf in MapComponent::GetMapComponent when the dead pawn's
// component has already been pulled.
//
// The canonical encoding is a top-of-function early-out:
//   if (IsBlockedMovement()) { return; }
// — same as ProduceInput's block branch.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_OnPostMoveEarlyReturnsWhenBlocked,
	"Bomber.MoverComponent.OnPostMoveEarlyReturnsWhenBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_OnPostMoveEarlyReturnsWhenBlocked::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPostMove"));
	}
	if (!TestTrue(
			TEXT("B7: BmrMoverComponent.cpp must define OnPostMove (or its _Implementation BNE "
				 "body) with a body."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// IsBlockedMovement must be called and the result must guard an early
	// return. The strongest structural signal: IsBlockedMovement appears
	// BEFORE the sync-state write (CachedSkatePowerupAttribute / FBmrMoverSyncState
	// access) and BEFORE the cell-update calls.
	const int32 BlockedIdx = FindOffsetIn(CodeOnly, TEXT("IsBlockedMovement"));
	TestTrue(
		TEXT("B7: OnPostMove must call IsBlockedMovement at the top of the function. The named "
			 "failure mode: 'Cell update running when movement is blocked — dead pawn updates "
			 "cell on an empty grid.' The pre-removal block (Test 9) relies on this check to "
			 "freeze the pawn's last cell write before its MapComponent is torn down."),
		BlockedIdx != INDEX_NONE);

	const int32 SyncIdx = FindOffsetIn(CodeOnly, TEXT("CachedSkatePowerupAttribute"));
	const int32 SetNearestIdx = FindOffsetIn(CodeOnly, TEXT("SetNearestCell"));
	const int32 SetCellIdx = FindOffsetIn(CodeOnly, TEXT("SetCell"));

	if (BlockedIdx != INDEX_NONE && SyncIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B7: The IsBlockedMovement check must precede the sync-state write — the spec "
				 "explicitly: 'Then update the pawn's grid cell — but only if movement is not "
				 "blocked.' A sync-state write that runs while blocked still injects the cached "
				 "Skate value into a sync state for a pawn whose grid context is being torn down, "
				 "which downstream movement modes may consume in inconsistent state."),
			BlockedIdx < SyncIdx);
	}

	if (BlockedIdx != INDEX_NONE && SetNearestIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B7: The IsBlockedMovement check must precede the authority cell-update "
				 "(SetNearestCell). Running the server-side cell snap on a pawn that has just been "
				 "pre-removed from the level overwrites whatever cell index was freed for the next "
				 "respawn, corrupting the generated map's actor index."),
			BlockedIdx < SetNearestIdx);
	}

	if (BlockedIdx != INDEX_NONE && SetCellIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B7: The IsBlockedMovement check must precede the client cell-update "
				 "(MapComponent->SetCell). Running the client-side snap on a pawn whose "
				 "MapComponent is about to be unregistered races the teardown and either reads "
				 "from a partially-torn-down map or writes a cell that no longer maps to this "
				 "pawn."),
			BlockedIdx < SetCellIdx);
	}

	// The blocked branch must early-return — a `return` keyword must appear
	// before the sync-state write. Walking through the body: from the
	// IsBlockedMovement offset, the next `return` must come before the first
	// CachedSkatePowerupAttribute or SetNearestCell offset.
	if (BlockedIdx != INDEX_NONE)
	{
		const int32 FirstReturnAfterBlocked = CodeOnly.Find(
			TEXT("return"), ESearchCase::CaseSensitive, ESearchDir::FromStart, BlockedIdx);
		const int32 FirstWriteAfterBlocked = [&]() -> int32
		{
			int32 Best = INT32_MAX;
			for (int32 Idx : {SyncIdx, SetNearestIdx, SetCellIdx})
			{
				if (Idx != INDEX_NONE && Idx > BlockedIdx)
				{
					Best = FMath::Min(Best, Idx);
				}
			}
			return Best == INT32_MAX ? INDEX_NONE : Best;
		}();

		if (FirstReturnAfterBlocked != INDEX_NONE && FirstWriteAfterBlocked != INDEX_NONE)
		{
			TestTrue(
				TEXT("B7: The IsBlockedMovement branch must early-return (the `return` keyword) "
					 "BEFORE the sync-state / cell-update side effects. A check that logs the "
					 "block but still falls through to the writes produces the same dead-pawn-"
					 "writes-cell failure the early-return is meant to prevent."),
				FirstReturnAfterBlocked < FirstWriteAfterBlocked);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — BeginPlay's binding fan-out.
//
// The spec: "On BeginPlay: bind to post-movement, pawn-ready, game-state-
// changed, pre-removed-from-level, and controller-changed events." Five
// separate event sources, each driving a distinct concern: post-movement
// runs OnPostMove (the sync-state + cell-update pipeline, Tests 4/6/7);
// pawn-ready runs OnPawnReady (the Skate-attribute bind, Test 5);
// game-state-changed runs OnGameStateChanged (the in-game gate); pre-
// removed-from-level runs OnPreRemovedFromLevel (the death block, Test 9);
// controller-changed runs OnControllerChanged (the possession-loss
// block). Dropping any of these silently disables a feature whose only
// trigger lives on the missing event — e.g. a re-impl that forgets to
// bind OnPostMovement never runs the sync-state write at all, so client
// Skate-attribute interpolation falls back to the un-replicated default.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_BeginPlayBindsAllEvents,
	"Bomber.MoverComponent.BeginPlayBindsAllEvents",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_BeginPlayBindsAllEvents::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::BeginPlay"));
	if (!TestTrue(
			TEXT("BmrMoverComponent.cpp must define BeginPlay with a body — the start stub just "
				 "calls Super and every per-pawn binding is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("BeginPlay must call Super::BeginPlay — UMoverComponent's base does its own per-"
			 "frame registration, and skipping it leaves Mover-2.0 with no simulation hook to "
			 "drive ProduceInput / OnPostMove from."),
		CodeOnly.Contains(TEXT("Super::BeginPlay"), ESearchCase::CaseSensitive));

	// The OnPostMovement multicast on the base UMoverComponent is the trigger
	// for OnPostMove (Tests 4/6/7). Without binding it, the entire sync-state
	// + cell-update pipeline is dead — the spec's per-tick contract simply
	// never runs.
	TestTrue(
		TEXT("BeginPlay must bind OnPostMove to OnPostMovement (the base UMoverComponent's "
			 "broadcast after each simulation tick). Without this bind the entire sync-state + "
			 "cell-update pipeline (Tests 4/6/7) never runs and the cell-driven systems read "
			 "stale positions every tick."),
		CodeOnly.Contains(TEXT("OnPostMovement"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("OnPostMove"), ESearchCase::CaseSensitive));

	// Player_PawnReady drives OnPawnReady (Test 5). Without it, the Skate-
	// attribute cache is never initialised and CachedSkatePowerupAttribute
	// stays 0 forever; the walking-mode speed-multiplier is permanently the
	// base value regardless of Skate pickups.
	TestTrue(
		TEXT("BeginPlay must subscribe to BmrGameplayTags::Event::Player_PawnReady (typically "
			 "via UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage). That listener "
			 "is the only path that runs OnPawnReady, which in turn primes the Skate-attribute "
			 "cache and binds its change delegate (Test 5)."),
		CodeOnly.Contains(TEXT("Player_PawnReady"), ESearchCase::CaseSensitive));

	// GameState_Changed drives OnGameStateChanged → SetBlockMovement based on
	// whether the InGame tag is present. Without it, movement is never
	// blocked when transitioning out of InGame and pawns keep moving in the
	// end-of-round freeze frame.
	TestTrue(
		TEXT("BeginPlay must subscribe to BmrGameplayTags::Event::GameState_Changed. That "
			 "listener routes game-state transitions (Menu / GameStarting / InGame / EndGame) "
			 "into SetBlockMovement, so pawns freeze when the round ends and unfreeze when it "
			 "starts. Missing this bind keeps pawns mobile through end-of-round."),
		CodeOnly.Contains(TEXT("GameState_Changed"), ESearchCase::CaseSensitive));

	// OnPreRemovedFromLevel binds via the MapComponent's broadcast. Without
	// this, the death-block (Test 9) never fires and OnPostMove writes cells
	// during the dead pawn's teardown window (the B7 failure mode).
	TestTrue(
		TEXT("BeginPlay must bind OnPreRemovedFromLevel via the owner's MapComponent (the "
			 "UBmrMapComponent::OnPreRemovedFromLevel multicast). That bind is what wires the "
			 "death-window block (Test 9) that keeps OnPostMove from running while the cell is "
			 "being torn down."),
		CodeOnly.Contains(TEXT("OnPreRemovedFromLevel"), ESearchCase::CaseSensitive));

	// ReceiveControllerChangedDelegate fires when the pawn's controller is
	// (re-)assigned. The bind runs OnControllerChanged, which blocks movement
	// on possession loss.
	TestTrue(
		TEXT("BeginPlay must bind OnControllerChanged via the owner's "
			 "ReceiveControllerChangedDelegate. That bind is the only trigger for the "
			 "possession-loss block — without it, a pawn that loses its controller mid-round "
			 "keeps consuming its last input vector and walks off in a straight line until the "
			 "next game-state transition fires the GameState_Changed listener instead."),
		CodeOnly.Contains(TEXT("ReceiveControllerChangedDelegate"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — OnPreRemovedFromLevel triggers a movement block.
//
// The spec: "Controller loss, game-state transitions outside of InGame,
// and pre-removal-from-level all trigger a movement block — the pre-
// removal block is what stops the pawn from injecting input while its
// grid cell is being torn down on death."
//
// This is the trigger that Test 7's early-return is the response to.
// Without this block (or with a block on the wrong event, e.g.
// OnRemovedFromLevel which fires AFTER teardown rather than BEFORE),
// the dead pawn keeps running ProduceInput and OnPostMove for the
// window between death detection and actual map removal — exactly the
// window in which the cell is being unregistered.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_OnPreRemovedFromLevelBlocksMovement,
	"Bomber.MoverComponent.OnPreRemovedFromLevelBlocksMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_OnPreRemovedFromLevelBlocksMovement::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body = ExtractMemberBody(
		Source, TEXT("UBmrMoverComponent::OnPreRemovedFromLevel_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::OnPreRemovedFromLevel"));
	}
	if (!TestTrue(
			TEXT("BmrMoverComponent.cpp must define OnPreRemovedFromLevel (or its _Implementation "
				 "BNE body) with a body — the start stub is empty; the death-window block is "
				 "what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The body must call SetBlockMovement(true). The spec is explicit and
	// brief — this is a one-liner whose only contract is the block.
	TestTrue(
		TEXT("OnPreRemovedFromLevel must call SetBlockMovement(true). The spec: 'the pre-removal "
			 "block is what stops the pawn from injecting input while its grid cell is being "
			 "torn down on death.' Without this call, the dead pawn keeps running ProduceInput / "
			 "OnPostMove for the window between death detection and actual map removal — exactly "
			 "the window in which the cell is being unregistered (the B7 race)."),
		CodeOnly.Contains(TEXT("SetBlockMovement"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("true"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — SetBlockMovement applies the block GE on true / removes it
//           on false.
//
// The spec: "Movement can be blocked by the cheat manager, the AI
// controller, and game-state transitions — all independently. Because
// multiple systems add and remove the block, it must be tracked through
// a GAS gameplay tag rather than a boolean. ... Blocking zeroes the
// current move input and injects a do-nothing input to Mover."
//
// The on-true path applies the BlockMovementEffect from the player data
// asset (granted tags include Block::Movement) and zeroes
// CurrentMoveInput so the next ProduceInput call sees a clean slate.
// The on-false path removes the effect with that granted tag — not the
// effect class, since multiple systems may have applied effects whose
// granted-tag set includes Block::Movement.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_SetBlockMovementAppliesAndRemovesByTag,
	"Bomber.MoverComponent.SetBlockMovementAppliesAndRemovesByTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_SetBlockMovementAppliesAndRemovesByTag::RunTest(const FString& Parameters)
{
	using namespace MoverComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UBmrMoverComponent::SetBlockMovement"));
	if (!TestTrue(
			TEXT("BmrMoverComponent.cpp must define SetBlockMovement with a body — the start stub "
				 "is empty; the apply / remove fork is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// On-true: apply a GE. The canonical source is UBmrPlayerDataAsset::Get().
	// GetBlockMovementEffect() — the data-asset-authored block effect. Applying
	// any other GE (or hard-coding a class) drifts from the data asset and the
	// granted tag may not match what IsBlockedMovement queries.
	TestTrue(
		TEXT("SetBlockMovement's enable path must apply the BlockMovementEffect from the player "
			 "data asset (GetBlockMovementEffect). Applying a different GE would either grant the "
			 "wrong tag (so IsBlockedMovement disagrees) or skip the cooldown / immunity payload "
			 "the data-asset-authored effect carries."),
		CodeOnly.Contains(TEXT("GetBlockMovementEffect"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("SetBlockMovement's enable path must apply the effect via ApplyGameplayEffectToSelf "
			 "(or an equivalent ASC-side apply). That is the GAS surface that grants the effect's "
			 "tag set onto the ASC's tag store, which is what IsBlockedMovement reads."),
		CodeOnly.Contains(TEXT("ApplyGameplayEffectToSelf"), ESearchCase::CaseSensitive));

	// On-false: removal must be by granted-tag, not by effect class. Multiple
	// systems may have stacked GEs whose granted tags include Block::Movement
	// (cheat manager applies the data-asset effect; AI may apply a different
	// effect that also grants the tag); removing by class would miss those.
	TestTrue(
		TEXT("SetBlockMovement's disable path must call RemoveActiveEffectsWithGrantedTags (or "
			 "RemoveActiveGameplayEffectBySourceEffect with the granted-tag query). Removing by "
			 "effect class would miss any GE applied by another system that grants the "
			 "Block::Movement tag from a different source — the spec is explicit that multiple "
			 "systems can apply blocks."),
		CodeOnly.Contains(TEXT("RemoveActiveEffectsWithGrantedTags"), ESearchCase::CaseSensitive));

	// On-true must also zero CurrentMoveInput so the next ProduceInput sees a
	// clean slate; otherwise a pawn that pressed "right" the frame before the
	// block keeps that intent cached until the block is released.
	TestTrue(
		TEXT("SetBlockMovement's enable path must zero CurrentMoveInput (typically via "
			 "RequestMoveByIntent(FVector::ZeroVector)). The spec: 'Blocking zeroes the current "
			 "move input and injects a do-nothing input to Mover.' Without this, a pawn that "
			 "pressed `right` the frame before the block keeps that intent cached and resumes "
			 "moving right the instant the block is released."),
		CodeOnly.Contains(TEXT("FVector::ZeroVector"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("CurrentMoveInput"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — Runtime UClass surface.
//
// The Bomber module is loaded by the test process (every test in this
// suite includes a Bomber header), so UBmrMoverComponent's UClass /
// UFUNCTION / UPROPERTY reflection data is registered before any test
// runs. This test reaches the contract via the engine reflection
// database instead of via source-text scanning, catching API-rename
// regressions that source scans alone would miss: a re-impl that
// renames a UFUNCTION breaks every Blueprint caller (the pawn
// blueprints call into this surface by name) and every C++ caller
// that uses ProcessEvent.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_RuntimeUClassSurface,
	"Bomber.MoverComponent.RuntimeUClassSurface",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_RuntimeUClassSurface::RunTest(const FString& Parameters)
{
	UClass* ComponentClass = UBmrMoverComponent::StaticClass();
	if (!TestNotNull(
			TEXT("UBmrMoverComponent::StaticClass() must resolve at runtime — the Bomber module "
				 "must be loaded for the reflection database to register the class."),
			ComponentClass))
	{
		return true;
	}

	// The component must derive from UMoverComponent, NOT UCharacterMovementComponent.
	// A re-impl that swaps the base class would lose the Mover-2.0
	// simulation hook the spec is built around (ProduceInput, OnPostMovement
	// multicast, sync-state collection, QueueInstantMovementEffect).
	UClass* MoverBase = FindObject<UClass>(nullptr, TEXT("/Script/Mover.MoverComponent"));
	if (TestNotNull(
			TEXT("UMoverComponent must be discoverable via reflection — the Mover-2.0 plugin "
				 "must be loaded. If this fails the entire suite is meaningless because the "
				 "stripped file's includes wouldn't resolve either."),
			MoverBase))
	{
		TestTrue(
			TEXT("UBmrMoverComponent must derive from UMoverComponent. The spec is built on "
				 "Mover-2.0 contracts (ProduceInput override, OnPostMovement multicast, sync-state "
				 "collection, QueueInstantMovementEffect) — a different base class would lose all "
				 "of them and the tests in this suite would be scanning a file that doesn't even "
				 "implement the right interface."),
			ComponentClass->IsChildOf(MoverBase));
	}

	// The five public UFUNCTIONs on the spec must be reflected. Each is a
	// BlueprintCallable surface a pawn blueprint may invoke by FName.
	const TCHAR* ExpectedUFunctions[] = {
		TEXT("RequestMoveByIntent"),
		TEXT("TeleportToLocation"),
		TEXT("SetBlockMovement"),
		TEXT("IsBlockedMovement"),
	};
	for (const TCHAR* FnName : ExpectedUFunctions)
	{
		UFunction* Fn = ComponentClass->FindFunctionByName(FName(FnName));
		TestNotNull(
			*FString::Printf(
				TEXT("UBmrMoverComponent must expose %s as a UFUNCTION at runtime. The pawn "
					 "blueprints and the cheat manager call this surface by FName; a rename or a "
					 "dropped BlueprintCallable would break every blueprint that invokes it (the "
					 "bind is by name, not by symbol)."),
				FnName),
			Fn);
	}

	// The six BlueprintNativeEvent surfaces are also UFUNCTIONs (BNE wraps
	// them as reflected functions). The global message subsystem invokes
	// OnPawnReady / OnGameStateChanged via ProcessEvent on the cached
	// FName, so a rename here breaks the wiring even if the C++ binder is
	// unchanged.
	const TCHAR* ExpectedBNEs[] = {
		TEXT("OnPawnReady"),
		TEXT("OnGameStateChanged"),
		TEXT("OnPreRemovedFromLevel"),
		TEXT("OnControllerChanged"),
		TEXT("OnPostMove"),
		TEXT("OnMoveInputTriggered"),
		TEXT("OnMoveInputCompleted"),
	};
	for (const TCHAR* FnName : ExpectedBNEs)
	{
		UFunction* Fn = ComponentClass->FindFunctionByName(FName(FnName));
		TestNotNull(
			*FString::Printf(
				TEXT("UBmrMoverComponent must expose %s as a UFUNCTION (the BlueprintNativeEvent "
					 "surface). The global message subsystem and the MapComponent's pre-remove "
					 "broadcast resolve handlers by FName; dropping the BNE attribute or renaming "
					 "the function leaves those invokers unable to dispatch."),
				FnName),
			Fn);
	}

	// CachedSkatePowerupAttribute and CurrentMoveInput must be reflected
	// UPROPERTIES. The spec uses CurrentMoveInput as the carrier between
	// RequestMoveByIntent and ProduceInput; without the UPROPERTY flag the
	// field still works for C++ but is invisible to the debug visualisation
	// / blueprint inspection the spec's `VisibleInstanceOnly, BlueprintReadOnly`
	// header attributes imply.
	const FProperty* CachedSkateProp =
		ComponentClass->FindPropertyByName(FName(TEXT("CachedSkatePowerupAttribute")));
	TestNotNull(
		TEXT("UBmrMoverComponent must declare CachedSkatePowerupAttribute as a UPROPERTY — the "
			 "header's `VisibleInstanceOnly, BlueprintReadOnly` attributes are what surface the "
			 "cached value to the debug overlays the QA team uses to verify Skate state without "
			 "running a profiler. A field without the UPROPERTY flag is invisible there."),
		CachedSkateProp);

	const FProperty* MoveInputProp =
		ComponentClass->FindPropertyByName(FName(TEXT("CurrentMoveInput")));
	TestNotNull(
		TEXT("UBmrMoverComponent must declare CurrentMoveInput as a UPROPERTY — it is the carrier "
			 "between RequestMoveByIntent's writes (input pipeline, AI controller, cheat manager) "
			 "and ProduceInput's reads. Without reflection it cannot be inspected from blueprint "
			 "and the debug surface the header advertises is silently broken."),
		MoveInputProp);

	return true;
}

// ---------------------------------------------------------------------------
// Test 12 — B1 (runtime): IsBlockedMovement reports blocked when no ASC.
//
// Direct runtime behavioral discrimination, not source inspection. The
// canonical tag-based impl is:
//   const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::
//       GetAbilitySystemComponentFromActor(GetOwner());
//   return !ASC || ASC->HasMatchingGameplayTag(Block::Movement);
// On a free-standing component (no outer actor → GetOwner() == nullptr →
// ASC == nullptr), the !ASC clause short-circuits to TRUE — movement is
// reported as blocked, which is the safe default when GAS isn't wired.
//
// A bool-backed impl (the named B1 failure mode 'Boolean movement block —
// external tag adders diverge from the bool') returns its zero-initialized
// default of FALSE — the pawn is reported movable even though there is no
// system to gate it. Calling IsBlockedMovement() on a free-standing
// component directly discriminates the two encodings without needing a
// populated PIE world, the GlobalMessageSubsystem, or a registered ASC.
//
// Also probes SetBlockMovement crash-safety in the same context: the
// canonical guard '!ASC || bShouldBlock == IsBlockedMovement()' early-
// returns when ASC is null, so calls from contexts where the ASC has not
// yet been registered (cheat manager early bind, AI controller during
// possession) are safe no-ops. An impl that dereferences a null ASC
// would crash here, and an impl that flips a bool would diverge from
// IsBlockedMovement's continuing report of true. After both calls the
// no-ASC defensive default still applies because the !ASC clause still
// wins on the next IsBlockedMovement query.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoverComponent_IsBlockedMovementRuntimeDefaults,
	"Bomber.MoverComponent.IsBlockedMovementRuntimeDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoverComponent_IsBlockedMovementRuntimeDefaults::RunTest(const FString& Parameters)
{
	UBmrMoverComponent* Component = NewObject<UBmrMoverComponent>(GetTransientPackage());
	if (!TestNotNull(
			TEXT("UBmrMoverComponent must be constructible via NewObject — "
				 "BlueprintSpawnableComponent implies standalone allocation must work for the "
				 "pawn pool's reuse path and for component-level automation."),
			Component))
	{
		return false;
	}

	// Direct runtime observation, no source inspection. The canonical tag-
	// based impl short-circuits through '!ASC' to true when GAS is not
	// wired; a bool-backed impl returns its zero-initialized default of
	// false. This single observable behavior cleanly discriminates the
	// failure mode without needing a populated world.
	TestTrue(
		TEXT("B1 (runtime): IsBlockedMovement() on a component with no owner / no ASC must "
			 "return true. The canonical tag-based impl '!ASC || HasMatchingGameplayTag(Block::"
			 "Movement)' short-circuits via !ASC to true — the safe default since input "
			 "injection can't be reasoned about without the tag store. A bool-backed impl (the "
			 "named B1 failure mode 'Boolean movement block — external tag adders diverge from "
			 "the bool') returns its zero-initialized default of false. This default-state "
			 "observation directly discriminates the two encodings at runtime."),
		Component->IsBlockedMovement());

	// SetBlockMovement must be safe to invoke against a component with no
	// ASC — the canonical guard '!ASC || bShouldBlock == IsBlockedMovement()'
	// early-returns. An impl that dereferences a null ASC would crash here;
	// a bool-backed impl would flip its bool and IsBlockedMovement would
	// report false on the next read.
	Component->SetBlockMovement(false);
	Component->SetBlockMovement(true);
	TestTrue(
		TEXT("B1 (runtime): After SetBlockMovement(false) then SetBlockMovement(true) on a "
			 "component with no ASC, IsBlockedMovement() must still return true — the !ASC "
			 "clause keeps winning regardless of the toggled argument. A bool-backed impl "
			 "would have flipped its internal bool on the second call and would now disagree "
			 "with what the GAS-tag query would have reported (which is the named B1 "
			 "divergence). The cheat manager and AI controller both invoke this surface from "
			 "contexts where the ASC may not yet be registered; the canonical guard makes "
			 "those calls safe no-ops rather than crash or silent state drift."),
		Component->IsBlockedMovement());

	return true;
}
