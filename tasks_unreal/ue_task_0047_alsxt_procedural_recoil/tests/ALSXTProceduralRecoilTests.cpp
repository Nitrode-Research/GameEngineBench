#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/TimelineComponent.h"
#include "AlsxtCharacter.h"
#include "Components/Character/AlsxtProceduralRecoilAnimComponent.h"
#include "Settings/AlsxtProceduralRecoilSettingsDataAsset.h"
#include "Curves/CurveVector.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogProceduralRecoilTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Test-only access to private members/methods (no engine headers or production code
// are modified). Production code only ever builds the recoil state machine from
// BeginPlay, which early-returns unless the owner has a full GAS setup
// (UAlsxtAbilitySystemInterface + ASC + UAlsxtRecoilAttributeSet) — a setup headless
// PIE cannot provide on a plainly spawned character (AAlsxtCharacter implements
// IAbilitySystemInterface but NOT UAlsxtAbilitySystemInterface, so BeginPlay returns
// before reaching InitializeStateMachine). We therefore build the state machine
// directly via the standard explicit-instantiation idiom (the address-of in an
// explicit instantiation is not subject to access control) and read a few private
// fields to make assertions. InitializeStateMachine() is still the stubbed function
// under test: StateMachine.Reset() in start/, populated in solution/.
namespace RecoilAccess
{
	template <class Tag> struct TBag { static typename Tag::Type Ptr; };
	template <class Tag> typename Tag::Type TBag<Tag>::Ptr{};

	template <class Tag, typename Tag::Type P> struct TPick
	{
		struct FInit { FInit() { TBag<Tag>::Ptr = P; } };
		static FInit Init;
	};
	template <class Tag, typename Tag::Type P> typename TPick<Tag, P>::FInit TPick<Tag, P>::Init;
}

#define RECOIL_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace RecoilAccess { \
		struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &UAlsxtProceduralRecoilAnimComponent::MEMBER>; \
	}
#define RECOIL_GET(COMP, TAG) ((COMP)->*RecoilAccess::TBag<RecoilAccess::TAG>::Ptr)

using FRecoilComp = UAlsxtProceduralRecoilAnimComponent;
RECOIL_STEAL(FInitSMTag,       void (FRecoilComp::*)(),                                     InitializeStateMachine)
RECOIL_STEAL(FStateMachineTag, TArray<FAlsxtProceduralRecoilAnimationState> FRecoilComp::*, StateMachine)
RECOIL_STEAL(FTimelineTag,     FTimeline FRecoilComp::*,                                    RecoilAnimationTimeline)

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0050 (ALSXT Procedural Recoil).
//
// EDITABLE FILE UNDER TEST: AlsxtProceduralRecoilAnimComponent.cpp. In start/ the three
// recoil drivers are empty stubs:
//   • InitializeStateMachine()      → StateMachine.Reset();              (no machine)
//   • PlayProceduralRecoilAnimation()→ zeroes output, RecoilAnimationTimeline.Stop();
//   • TickComponent()               → {}                                (no advance)
// The solution implements all three: InitializeStateMachine() builds the single/repeat
// fire state machine, PlayProceduralRecoilAnimation() selects a state and (via its
// OnPlay → SetupTransition) starts the recoil-animation timeline, and TickComponent()
// advances it.
//
// WHAT THESE TESTS ASSERT (observable effects of the stubbed functions):
//   A. Firing STARTS and ADVANCES the recoil-animation timeline. In start/ Play() calls
//      Stop(), so the timeline is never playing; in solution/ it plays and its playback
//      position advances over the next frames. This is the spec behavior "firing
//      produces procedural recoil" observed at the animation driver.
//   B. InitializeStateMachine() builds a WIRED fire state machine (>=2 states whose
//      transition/play delegates are bound). In start/ the machine is empty.
//
// WHY OUTPUT MAGNITUDE ITSELF IS NOT ASSERTED (documented, not faked): the recoil
// magnitude (CurrentTargetRotation/Translation in RefreshRecoilAnimationTimeline) is
// produced by CalculateTargetRecoilParameters from the settings ranges scaled by
// PlayerControllerMagnitudes — values driven by the owning player controller and GAS
// recoil attributes that do not exist on a headless, controller-less spawned
// character, so GetOutput() stays at identity even though the timeline is correctly
// driven. We assert the timeline being driven (the function-under-test behavior)
// rather than a magnitude that depends on absent runtime state. Also not robustly
// testable headless: controller kick smoothing, aiming/ready scaling, noise/sway, and
// per-frame decay shape (all timing/controller/attribute dependent).
//
// PIE is required (the recoil timeline ticks with the live component). The broken
// game-mode/EOS boot is bypassed via the stock GameInstance/GameMode override in
// DefaultEngine.ini (see tasks_unreal/TARGETVECTOR_PIE_FIXES.md); the test spawns its
// own character. bSuppressLogErrors whitelists the character's benign Error-level
// noise (incl. the expected "UAlsxtAbilitySystemInterface Not Implemented" from
// BeginPlay); explicit TestTrue/TestFalse still fail.
// ─────────────────────────────────────────────────────────────────────────────

