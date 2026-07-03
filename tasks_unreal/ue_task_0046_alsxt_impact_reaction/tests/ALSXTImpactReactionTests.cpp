#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AlsxtCharacter.h"
#include "Components/Character/ALSXTImpactReactionComponent.h"
#include "State/AlsxtImpactReactionState.h"
#include "Settings/AlsxtImpactReactionSettings.h"
#include "Utility/AlsxtStructs.h"
#include "Utility/AlsxtGameplayTags.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogImpactReactionTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0049 (ALSXT Impact Reaction).
//
// EDITABLE FILE UNDER TEST: ALSXTImpactReactionComponent.cpp. In start/ the reaction
// entry/sequencing functions (ImpactReaction, StartImpactReaction, AttackReaction,
// BumpReaction, the Start*Reaction/Fall/Response family) are empty stubs {}; the
// solution implements them. The pure mapping helpers (ConvertVelocityToTag,
// HealthToHealthTag, LocationToImpactSide/Height, IsImpactReactionAllowedToStart) are
// IDENTICAL in start and solution, so they do not discriminate and are not tested.
//
// OBSERVABLE PUBLIC CONTRACT used here:
//   • void ImpactReaction(FDoubleHitResult Hit)        — public reaction entry
//   • const FAlsxtImpactReactionState& GetImpactReactionState() const
// On an Authority character, ImpactReaction() calls StartImpactReaction() which
// populates ImpactReactionParameters from the hit (ImpactVelocity = Hit.Strength) and
// calls SetImpactReactionState() — which assigns the state synchronously. So the hit's
// classification is observable immediately via GetImpactReactionState(); the empty
// start stub never sets it.
//
// SPAWN STRATEGY (and why PIE): ImpactReaction dereferences the owning ALSXT character
// through its interfaces (settings selection, movement-mode lock), so the component
// must live on a real, spawned AAlsxtCharacter (which creates the component in its
// constructor). The project's broken game-mode/EOS boot is bypassed via the stock
// GameInstance/GameMode override in DefaultEngine.ini (see
// tasks_unreal/TARGETVECTOR_PIE_FIXES.md); the test spawns its own character.
// `bSuppressLogErrors` whitelists the character's benign Error-level tick noise (the
// automation framework would otherwise record it as a failure); explicit
// TestTrue/TestFalse still fail normally.
//
// NOT ROBUSTLY TESTABLE HEADLESS (documented, not faked): multi-stage transitions
// (anticipation→response→fall→recovery), replicated client cosmetic/animation, and
// effect/physical-animation selection — driven by montages, timers, replication, and
// physical animation that do not run deterministically (or at all) under -NullRHI.
// The Attack/Bump reaction paths only act on SimulatedProxy clients, not on a
// standalone Authority character, so they are not reachable here either.
// ─────────────────────────────────────────────────────────────────────────────

namespace ImpactTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");

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

	// Return the impact-reaction component on a live ALSXT character (the character
	// creates it as a default subobject), spawning the character if none exists.
	static UAlsxtImpactReactionComponent* GetImpactReactionComponent(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AAlsxtCharacter> It(World); It; ++It)
		{
			if (AAlsxtCharacter* C = *It)
			{
				if (auto* Comp = C->FindComponentByClass<UAlsxtImpactReactionComponent>())
				{
					return Comp;
				}
			}
		}
		if (AAlsxtCharacter* C = SpawnAlsxtCharacter(World))
		{
			return C->FindComponentByClass<UAlsxtImpactReactionComponent>();
		}
		return nullptr;
	}
}

// ── REQUIRED — PIE: an impact resolves into reaction state derived from the hit ─
// Spec: "Combat hits and movement impacts resolve into the appropriate reaction
// family instead of a generic fallback" / "Reaction choice depends on impact side,
// velocity, form …". We deliver a hit with a distinctive velocity (Strength = Fast)
// and assert the component classifies it into the reaction state (ImpactVelocity =
// Fast). Fails on start/ (ImpactReaction/StartImpactReaction are empty →
// SetImpactReactionState is never called → the state stays default). Passes on
// solution/ (the hit's velocity is classified into the state). Catches a no-op
// reaction pipeline. Robust: asserts the observable public state reflects the input,
// not any montage/internal.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FImpactReactionPopulatesStateTest,
	"ALSXT.ImpactReaction.impact_resolves_into_reaction_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FImpactReactionPopulatesStateTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ImpactTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// Spawn the character (its impact-reaction component is the subject).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = ImpactTests::GetTestWorld();
		if (!ImpactTests::GetImpactReactionComponent(World))
		{
			AddError(TEXT("No AAlsxtCharacter with a UAlsxtImpactReactionComponent could be obtained — cannot test impact reaction"));
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f)); // let BeginPlay / component setup run

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = ImpactTests::GetTestWorld();
		UAlsxtImpactReactionComponent* Comp = ImpactTests::GetImpactReactionComponent(World);
		if (!Comp) return true; // already errored above

		// Precondition: the state has not classified this velocity yet.
		if (!TestTrue(TEXT("Impact velocity must be unset before any impact (precondition)"),
			Comp->GetImpactReactionState().ImpactReactionParameters.ImpactVelocity != ALSXTImpactVelocityTags::Fast))
		{
			return true;
		}

		// Deliver an impact with a distinctive velocity classification.
		FDoubleHitResult Hit;
		Hit.Strength = ALSXTImpactVelocityTags::Fast;
		Comp->ImpactReaction(Hit);

		// The impact must be classified into the observable reaction state.
		TestTrue(TEXT("ImpactReaction must store the hit's velocity into the reaction state (ImpactVelocity == Fast)"),
			Comp->GetImpactReactionState().ImpactReactionParameters.ImpactVelocity == ALSXTImpactVelocityTags::Fast);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
