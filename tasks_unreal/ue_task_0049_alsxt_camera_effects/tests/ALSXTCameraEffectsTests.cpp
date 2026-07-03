#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Scene.h"
#include "Components/PostProcessComponent.h"
#include "Components/Character/ALSXTCharacterCameraEffectsComponent.h"
#include "Components/PlayerController/AlsxtPlayerViewportEffectsComponent.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCameraEffectsTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Test-only access to the private blendable-index members (no production code is
// modified). Standard explicit-instantiation idiom: the address-of a private member in
// an explicit instantiation is not subject to access control. We only set the index
// (which BeginPlay would normally assign) to point at the blendable we add; the effect
// logic under test is unchanged.
namespace CamAccess
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
#define CAM_STEAL(TAG, OWNER, MEMBERPTRTYPE, MEMBER) \
	namespace CamAccess { struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &OWNER::MEMBER>; }
#define CAM_SET(OBJ, TAG, VALUE) ((OBJ)->*CamAccess::TBag<CamAccess::TAG>::Ptr = (VALUE))

CAM_STEAL(FCharSupIdx, UAlsxtCharacterCameraEffectsComponent, int32 UAlsxtCharacterCameraEffectsComponent::*, SuppressionBlendableIndex)
CAM_STEAL(FViewSupIdx, UAlsxtPlayerViewportEffectsComponent,  int32 UAlsxtPlayerViewportEffectsComponent::*,  SuppressionEffectBlendableIndex)

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0052 (ALSXT Camera Effects).
//
// EDITABLE FILES UNDER TEST:
//   ALSXTCharacterCameraEffectsComponent.cpp  (character side)
//   AlsxtPlayerViewportEffectsComponent.cpp   (player-controller / viewport side)
// In start/ the effect-add paths are stubs: the character's AddSuppression() just
// writes CurrentSuppressionAmount = 0 (a no-op), and the viewport's
// AddSuppressionEffect() is empty {}. The solution drives the suppression effect
// through the post-process volume: it clamps and writes the weight of the suppression
// WeightedBlendable on the owning PostProcessComponent (and the character side mirrors
// that into CurrentSuppressionAmount), then schedules a timer-driven fade-out.
//
// WHAT THESE TESTS ASSERT (the spec's core, observable without a GPU):
//   • AddSuppression raises the post-process suppression blendable weight above zero
//     and reports it via CurrentSuppressionAmount (character side).
//   • AddSuppressionEffect raises the post-process suppression blendable weight above
//     zero (viewport side).
//   • Accumulation is clamped to the configured max weight (not unbounded).
// The spec requires effects to appear "through the character's post-process volume —
// they cannot be achieved by spawning separate screen-space actors"; asserting the
// blendable weight verifies exactly that routing. Fails on start/ (no-op stubs leave
// the weight at zero); passes on solution/.
//
// SET-UP NOTE: BeginPlay only builds the PostProcessComponent + blendables on an
// AutonomousProxy ALSXT character (it early-returns otherwise), which headless PIE
// can't provide. The test therefore performs the same wiring directly — create a
// PostProcessComponent, add one suppression blendable, and point the (private)
// blendable index at it — which is legitimate harness setup, not a change to the logic
// under test. The component's default GeneralCameraEffectsSettings already enable the
// suppression effect with max weight 1.0. PIE is required because AddSuppression
// schedules a fade-out timer (needs a live world); the broken game-mode/EOS boot is
// bypassed via the DefaultEngine.ini override (see tasks_unreal/TARGETVECTOR_PIE_FIXES.md).
//
// NOT ROBUSTLY TESTABLE HEADLESS (documented): the actual on-screen visual, camera
// shake driven by movement state, the depth-of-field trace, and the multi-second
// incremental fade-out shape — all depend on rendering, a possessed pawn/controller,
// and real-time timer progression. We assert the add-through-post-process behavior and
// the clamp, which are deterministic.
// ─────────────────────────────────────────────────────────────────────────────

namespace CamEffectTests
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

	// Create a PostProcessComponent with a single suppression blendable on Host and
	// return its weight reference index (0). Sets the PostProcess member on the target.
	static UPostProcessComponent* WirePostProcess(AActor* Host)
	{
		UPostProcessComponent* PP = NewObject<UPostProcessComponent>(Host);
		PP->RegisterComponent();
		PP->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(0.0f, nullptr));
		return PP;
	}
}