namespace RecoilTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");
	static const TCHAR* RecoilSettingsPath =
		TEXT("/ALSXT/ALS/Data/Character/RecoilData/Shotgun/DA_AlsxtPRS_Shotgun.DA_AlsxtPRS_Shotgun");

	static UWorld* GetTestWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if ((Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game) && Ctx.World())
				return Ctx.World();
		}
		return nullptr;
	}

	static AAlsxtCharacter* SpawnAlsxtCharacter(UWorld* World)
	{
		if (!World) return nullptr;
		static const TCHAR* Paths[] = {
			TEXT("/ALSXT/ALS/Character/B_ALSXT_Character.B_ALSXT_Character_C"),
			TEXT("/ALSXT/ALS/Character/B_AlsxtCharacterPlayer.B_AlsxtCharacterPlayer_C"),
		};
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		for (const TCHAR* Path : Paths)
		{
			if (UClass* CharClass = LoadClass<AAlsxtCharacter>(nullptr, Path))
			{
				if (AAlsxtCharacter* C = World->SpawnActor<AAlsxtCharacter>(
						CharClass, FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Params))
				{
					return C;
				}
			}
		}
		return nullptr;
	}

	static UAlsxtProceduralRecoilAnimComponent* GetRecoilComponent(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AAlsxtCharacter> It(World); It; ++It)
		{
			if (AAlsxtCharacter* C = *It)
			{
				if (auto* Comp = C->FindComponentByClass<UAlsxtProceduralRecoilAnimComponent>())
				{
					return Comp;
				}
			}
		}
		if (AAlsxtCharacter* C = SpawnAlsxtCharacter(World))
		{
			return C->FindComponentByClass<UAlsxtProceduralRecoilAnimComponent>();
		}
		return nullptr;
	}

	// Synthesize non-zero recoil curves (the frozen settings asset ships none) and arm
	// the component, then build the state machine directly (BeginPlay's GAS gate blocks
	// it on a plain character). Returns false (and records an error) if anything needed
	// is missing.
	static bool ArmRecoil(FAutomationTestBase& Test, UAlsxtProceduralRecoilAnimComponent* Comp)
	{
		if (!Comp)
		{
			Test.AddError(TEXT("No AAlsxtCharacter with a UAlsxtProceduralRecoilAnimComponent could be obtained"));
			return false;
		}
		UAlsxtProceduralRecoilSettingsDataAsset* Settings =
			LoadObject<UAlsxtProceduralRecoilSettingsDataAsset>(nullptr, RecoilSettingsPath);
		if (!Settings)
		{
			Test.AddError(TEXT("Could not load recoil settings asset — cannot drive recoil"));
			return false;
		}

		UCurveVector* RotCurve = NewObject<UCurveVector>(GetTransientPackage());
		UCurveVector* LocCurve = NewObject<UCurveVector>(GetTransientPackage());
		RotCurve->AddToRoot();
		LocCurve->AddToRoot();
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			RotCurve->FloatCurves[Axis].AddKey(0.0f, 0.0f);
			RotCurve->FloatCurves[Axis].AddKey(0.5f, 30.0f);
			LocCurve->FloatCurves[Axis].AddKey(0.0f, 0.0f);
			LocCurve->FloatCurves[Axis].AddKey(0.5f, 30.0f);
		}
		Settings->SingleFireRotationCurve = RotCurve;
		Settings->SingleFireLocationCurve = LocCurve;
		Settings->AutoFireRotationCurve = RotCurve;
		Settings->AutoFireLocationCurve = LocCurve;
		Settings->PlayRate = 1.0f;

		Comp->ProceduralRecoilSettings = Settings;
		Comp->SetProceduralRecoilSettings(Settings, 600.0f, 0);
		(Comp->*RecoilAccess::TBag<RecoilAccess::FInitSMTag>::Ptr)(); // InitializeStateMachine
		return true;
	}
}

