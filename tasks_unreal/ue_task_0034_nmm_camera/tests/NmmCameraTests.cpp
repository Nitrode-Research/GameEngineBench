// Copyright (c) 2026 GameDevBench. Main-Menu camera-subsystem automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Plugins/.../NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMCameraSubsystem.cpp
//
// The subsystem owns the main-menu camera lifecycle:
//   * State-driven dispatch picks the right camera component (level, rail-rig,
//     or per-spot cinematic) for the current menu state.
//   * A blend-during-open flag prevents the rail rig from traversing while the
//     opening view-target blend is still running.
//   * A subscriber bound to UNMMSpotsSubsystem::OnActiveMenuSpotReady re-resolves
//     the view target when the cinematic spot finishes its background load.
//   * Two rail rigs (forward/backward) are picked by cycle direction so the
//     wrong-direction cycle does not visually play the cinematic backward.
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface needs a hydrated NewMainMenu
//   plugin (a Game-Feature plugin not loaded by default), the menu level
//   ("/Game/Bomber/Maps/MenuMain") with its placed BmrSkeletalMeshActor
//   spot owners, the rail-rig actors attached to each spot, a Data-Registry
//   row populated with FBmrCinematicRow soft-pointers for each character's
//   ULevelSequence asset, and the NMMBaseSubsystem driving menu-state
//   transitions via the GlobalMessageSubsystem. None of that exists as a
//   reusable PIE harness; reproducing it inside an automation test would
//   dwarf the surface under test. The NewMainMenu plugin is also not a
//   Bomber.Build.cs dependency, so we cannot include any of its headers
//   (FNmmStateTag, ENMMCameraRailTransitionState, UNMMCameraSubsystem,
//   UNMMSpotsSubsystem, UNMMBaseSubsystem) from a Source/Bomber/Tests/ file
//   without changing the module graph.
//
//   The spec and the evaluator notes pin down a small set of code-path
//   choices that separate a working implementation from a plausibly-working
//   one. Each test below targets one of the seven required behaviors (B1–B7
//   from the task brief). Where a behavior has multiple acceptable spellings,
//   the assertions accept the equivalences (`!MenuState.IsValid()` vs
//   `MenuState.IsValid() == false`, `.HasTag(...)` vs `.HasTagExact(...)`,
//   …) that would produce identical runtime behavior.

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogNmmCameraTests, Log, All);

namespace NmmCameraTests
{
	// The stripped file lives inside the NewMainMenu plugin (a Game Feature
	// plugin), not the Bomber game module. The header is unchanged in this
	// task; the tests inspect both so a re-impl that renames a method (and
	// breaks the API contract) is caught alongside body changes.
	static const TCHAR* CameraSubsystemRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Subsystems/NMMCameraSubsystem.cpp");
	static const TCHAR* CameraSubsystemHeaderRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Public/Subsystems/NMMCameraSubsystem.h");

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
	// canonical solution and many plausible re-impls reference "don't enumerate
	// one-by-one" or "no eager LoadSynchronous here" in comments). String
	// literals are preserved so TEXT("BasicMenu") inside a log call still
	// shows up.
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

	// Brace-depth walker to extract the body of a member-function definition.
	// Counts braces only, not parens, so nested initialiser lists / lambdas
	// (e.g. SetTimer's [&]{ … } captures) don't confuse the scan.
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
} // namespace NmmCameraTests