// ── REQUIRED — PIE: AddSuppression drives the post-process blendable weight ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterAddSuppressionDrivesPostProcessTest,
	"ALSXT.CameraEffects.character_add_suppression_drives_post_process",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCharacterAddSuppressionDrivesPostProcessTest::RunTest(const FString&)
{
#if WITH_EDITOR
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(CamEffectTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = CamEffectTests::GetTestWorld();
		if (!TestNotNull(TEXT("PIE world must be live"), World)) return true;

		AActor* Host = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
		if (!TestNotNull(TEXT("Host actor must spawn"), Host)) return true;

		UAlsxtCharacterCameraEffectsComponent* Comp =
			NewObject<UAlsxtCharacterCameraEffectsComponent>(Host);
		Comp->RegisterComponent();

		// Wire the post-process volume the same way BeginPlay would on a real character.
		Comp->GeneralCameraEffectsSettings.bEnableSuppressionEffect = true;
		Comp->PostProcessComponent = CamEffectTests::WirePostProcess(Host);
		CAM_SET(Comp, FCharSupIdx, 0);

		auto BlendWeight = [&]() -> float
		{
			return Comp->PostProcessComponent->Settings.WeightedBlendables.Array[0].Weight;
		};

		if (!TestEqual(TEXT("Suppression weight must start at zero"), BlendWeight(), 0.0f)) return true;

		// Add a suppression effect with a long recovery delay (so the fade timer does
		// not reduce it before we read it).
		Comp->AddSuppression(0.5f, 100.0f);

		TestTrue(TEXT("AddSuppression must report a positive CurrentSuppressionAmount"),
			Comp->CurrentSuppressionAmount > 0.0f);
		TestTrue(TEXT("AddSuppression must raise the post-process suppression blendable weight"),
			BlendWeight() > 0.0f);

		// Accumulation must be clamped to the configured max weight (1.0), not unbounded.
		Comp->AddSuppression(0.9f, 100.0f);
		TestTrue(TEXT("Repeated AddSuppression must clamp to the configured max weight"),
			BlendWeight() <= Comp->GeneralCameraEffectsSettings.RadialBlurMaxWeight + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Repeated AddSuppression must still leave a positive weight"),
			BlendWeight() > 0.0f);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── REQUIRED — PIE: viewport AddSuppressionEffect drives the post-process weight ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FViewportAddSuppressionDrivesPostProcessTest,
	"ALSXT.CameraEffects.viewport_add_suppression_drives_post_process",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FViewportAddSuppressionDrivesPostProcessTest::RunTest(const FString&)
{
#if WITH_EDITOR
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(CamEffectTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = CamEffectTests::GetTestWorld();
		if (!TestNotNull(TEXT("PIE world must be live"), World)) return true;

		AActor* Host = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
		if (!TestNotNull(TEXT("Host actor must spawn"), Host)) return true;

		UAlsxtPlayerViewportEffectsComponent* Comp =
			NewObject<UAlsxtPlayerViewportEffectsComponent>(Host);
		// Deliberately NOT registered: this component's BeginPlay calls
		// IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner()) with its role guard
		// commented out, which asserts on a non-ALSXT owner. RegisterComponent would
		// auto-fire BeginPlay in the live PIE world. AddSuppressionEffect makes no
		// interface calls and GetWorld() still resolves via the owner, so the fade-out
		// timer it schedules works without registration.

		Comp->GeneralCameraEffectsSettings.bEnableSuppressionEffect = true;
		Comp->PostProcessComponent = CamEffectTests::WirePostProcess(Host);
		CAM_SET(Comp, FViewSupIdx, 0);

		auto BlendWeight = [&]() -> float
		{
			return Comp->PostProcessComponent->Settings.WeightedBlendables.Array[0].Weight;
		};

		if (!TestEqual(TEXT("Viewport suppression weight must start at zero"), BlendWeight(), 0.0f)) return true;

		Comp->AddSuppressionEffect(0.5f, 100.0f);
		TestTrue(TEXT("Viewport AddSuppressionEffect must raise the post-process suppression blendable weight"),
			BlendWeight() > 0.0f);

		Comp->AddSuppressionEffect(0.9f, 100.0f);
		TestTrue(TEXT("Repeated viewport AddSuppressionEffect must clamp to the configured max weight"),
			BlendWeight() <= Comp->GeneralCameraEffectsSettings.SuppressionEffectMaxWeight + KINDA_SMALL_NUMBER);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