// ── REQUIRED — PIE: firing starts and advances the recoil-animation timeline ──
// Catches a no-op recoil pipeline. Fails on start/ (Play() calls Stop() → timeline
// never plays). Passes on solution/ (Play() selects a fire state whose OnPlay starts
// the timeline, and TickComponent advances it). Robust: asserts the timeline is
// actively driven, independent of controller/attribute-derived magnitudes.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecoilFiringDrivesAnimationTest,
	"ALSXT.ProceduralRecoil.firing_drives_recoil_animation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecoilFiringDrivesAnimationTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	static bool bObservedPlaying;
	static float MaxPlaybackPos;
	bObservedPlaying = false;
	MaxPlaybackPos = 0.0f;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(RecoilTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = RecoilTests::GetTestWorld();
		UAlsxtProceduralRecoilAnimComponent* Comp = RecoilTests::GetRecoilComponent(World);
		if (!RecoilTests::ArmRecoil(*this, Comp))
		{
			return true;
		}

		// Precondition: the recoil-animation timeline is not playing before firing.
		if (!TestFalse(TEXT("Recoil-animation timeline must not be playing before firing (precondition)"),
			RECOIL_GET(Comp, FTimelineTag).IsPlaying()))
		{
			return true;
		}

		Comp->PlayProceduralRecoilAnimation();
		return true;
	}));

	// Sample over a short window: did the timeline play, and did its position advance?
	for (int32 i = 0; i < 8; ++i)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.05f));
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
		{
			UWorld* World = RecoilTests::GetTestWorld();
			if (UAlsxtProceduralRecoilAnimComponent* Comp = RecoilTests::GetRecoilComponent(World))
			{
				const FTimeline& TL = RECOIL_GET(Comp, FTimelineTag);
				bObservedPlaying |= TL.IsPlaying();
				MaxPlaybackPos = FMath::Max(MaxPlaybackPos, TL.GetPlaybackPosition());
			}
			return true;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		TestTrue(TEXT("Firing must start the recoil-animation timeline (Play, not Stop)"), bObservedPlaying);
		if (!TestTrue(TEXT("The recoil-animation timeline must advance after firing"), MaxPlaybackPos > 0.01f))
		{
			AddError(FString::Printf(TEXT("Max timeline playback position observed: %f"), MaxPlaybackPos));
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── REQUIRED — PIE: InitializeStateMachine builds a wired fire state machine ───
// Isolates InitializeStateMachine(). Fails on start/ (StateMachine.Reset() → empty).
// Passes on solution/ (builds the single- and repeat-fire states with their transition
// and play delegates bound). Robust against a trivial "add empty placeholder states"
// stub: requires the delegates to actually be bound.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecoilStateMachineBuiltTest,
	"ALSXT.ProceduralRecoil.initialize_builds_fire_state_machine",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecoilStateMachineBuiltTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(RecoilTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = RecoilTests::GetTestWorld();
		UAlsxtProceduralRecoilAnimComponent* Comp = RecoilTests::GetRecoilComponent(World);
		if (!Comp)
		{
			AddError(TEXT("No AAlsxtCharacter with a UAlsxtProceduralRecoilAnimComponent could be obtained"));
			return true;
		}

		// Build the state machine (the function under test).
		(Comp->*RecoilAccess::TBag<RecoilAccess::FInitSMTag>::Ptr)();

		const TArray<FAlsxtProceduralRecoilAnimationState>& SM = RECOIL_GET(Comp, FStateMachineTag);
		if (!TestTrue(TEXT("InitializeStateMachine must build at least the single- and repeat-fire states"),
			SM.Num() >= 2))
		{
			AddError(FString::Printf(TEXT("StateMachine state count: %d"), SM.Num()));
			return true;
		}

		bool bAllWired = true;
		for (const FAlsxtProceduralRecoilAnimationState& State : SM)
		{
			bAllWired &= State.TransitionConditionDelegate.IsBound() && State.OnPlayDelegate.IsBound();
		}
		TestTrue(TEXT("Each fire state must have its transition-condition and on-play delegates bound"), bAllWired);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