// ---------------------------------------------------------------------------
// Test 1 — B1: Non-menu / BasicMenu states dispatch to the level camera.
//
// "In states where the menu is not active (the player has started the game,
// or is between menu and game), the active camera is the level camera — the
// gameplay-level camera owner." A re-impl that handles only one specific
// exit transition leaves the cinematic camera as the view target on every
// other exit — the player-visible "stuck cinematic" after starting the game.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_BasicMenuAndInvalidDispatchToLevelCamera,
	"Bomber.NMMCamera.BasicMenuAndInvalidDispatchToLevelCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_BasicMenuAndInvalidDispatchToLevelCamera::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::FindCameraComponent"));
	if (!TestTrue(
			TEXT("B1: NMMCameraSubsystem.cpp must define FindCameraComponent with a body — "
				 "the start/ stub is empty; the state-dispatch table is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The body must call GetLevelCamera() — the gameplay-level camera owner
	// the spec names as the destination for non-menu / BasicMenu states. A
	// re-impl that synthesises its own camera (NewObject<UCameraComponent>)
	// or walks the world for any camera-bearing actor breaks the
	// "gameplay-level camera owner" contract (only the level's authored
	// camera has the PostProcess + view-rotation rig the HUD expects).
	TestTrue(
		TEXT("B1: FindCameraComponent must route the non-menu / BasicMenu / invalid-state "
			 "branch through UBmrBlueprintFunctionLibrary::GetLevelCamera(). The spec: 'when "
			 "the menu is not active, the active camera is the level camera — the gameplay-"
			 "level camera owner.' Returning a freshly-constructed component or any non-level "
			 "camera leaves the gameplay HUD pointed at the wrong rig."),
		CodeOnly.Contains(TEXT("GetLevelCamera"), ESearchCase::CaseSensitive));

	// The branch must trigger on BOTH BasicMenu AND the invalid/None case in
	// the same logical path. The spec calls this out: "A previous bug only
	// handled one specific exit transition and left the cinematic camera as
	// the view target on others." The canonical encoding tests
	// `!MenuState.IsValid() || MenuState == FNmmStateTag::BasicMenu` together.
	TestTrue(
		TEXT("B1: FindCameraComponent must handle FNmmStateTag::BasicMenu in CODE. The spec "
			 "calls BasicMenu the canonical 'menu-not-active' state — its dispatch must reach "
			 "the level-camera path, not the cinematic-camera path."),
		CodeOnly.Contains(TEXT("BasicMenu"), ESearchCase::CaseSensitive));

	const bool bHandlesInvalid =
		CodeOnly.Contains(TEXT("!MenuState.IsValid"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("!NewState.IsValid"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MenuState.IsValid() =="), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("IsValid() == false"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("FNmmStateTag::None"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("MenuState.IsValid()"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: FindCameraComponent must dispatch the invalid/None state to the level camera "
			 "as well, not just BasicMenu. The named regression — 'a previous bug only handled "
			 "one specific exit transition and left the cinematic camera as the view target on "
			 "others' — is exactly the failure mode of branching only on BasicMenu. The canonical "
			 "encoding is `!MenuState.IsValid() || MenuState == BasicMenu` in a single conditional "
			 "that both shapes share."),
		bHandlesInvalid);

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B2: Transition state dispatches to the rail-rig camera, and the
//          rail camera is the camera attached to the rig actor (not the
//          rig actor itself — that's the Quality-bar pattern the spec names).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_TransitionStateDispatchesToRailCamera,
	"Bomber.NMMCamera.TransitionStateDispatchesToRailCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_TransitionStateDispatchesToRailCamera::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString FindBody = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::FindCameraComponent"));
	if (!TestTrue(
			TEXT("B2: NMMCameraSubsystem.cpp must define FindCameraComponent with a body."),
			!FindBody.IsEmpty()))
	{
		return false;
	}

	const FString FindCode = StripComments(FindBody);

	TestTrue(
		TEXT("B2: FindCameraComponent's Transition branch must reach GetCurrentRailCamera() — "
			 "the helper that returns the CameraActor attached to the rail rig. The spec's "
			 "Quality bar: 'the rail-rig camera is the camera attached to the active rig "
			 "actor, not the rig actor itself'. A branch that returns the rig actor's own "
			 "camera component dereferences nullptr on the very first cycle."),
		FindCode.Contains(TEXT("GetCurrentRailCamera"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: FindCameraComponent's Transition branch must handle FNmmStateTag::Transition "
			 "in CODE. A body that mentions Transition only in a comment can't possibly route "
			 "it to the rail camera, and the carousel cuts between slots instead of blending."),
		FindCode.Contains(TEXT("Transition"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: FindCameraComponent's Transition branch must extract the rail camera's "
			 "UCameraComponent (e.g. `RailCamera->GetCameraComponent()`). The function "
			 "returns UCameraComponent*; reaching for a different attribute (RootComponent "
			 "cast) loses the camera-cut binding the engine uses for cinematic cameras."),
		FindCode.Contains(TEXT("GetCameraComponent"), ESearchCase::CaseSensitive));

	// GetCurrentRailCamera itself must walk the rail-rig attach pattern —
	// UGameplayUtilsLibrary::GetAttachedActorByClass<ACameraActor>(rig).
	// Without the template specialisation, the helper returns nullptr and
	// the Transition branch fails the ensureMsgf in PossessCamera every cycle.
	const FString GetRailCameraBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::GetCurrentRailCamera"));
	const FString GetRailCameraCode = StripComments(GetRailCameraBody);
	TestTrue(
		TEXT("B2: GetCurrentRailCamera must walk the rail-rig attach pattern — "
			 "GetAttachedActorByClass<ACameraActor>(GetCurrentRailRig()) — not the spot directly. "
			 "Quality bar: 'the rail-rig camera is the camera attached to the active rig actor, "
			 "not to the spot directly'. A body that reaches for the spot's owner first finds "
			 "the spot's own preview camera instead of the rail's filmed-traversal camera."),
		GetRailCameraCode.Contains(TEXT("GetAttachedActorByClass"), ESearchCase::CaseSensitive)
			&& GetRailCameraCode.Contains(TEXT("ACameraActor"), ESearchCase::CaseSensitive)
			&& GetRailCameraCode.Contains(TEXT("GetCurrentRailRig"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B3: Idle / Cinematic states dispatch to the spot's cinematic
//          camera extracted from the slot's sequence player.
//
// The spec: "In idle/cinematic states (a single slot is currently active
// and its cinematic is playing), the active camera is the active slot's
// cinematic camera, extracted from the slot's sequence player." The
// canonical wiring walks `SpotsSubsystem.GetCurrentSpot() → GetMasterPlayer()
// → UCinematicUtils::FindSequenceCameraComponent(MasterPlayer)`.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_IdleAndCinematicDispatchToSequenceCamera,
	"Bomber.NMMCamera.IdleAndCinematicDispatchToSequenceCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_IdleAndCinematicDispatchToSequenceCamera::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::FindCameraComponent"));
	if (!TestTrue(
			TEXT("B3: NMMCameraSubsystem.cpp must define FindCameraComponent with a body."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B3: FindCameraComponent's idle / cinematic branch must mention both Idle and "
			 "Cinematic in CODE. The spec groups them: 'a single slot is currently active "
			 "and its cinematic is playing'; routing them to different camera paths splits "
			 "the dispatch where the spec joins it."),
		CodeOnly.Contains(TEXT("Idle"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("Cinematic"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: FindCameraComponent's idle / cinematic branch must read the current spot's "
			 "GetMasterPlayer() — the ULevelSequencePlayer the spot caches once the cinematic's "
			 "soft pointer resolves. Reaching for the ULevelSequence asset directly bypasses "
			 "the player's current camera-cut binding and freezes on the asset's default frame."),
		CodeOnly.Contains(TEXT("GetMasterPlayer"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: FindCameraComponent's idle / cinematic branch must reach the active spot "
			 "through GetCurrentSpot() on the spots subsystem — the single source of truth for "
			 "'which slot is the player currently viewing'. Any other discovery path (world "
			 "iterator, FindObjectFast) can return the wrong slot during transition windows "
			 "where multiple spots are alive in the menu level."),
		CodeOnly.Contains(TEXT("GetCurrentSpot"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B3: FindCameraComponent's idle / cinematic branch must extract the camera through "
			 "UCinematicUtils::FindSequenceCameraComponent (or an equivalent helper that "
			 "resolves the sequencer's current camera-cut). A body that reads a cached camera "
			 "attribute off the player can pick up a stale binding from a previous take."),
		CodeOnly.Contains(TEXT("FindSequenceCameraComponent"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B4: Dispatch uses tag containment, not enumeration of
//          exit-from-cinematic transitions.
//
// The spec's load-bearing point: "The dispatch must route every non-menu,
// non-transition state through the level-camera lookup, not enumerate
// states one-by-one." The canonical shape uses `(Idle | Cinematic).HasTag(
// MenuState)` containment for the cinematic-camera branch and falls through
// to the level-camera path for everything else; a re-impl that branches on
// `from-state → to-state` pairs is the original regression.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_DispatchUsesTagContainmentNotEnumeration,
	"Bomber.NMMCamera.DispatchUsesTagContainmentNotEnumeration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_DispatchUsesTagContainmentNotEnumeration::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::FindCameraComponent"));
	if (!TestTrue(
			TEXT("B4: NMMCameraSubsystem.cpp must define FindCameraComponent with a body."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	const bool bUsesTagContainment =
		CodeOnly.Contains(TEXT(".HasTag("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".HasTagExact("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".HasAny("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".MatchesAny("), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT(".HasAnyExact("), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B4: FindCameraComponent must use tag-containment (`.HasTag(...)`, `.HasAny(...)`, "
			 "or equivalent FGameplayTagContainer membership test) to group the Idle/Cinematic "
			 "states. Equality-chains (`MenuState == Idle || MenuState == Cinematic`) are "
			 "functionally identical for today's state set but the spec calls out the failure "
			 "mode explicitly: 'Any new menu state added later breaks the dispatch.' The "
			 "containment idiom keeps the cinematic-like family extensible — a new tag added "
			 "under the Idle/Cinematic umbrella participates automatically."),
		bUsesTagContainment);

	TestTrue(
		TEXT("B4: FindCameraComponent must combine FNmmStateTag::Idle and FNmmStateTag::Cinematic "
			 "into a single container (e.g. `Idle | Cinematic`) before the containment test. "
			 "Two separate `.HasTag()` calls would defeat containment — they still enumerate "
			 "the family member-by-member."),
		CodeOnly.Contains(TEXT("Idle | "), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("| FNmmStateTag::Cinematic"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("| FNmmStateTag::Idle"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("Idle | Cinematic"), ESearchCase::CaseSensitive));

	// Negative: the dispatch must NOT branch on a from-state / to-state pair
	// the way the original regression did. The named anti-pattern is
	// `if (CurrentState == Cinematic && NewState == BasicMenu) → level cam`;
	// that shape catches only the cinematic-to-BasicMenu transition and leaves
	// every other cinematic-exit stuck on the cinematic camera. The dispatch
	// is purely `f(MenuState) → camera`; it doesn't read a "from" state.
	// (OnActiveMenuSpotReady_Implementation legitimately consults the current
	// state, but FindCameraComponent itself must not.)
	const bool bReadsCurrentState =
		CodeOnly.Contains(TEXT("GetCurrentMenuState"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("CurrentMenuState"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B4: FindCameraComponent must NOT consult the current menu state when dispatching. "
			 "The function takes the target MenuState as a parameter; the dispatch is purely "
			 "`f(MenuState) → camera`. Branching on `(from, to)` pairs is the exact regression "
			 "the spec calls out: 'A previous bug only handled one specific exit transition'."),
		bReadsCurrentState);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B5: Two rail rigs (forward and backward); GetCurrentRailRig picks
//          the right one by cycle direction.
//
// The spec: "There are two rail rigs — one for forward cycles, one for
// backward — selected by cycle direction." The canonical encoding:
//     IsCameraForwardTransition()
//         ? SpotsSubsystem.GetCurrentSpot()
//         : SpotsSubsystem.GetNextSpot(/*ForwardDir=*/1);
// A re-impl that always uses `GetCurrentSpot()` (ignoring direction) picks
// the forward rig for the backward cycle — the spec's named failure mode:
// "the wrong-direction cycle visually plays the cinematic backward."
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_RailRigSelectedByCycleDirection,
	"Bomber.NMMCamera.RailRigSelectedByCycleDirection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_RailRigSelectedByCycleDirection::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::GetCurrentRailRig"));
	if (!TestTrue(
			TEXT("B5: NMMCameraSubsystem.cpp must define GetCurrentRailRig with a body — the "
				 "start/ stub is empty; the direction-aware rig selection is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	const bool bConsultsDirection =
		CodeOnly.Contains(TEXT("IsCameraForwardTransition"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetLastMoveSpotDirection"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B5: GetCurrentRailRig must consult the cycle direction (IsCameraForwardTransition() "
			 "or GetLastMoveSpotDirection()). A body that always returns the rig attached to the "
			 "current spot is the spec's named failure mode: 'Using one rail rig for both cycle "
			 "directions' produces a backward cycle that visually plays the rail traversal in "
			 "the wrong direction."),
		bConsultsDirection);

	TestTrue(
		TEXT("B5: GetCurrentRailRig must reach GetNextSpot(...) for the backward branch. The two "
			 "rails are attached to different spot owners; using GetCurrentSpot for both directions "
			 "collapses the two-rig design back to one and the backward cycle plays the rail in "
			 "reverse — the spec's exact 'wrong-direction cycle visually plays the cinematic "
			 "backward' failure mode."),
		CodeOnly.Contains(TEXT("GetNextSpot"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B5: GetCurrentRailRig must reach GetCurrentSpot for the forward branch."),
		CodeOnly.Contains(TEXT("GetCurrentSpot"), ESearchCase::CaseSensitive));

	// The selected spot must be walked to find the attached ACineCameraRigRail
	// — the rig actor is attached to the *spot owner* (the BmrSkeletalMeshActor
	// placed in the menu level), not to the spot component directly.
	TestTrue(
		TEXT("B5: GetCurrentRailRig must walk the spot's owner to find the attached rail rig "
			 "(GetAttachedActorByClass<ACineCameraRigRail>(MenuSpot->GetOwner())). The rig is "
			 "attached to the spot-owning actor in the level, not to the spot component itself."),
		CodeOnly.Contains(TEXT("GetAttachedActorByClass"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("CineCameraRigRail"), ESearchCase::CaseSensitive));

	// Sanity: IsCameraForwardTransition reads GetLastMoveSpotDirection > 0.
	// A re-impl that inverts the comparison swaps the rigs.
	const FString IsFwdBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::IsCameraForwardTransition"));
	const FString IsFwdCode = StripComments(IsFwdBody);
	TestTrue(
		TEXT("B5: IsCameraForwardTransition must read GetLastMoveSpotDirection() and compare it "
			 "against zero. The spots-subsystem records the sign of the last cycle (+1 forward, "
			 "-1 backward); the camera-direction predicate is a single `> 0.f` test. A re-impl "
			 "that inverts the comparison swaps the rigs."),
		IsFwdCode.Contains(TEXT("GetLastMoveSpotDirection"), ESearchCase::CaseSensitive)
			&& IsFwdCode.Contains(TEXT("> 0"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B6: Blend-during-open guard. The rail rig must not traverse
//          while the opening view-target blend is still running.
//
// The spec: "While that blend is running, the rail rig must not start
// traversing — otherwise the blend-in and the rail traversal both
// interpolate the view at the same time and the camera judders." The
// mechanism is a flag (`bIsBlendingInOut`) set when the blend starts and
// cleared by a timer when the blend completes; Tick reads the flag and
// holds rail progression while it's set.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_TickGuardedByBlendInOutFlag,
	"Bomber.NMMCamera.TickGuardedByBlendInOutFlag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_TickGuardedByBlendInOutFlag::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// Tick must guard the rail-progress call with !bIsBlendingInOut.
	const FString TickBody = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::Tick"));
	if (!TestTrue(
			TEXT("B6: NMMCameraSubsystem.cpp must define Tick with a body."),
			!TickBody.IsEmpty()))
	{
		return false;
	}

	const FString TickCode = StripComments(TickBody);

	TestTrue(
		TEXT("B6: Tick must guard the rail-progress call with !bIsBlendingInOut. The spec: "
			 "'while that blend is running, the rail rig must not start traversing — otherwise "
			 "the blend-in and the rail traversal both interpolate the view at the same time "
			 "and the camera judders.' A Tick that calls TickTransition unconditionally produces "
			 "the visible judder at menu entry."),
		TickCode.Contains(TEXT("!bIsBlendingInOut"), ESearchCase::CaseSensitive)
			|| TickCode.Contains(TEXT("bIsBlendingInOut == false"), ESearchCase::CaseSensitive)
			|| TickCode.Contains(TEXT("false == bIsBlendingInOut"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B6: Tick must invoke TickTransition once the guard passes — without that call, "
			 "the rail never advances and the carousel sits frozen at its start position."),
		TickCode.Contains(TEXT("TickTransition"), ESearchCase::CaseSensitive));

	// OnBeginTransition must SET the flag — that's the open-blend window.
	const FString BeginBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnBeginTransition"));
	if (!TestTrue(
			TEXT("B6: NMMCameraSubsystem.cpp must define OnBeginTransition with a body."),
			!BeginBody.IsEmpty()))
	{
		return false;
	}

	const FString BeginCode = StripComments(BeginBody);

	TestTrue(
		TEXT("B6: OnBeginTransition must set bIsBlendingInOut = true at the start of the rail "
			 "blend window. Without the set, Tick's `!bIsBlendingInOut` guard is perpetually "
			 "true and the rail traverses during the open blend — the exact judder the spec "
			 "is preventing."),
		BeginCode.Contains(TEXT("bIsBlendingInOut = true"), ESearchCase::CaseSensitive)
			|| BeginCode.Contains(TEXT("bIsBlendingInOut=true"), ESearchCase::CaseSensitive));

	// The flag must be cleared on a timer — not synchronously inside
	// OnBeginTransition (which would leave the guard useless) and not in
	// Tick (which races against the rail-progress call). The canonical
	// encoding uses GetTimerManager().SetTimer(...) with a lambda that flips
	// `bIsBlendingInOut = false` after the blend duration.
	TestTrue(
		TEXT("B6: OnBeginTransition must clear bIsBlendingInOut on a TIMER (SetTimer with a "
			 "lambda or delegate). Clearing it synchronously in the same function would make "
			 "the guard a no-op; clearing it in Tick would race against TickTransition. The "
			 "spec's mechanism is explicit: 'a flag set when the blend starts, cleared when "
			 "the blend completes (via a timer)'."),
		BeginCode.Contains(TEXT("SetTimer"), ESearchCase::CaseSensitive)
			&& BeginCode.Contains(TEXT("bIsBlendingInOut = false"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B6: OnBeginTransition's timer must use GetCameraBlendTime() (from the NMM data "
			 "asset) as its duration. A hard-coded constant decouples the guard from the actual "
			 "blend the engine is performing, so the rail either still judders (timer shorter "
			 "than the blend) or stalls visibly (timer longer)."),
		BeginCode.Contains(TEXT("GetCameraBlendTime"), ESearchCase::CaseSensitive));

	// OnEndTransition's symmetric blend (rail-end → idle-spot handoff) must
	// use the same guard mechanism so the closing blend doesn't race against
	// a stray TickTransition.
	const FString EndBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnEndTransition"));
	const FString EndCode = StripComments(EndBody);
	TestTrue(
		TEXT("B6: OnEndTransition must re-arm bIsBlendingInOut for the closing blend (rail end "
			 "→ idle spot). The closing blend has the same judder potential as the opening "
			 "blend: if Tick continues to advance the rail while the camera blends to the spot "
			 "pose, the player sees a second judder. The canonical body sets the flag, blends, "
			 "and clears the flag on a timer of length GetCameraBlendTime()."),
		EndCode.Contains(TEXT("bIsBlendingInOut = true"), ESearchCase::CaseSensitive)
			&& EndCode.Contains(TEXT("SetTimer"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — B7: View-target re-resolution on slot-ready signals, not only
//          on menu-state changes.
//
// The spec: "When the active slot becomes ready (signalled by the spots
// subsystem), the camera subsystem re-resolves the view target. This is
// what handles the slot-load-completed-after-state-change case: the state
// change came first, the cinematic loaded later, the re-resolve picks up
// the now-ready cinematic camera." The wiring:
//   * OnGameFeatureInitialize subscribes to UNMMSpotsSubsystem::OnActiveMenuSpotReady
//   * OnActiveMenuSpotReady_Implementation possesses the camera when the
//     current state is Idle or Cinematic
//   * OnGameFeatureDeinitialize unsubscribes (RemoveAll(this))
//   * OnNewMainMenuStateChanged_Implementation does NOT call PossessCamera
//     directly for Cinematic — that path is deferred to the slot-ready
//     callback so it does not crash on a half-loaded slot.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_SubscribesToSlotReadyAndDefersIdlePossession,
	"Bomber.NMMCamera.SubscribesToSlotReadyAndDefersIdlePossession",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_SubscribesToSlotReadyAndDefersIdlePossession::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// OnGameFeatureInitialize must subscribe to OnActiveMenuSpotReady.
	const FString InitBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnGameFeatureInitialize_Implementation"));
	if (!TestTrue(
			TEXT("B7: NMMCameraSubsystem.cpp must define OnGameFeatureInitialize_Implementation "
				 "with a body — the start/ stub is empty; the subscription wiring is what was "
				 "stripped."),
			!InitBody.IsEmpty()))
	{
		return false;
	}

	const FString InitCode = StripComments(InitBody);

	TestTrue(
		TEXT("B7: OnGameFeatureInitialize must subscribe to UNMMSpotsSubsystem::"
			 "OnActiveMenuSpotReady (the slot-load completion signal). The spec: 'the camera "
			 "subsystem re-resolves the view target when the active slot becomes ready'. "
			 "Without the subscription, the slot-load-completed-after-state-change race never "
			 "resolves — the cinematic loads but the camera stays on whatever target the "
			 "previous state had."),
		InitCode.Contains(TEXT("OnActiveMenuSpotReady"), ESearchCase::CaseSensitive)
			&& (InitCode.Contains(TEXT("AddUniqueDynamic"), ESearchCase::CaseSensitive)
				|| InitCode.Contains(TEXT("AddDynamic"), ESearchCase::CaseSensitive)));

	// OnGameFeatureInitialize must also start listening for the menu-state-
	// changed global message — the other half of the dispatch. Without the
	// global-message listener, state changes never reach the camera at all
	// (the slot-ready callback alone covers only one of the two race orders).
	TestTrue(
		TEXT("B7: OnGameFeatureInitialize must listen for the MenuStateChanged global message "
			 "(via UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage or equivalent). "
			 "The state-changed path AND the slot-ready path together cover both race orders; "
			 "a re-impl that wires only one leaves half the state graph stuck."),
		InitCode.Contains(TEXT("MenuStateChanged"), ESearchCase::CaseSensitive)
			&& (InitCode.Contains(TEXT("CallOrStartListeningForGlobalMessage"), ESearchCase::CaseSensitive)
				|| InitCode.Contains(TEXT("StartListeningForGlobalMessage"), ESearchCase::CaseSensitive)));

	// The slot-ready callback must call PossessCamera only when the current
	// state is Idle or Cinematic. The spec's contract: 'the re-resolve picks
	// up the now-ready cinematic camera' — the slot-ready event is meaningful
	// only when the menu is showing a cinematic slot.
	const FString OnReadyBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnActiveMenuSpotReady_Implementation"));
	if (!TestTrue(
			TEXT("B7: NMMCameraSubsystem.cpp must define OnActiveMenuSpotReady_Implementation "
				 "with a body — that is the subscriber the spec names."),
			!OnReadyBody.IsEmpty()))
	{
		return false;
	}

	const FString OnReadyCode = StripComments(OnReadyBody);

	TestTrue(
		TEXT("B7: OnActiveMenuSpotReady_Implementation must call PossessCamera. The whole "
			 "point of subscribing is to re-resolve the view target — a callback that only "
			 "logs or that mutates internal state without ever possessing the camera leaves "
			 "the cinematic slot loaded but the view target stuck on its previous owner."),
		OnReadyCode.Contains(TEXT("PossessCamera"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: OnActiveMenuSpotReady_Implementation must consult the current menu state "
			 "and only possess for Idle / Cinematic. Possessing unconditionally on slot-ready "
			 "clobbers an in-flight Transition (which legitimately holds the rail-rig camera) "
			 "and races a Game-Started state change. The canonical body reads "
			 "GetCurrentMenuState() and tag-tests the result."),
		(OnReadyCode.Contains(TEXT("Idle"), ESearchCase::CaseSensitive)
			|| OnReadyCode.Contains(TEXT("Cinematic"), ESearchCase::CaseSensitive))
			&& OnReadyCode.Contains(TEXT("GetCurrentMenuState"), ESearchCase::CaseSensitive));

	// The state-change handler must trigger OnBeginTransition for Transition,
	// possess for BasicMenu/invalid, and must NOT call PossessCamera directly
	// for Cinematic — that path is deferred to OnActiveMenuSpotReady so the
	// spot is guaranteed to be loaded before possession.
	const FString OnStateBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnNewMainMenuStateChanged_Implementation"));
	if (!TestTrue(
			TEXT("B7: NMMCameraSubsystem.cpp must define OnNewMainMenuStateChanged_Implementation "
				 "with a body."),
			!OnStateBody.IsEmpty()))
	{
		return false;
	}

	const FString OnStateCode = StripComments(OnStateBody);

	TestTrue(
		TEXT("B7: OnNewMainMenuStateChanged must trigger OnBeginTransition for the Transition "
			 "state. The Transition entry is the one state where the camera-subsystem state "
			 "machine drives the rail rig itself; missing this branch leaves cycle attempts "
			 "with no rail movement at all."),
		OnStateCode.Contains(TEXT("OnBeginTransition"), ESearchCase::CaseSensitive)
			&& OnStateCode.Contains(TEXT("Transition"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: OnNewMainMenuStateChanged must call PossessCamera for BasicMenu / invalid "
			 "(the level-camera path is safe to call synchronously; no spot needs to be loaded)."),
		OnStateCode.Contains(TEXT("PossessCamera"), ESearchCase::CaseSensitive)
			&& OnStateCode.Contains(TEXT("BasicMenu"), ESearchCase::CaseSensitive));

	const bool bDirectCinematicPossess =
		OnStateCode.Contains(TEXT("PossessCamera(FNmmStateTag::Cinematic"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B7: OnNewMainMenuStateChanged must NOT directly call "
			 "PossessCamera(FNmmStateTag::Cinematic). The Cinematic state's possession is "
			 "deferred to OnActiveMenuSpotReady because the slot may not yet have a master "
			 "player when the state arrives. Calling PossessCamera directly causes the inner "
			 "FindCameraComponent → MasterPlayer dereference to walk a nullptr during the "
			 "async-load window. The slot-ready subscriber is the canonical resolver."),
		bDirectCinematicPossess);

	// OnGameFeatureDeinitialize must unsubscribe — leaking the binding past
	// plugin teardown produces a stale dispatch the next time the plugin
	// is re-activated.
	const FString DeinitBody =
		ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::OnGameFeatureDeinitialize_Implementation"));
	const FString DeinitCode = StripComments(DeinitBody);
	TestTrue(
		TEXT("B7: OnGameFeatureDeinitialize must unsubscribe from OnActiveMenuSpotReady — the "
			 "binding must not survive plugin teardown. The canonical encoding is "
			 "`SpotsSubsystem->OnActiveMenuSpotReady.RemoveAll(this)`."),
		DeinitCode.Contains(TEXT("OnActiveMenuSpotReady"), ESearchCase::CaseSensitive)
			&& DeinitCode.Contains(TEXT("RemoveAll"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — B7 (partner): PossessCamera defers possession until the spot
//          is ready.
//
// The api_surface pins the contract of PossessCamera's first lines: it
// must early-out when SpotsSubsystem.IsActiveMenuSpotReady() is false AND
// the requested state is neither BasicMenu nor invalid. This is the partner
// of the slot-ready subscription — the subscription handles the case where
// the state arrived first; the early-out handles the symmetric race where
// the state-change handler tried to possess before the spot finished its
// background load. Without the guard, FindCameraComponent's idle/cinematic
// branch dereferences a null MasterPlayer.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_PossessCameraDefersUntilSpotReady,
	"Bomber.NMMCamera.PossessCameraDefersUntilSpotReady",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_PossessCameraDefersUntilSpotReady::RunTest(const FString& Parameters)
{
	using namespace NmmCameraTests;

	const FString Source = LoadProjectFile(this, CameraSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMCameraSubsystem::PossessCamera"));
	if (!TestTrue(
			TEXT("B7: NMMCameraSubsystem.cpp must define PossessCamera with a body — the "
				 "start/ stub is empty; the deferred-possession guard is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B7: PossessCamera must consult UNMMSpotsSubsystem::IsActiveMenuSpotReady() before "
			 "forwarding the request to FindCameraComponent. Without the guard, an Idle/"
			 "Cinematic state that arrives during the async-load window walks a nullptr inside "
			 "FindSequenceCameraComponent. The slot-ready subscriber will pick the request back "
			 "up once the spot finishes loading; the deferred-possession early-out is what "
			 "makes the subscriber the authoritative path."),
		CodeOnly.Contains(TEXT("IsActiveMenuSpotReady"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: PossessCamera's deferred-possession guard must let BasicMenu and invalid "
			 "states through (they target the level camera, which doesn't need a spot). The "
			 "guard reads `!IsActiveMenuSpotReady && MenuState != BasicMenu && IsValid()`; a "
			 "guard that blocks BasicMenu too would prevent the gameplay-camera handoff from "
			 "ever running on a cold-start menu open."),
		CodeOnly.Contains(TEXT("BasicMenu"), ESearchCase::CaseSensitive)
			&& (CodeOnly.Contains(TEXT("MenuState.IsValid"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("NewState.IsValid"), ESearchCase::CaseSensitive)));

	// The body must also short-circuit when the requested camera owner is
	// already the view target — `MyPC->GetViewTarget() == CameraComponent->
	// GetOwner()`. Without this, every state-change repossesses (re-blends)
	// the same camera, producing a visible double-blend on slot-ready
	// callbacks that fire after the state-change handler already possessed.
	TestTrue(
		TEXT("B7: PossessCamera must short-circuit when the requested camera owner is already "
			 "the view target (GetViewTarget() == CameraComponent->GetOwner()). Without this, "
			 "the slot-ready re-resolution and the state-change possession can each blend the "
			 "same camera in turn, producing a visible double-blend."),
		CodeOnly.Contains(TEXT("GetViewTarget"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B7: PossessCamera must call MyPC->SetViewTarget(...) once its guards pass. A "
			 "body that only short-circuits and never possesses leaves the view target stuck "
			 "on whatever the previous state had."),
		CodeOnly.Contains(TEXT("SetViewTarget"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Runtime UClass surface: the plugin's reflected class is registered,
//          the dispatch UFUNCTION has the contracted signature, and the
//          blend-guard UPROPERTY exists with the contracted bool type.
//
// The NewMainMenu plugin's module loads on engine startup (it's listed in
// Bomber.uproject with EnabledByDefault=true; only its Game-Feature
// *activation* is deferred via ExplicitlyLoaded=true). That means the
// plugin's UClass and its UFUNCTION/UPROPERTY tables are registered in the
// engine's reflection database before any test runs, even though the
// Bomber.Build.cs PrivateDependencyModuleNames list does not name the plugin
// (so we still cannot include the plugin's headers).
//
// This test reaches the dispatch via the engine's reflection system instead
// of via source-text scanning. That gives us a real runtime hook into the
// post-compile state of the class:
//   * The class is registered under /Script/NewMainMenu.NMMCameraSubsystem
//     (catches a re-impl that renames the class or moves it out of the
//     plugin's runtime module).
//   * FindCameraComponent UFUNCTION exists with the right return shape
//     (UCameraComponent*) — a re-impl that changes the return to e.g.
//     ACameraActor* would compile-pass against callers that void-cast and
//     silently regress the dispatch contract this group exists to test.
//   * bIsBlendingInOut UPROPERTY exists and is a bool — a re-impl that
//     changes the type to an int counter or removes the property would
//     break the Tick guard at runtime (the spec's named failure mode for
//     B6 in the open-blend judder).
//   * OnCameraRailTransitionStateChanged is a multicast-delegate UPROPERTY
//     — the local-broadcast surface SetNewCameraRailTransitionState fires
//     into; removing it disconnects the rail-progress side-channel from
//     listeners (HalfwayTransition / EndTransition states never reach UMG
//     vending logic).
//
// All checks degrade gracefully when the plugin module is not registered in
// the test process (some CI configurations strip Game-Feature plugins out
// of the editor target). In that mode the source-inspection tests above
// continue to enforce the same contract structurally.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmCamera_RuntimeUClassSurface,
	"Bomber.NMMCamera.RuntimeUClassSurface",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmCamera_RuntimeUClassSurface::RunTest(const FString& Parameters)
{
	// Reflect into the plugin's class via the engine reflection database.
	// FindObject<UClass> is the canonical lookup and does not require
	// including the plugin's header (we never reference the UClass type
	// directly — we only inspect its UFunctions / FProperties).
	UClass* CameraSubsystemClass = FindObject<UClass>(nullptr, TEXT("/Script/NewMainMenu.NMMCameraSubsystem"));
	if (!CameraSubsystemClass)
	{
		// Plugin's runtime module is not registered in this process. The
		// source-inspection tests above already enforce the same dispatch
		// and blend-guard contract structurally; we pass here so the test
		// suite is well-defined across CI configurations that strip the
		// plugin out of the editor target. The full set of source-level
		// assertions still has to pass — this test is purely additive.
		AddInfo(TEXT("NewMainMenu plugin's runtime module is not registered in this process — "
					 "runtime UClass surface check is skipped. The source-inspection tests "
					 "above continue to enforce the same contract."));
		return true;
	}

	// G1: FindCameraComponent must return a UCameraComponent* (the dispatch's
	// declared return type). A re-impl that changes the return to ACameraActor*
	// or void would compile-pass against any caller that casts/discards the
	// value but silently break the state-driven dispatch.
	UFunction* FindCameraComponentFn =
		CameraSubsystemClass->FindFunctionByName(FName(TEXT("FindCameraComponent")));
	if (TestNotNull(
			TEXT("G1/B1-B3: UNMMCameraSubsystem must expose the FindCameraComponent UFUNCTION at "
				 "runtime — that is the single dispatch entry the state-driven contract requires."),
			FindCameraComponentFn))
	{
		FProperty* ReturnProp = FindCameraComponentFn->GetReturnProperty();
		if (TestNotNull(
				TEXT("G1: FindCameraComponent must declare a return value (UCameraComponent*) — "
					 "the dispatch returns the camera component for the active state."),
				ReturnProp))
		{
			const FObjectProperty* ObjReturn = CastField<FObjectProperty>(ReturnProp);
			if (TestNotNull(
					TEXT("G1: FindCameraComponent's return must be a UObject-derived pointer "
						 "(UCameraComponent*). A non-object return breaks every caller that "
						 "feeds it into PlayerController::SetViewTarget."),
					ObjReturn))
			{
				const UClass* ReturnClass = ObjReturn->PropertyClass;
				const bool bIsCameraComponentReturn =
					ReturnClass
					&& (ReturnClass->GetFName() == FName(TEXT("CameraComponent"))
						|| ReturnClass->IsChildOf(FindObject<UClass>(nullptr, TEXT("/Script/Engine.CameraComponent"))));
				TestTrue(
					TEXT("G1: FindCameraComponent's return type must be (or derive from) "
						 "UCameraComponent. A different camera type (ACameraActor*, USceneComponent*) "
						 "loses the camera-cut binding the engine expects."),
					bIsCameraComponentReturn);
			}
		}

		// The static call modifier is part of the contract — the spec names
		// FindCameraComponent as a stateless lookup, not an instance call.
		TestTrue(
			TEXT("G1: FindCameraComponent must be a static UFUNCTION — the dispatch is "
				 "stateless `f(MenuState) -> camera`. An instance-bound dispatch would "
				 "couple it to a particular subsystem instance and break the static "
				 "callers (BP function-library style) in the rest of the menu pipeline."),
			(FindCameraComponentFn->FunctionFlags & FUNC_Static) != 0);
	}

	// G2/B6: the blend-during-open guard's mechanism — bIsBlendingInOut — must
	// exist as a bool UPROPERTY on the class. A re-impl that drops or
	// re-types this field defeats the Tick guard at runtime (the spec's
	// failure mode for B6 — visible judder at menu entry).
	const FBoolProperty* BlendingFlag = CastField<FBoolProperty>(
		CameraSubsystemClass->FindPropertyByName(FName(TEXT("bIsBlendingInOut"))));
	TestNotNull(
		TEXT("G2/B6: UNMMCameraSubsystem must declare a `bool bIsBlendingInOut` UPROPERTY — "
			 "that is the blend-during-open flag Tick reads to hold rail progression. A "
			 "missing or wrongly-typed field makes the source-level guard a no-op at "
			 "runtime; the rail traverses during the open blend and the player sees "
			 "the named camera judder at menu entry."),
		BlendingFlag);

	// G2/B5: the rail-direction side-channel — OnCameraRailTransitionStateChanged
	// must be a multicast-delegate UPROPERTY so SetNewCameraRailTransitionState
	// can broadcast halfway/end notifications to listeners (UMG, audio cue
	// subsystem). A re-impl that drops it disconnects the rail-progress
	// observation from the rest of the menu.
	const FMulticastDelegateProperty* RailStateDelegate = CastField<FMulticastDelegateProperty>(
		CameraSubsystemClass->FindPropertyByName(FName(TEXT("OnCameraRailTransitionStateChanged"))));
	TestNotNull(
		TEXT("G2: UNMMCameraSubsystem must declare OnCameraRailTransitionStateChanged as a "
			 "multicast-delegate UPROPERTY — SetNewCameraRailTransitionState fires into it on "
			 "every actual rail-state change. Removing the delegate (or making it a non-"
			 "multicast UFUNCTION) breaks the local-broadcast contract the spec names."),
		RailStateDelegate);

	// G2/B5 + G1/B2: the direction-aware rail-rig helper must exist as a
	// static UFUNCTION (the spec encodes it as a BlueprintCallable/Pure on
	// the class). Without it the dispatch can't pick the right rig per
	// cycle direction.
	UFunction* GetCurrentRailRigFn =
		CameraSubsystemClass->FindFunctionByName(FName(TEXT("GetCurrentRailRig")));
	if (TestNotNull(
			TEXT("G2/B5: UNMMCameraSubsystem must expose GetCurrentRailRig as a UFUNCTION at "
				 "runtime — the direction-aware rail-rig selector. Without it the dispatch "
				 "would have to hard-code one rig, which is the spec's named failure mode "
				 "(`Using one rail rig for both cycle directions`)."),
			GetCurrentRailRigFn))
	{
		FProperty* RailReturn = GetCurrentRailRigFn->GetReturnProperty();
		const FObjectProperty* ObjReturn = CastField<FObjectProperty>(RailReturn);
		const bool bReturnsRail =
			ObjReturn
			&& ObjReturn->PropertyClass
			&& ObjReturn->PropertyClass->GetFName().ToString().Contains(TEXT("CineCameraRigRail"));
		TestTrue(
			TEXT("G2/B5: GetCurrentRailRig must return ACineCameraRigRail* — the rail-rig "
				 "actor whose traversal produces the filmed transition. A different return "
				 "type silently breaks the rail-driving code paths in OnBeginTransition / "
				 "TickTransition that expect a manual-drive ACineCameraRigRail."),
			bReturnsRail);
	}

	// G2/B7: OnActiveMenuSpotReady must be a UFUNCTION (BlueprintNativeEvent on
	// the header) — that is the subscriber bound to UNMMSpotsSubsystem::
	// OnActiveMenuSpotReady. Without the UFUNCTION declaration, AddDynamic /
	// AddUniqueDynamic in OnGameFeatureInitialize can't compile against it,
	// and the slot-load-completed-after-state-change race never resolves.
	UFunction* OnSlotReadyFn =
		CameraSubsystemClass->FindFunctionByName(FName(TEXT("OnActiveMenuSpotReady")));
	TestNotNull(
		TEXT("G2/B7: UNMMCameraSubsystem must declare OnActiveMenuSpotReady as a UFUNCTION at "
			 "runtime — the dynamic-delegate target the spots subsystem fires when the "
			 "active slot finishes its async cinematic load. Without a reflected UFUNCTION "
			 "there is no way to AddUniqueDynamic the binding, so the race never resolves."),
		OnSlotReadyFn);

	return true;
}

// ---------------------------------------------------------------------------
// (Removed: Test 10 — FindCameraComponentRuntimeDispatch.)
//
// The deleted test attempted to exercise the dispatch by ProcessEvent-ing
// FindCameraComponent on the CDO with three tag inputs (invalid, BasicMenu,
// and a synthetic out-of-hierarchy tag) and asserting:
//   * InvalidResult == BasicMenuResult (B1: route both through the level-
//     camera branch),
//   * Invoke(UnknownTag) returns nullptr (B4: containment-based fall-through).
//
// Both assertions pass vacuously against the stripped stub. The stub's
// FindCameraComponent simply returns nullptr for every input, so:
//   * nullptr == nullptr trivially satisfies the TestEqual,
//   * nullptr trivially satisfies the TestNull.
//
// The runtime test cannot be tightened either: in a headless automation
// process the SOLUTION's dispatch also returns nullptr for the same three
// inputs. UBmrBlueprintFunctionLibrary::GetLevelCamera() walks
// UBmrGeneratedMapSubsystem::GetGeneratedMap(), which is null when no menu
// level is loaded, so both the BasicMenu and invalid branches yield nullptr;
// and the unknown-tag branch is an explicit `return nullptr;` in solution
// too. The branches that WOULD produce observable differences (Transition,
// Idle, Cinematic with real rigs / spots) all reach UNMMSpotsSubsystem::Get()
// which checkf-asserts when the world / local-player subsystem is missing —
// invoking them would crash the harness rather than report a test failure.
//
// B1 and B4 remain covered by the source-inspection tests above
// (FNmmCamera_BasicMenuAndInvalidDispatchToLevelCamera and
// FNmmCamera_DispatchUsesTagContainmentNotEnumeration), which gate on the
// CODE shape of the dispatch (presence of GetLevelCamera, an `!IsValid() ||
// BasicMenu` branch in one conditional, `.HasTag` containment over
// `Idle | Cinematic`, no `from-state -> to-state` enumeration). Those checks
// fail outright against the stripped stub (whose FindCameraComponent body
// is just `return nullptr;`), so the dispatch contract is still gated.
// ---------------------------------------------------------------------------
