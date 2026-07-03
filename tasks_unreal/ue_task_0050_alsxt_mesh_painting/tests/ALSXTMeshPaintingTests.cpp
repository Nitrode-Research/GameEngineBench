#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AlsxtCharacter.h"
#include "Components/Mesh/AlsxtPaintableSkeletalMeshComponent.h"
#include "Components/Mesh/AlsxtPaintableStaticMeshComponent.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAlsxtMeshPaintingTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0053 (ALSXT Mesh Painting).
//
// EDITABLE FILES UNDER TEST: AlsxtPaintableSkeletalMeshComponent.cpp and
// AlsxtPaintableStaticMeshComponent.cpp. In start/ the paint-prerequisite check is a
// hardcoded stub — IsMeshPaintingConfigured() returns false unconditionally, so a
// paintable component never recognizes a valid paint setup (and downstream
// initialization never runs). The solution evaluates the REAL prerequisites: a mesh
// asset, a material at slot 0, an owner implementing UAlsxtMeshPaintingInterface, and a
// scene-capture component returned by that interface.
//
// OBSERVABLE CONTRACT (CPU-only, no GPU): bool IsMeshPaintingConfigured() const.
// Each test builds a genuinely painting-configured component and asserts it reports
// configured. We spawn an AAlsxtCharacter as the owner — it implements
// UAlsxtMeshPaintingInterface and creates a MeshPaintingSceneCapture default subobject,
// so Execute_GetSceneCaptureComponent() returns non-null — then attach the paintable
// component and give it a mesh asset + a material at slot 0. start/ fails (hardcoded
// false even with everything wired); solution/ passes (real prerequisite evaluation).
//
// WHY THE COVERAGE IS NARROW (NullRHI headless constraints, documented not faked): the
// rest of the spec — dynamic-material/render-target initialization and paint
// accumulation/persistence — reads back from UTextureRenderTarget2D via scene-capture /
// draw-material passes, which require a real RHI; under -NullRHI no pixels are produced,
// so any accumulation assertion would be a false negative on a correct solution. Paint
// routing through GetMeshPaintCriteriaEntry() is also a no-op in BOTH start and solution
// (it returns a default-constructed criteria), so ShouldBePainted/CanBePainted cannot
// discriminate. IsMeshPaintingConfigured() is the one prerequisite that is
// deterministic, CPU-only, and hardcoded false in start/, so it is the reliable
// discriminator for both editable components.
//
// PIE is required (to spawn the owning character); the broken game-mode/EOS boot is
// bypassed via the DefaultEngine.ini override (see tasks_unreal/TARGETVECTOR_PIE_FIXES.md).
// ─────────────────────────────────────────────────────────────────────────────

namespace MeshPaintTests
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

	// Spawn an ALSXT character: it implements UAlsxtMeshPaintingInterface and owns a
	// MeshPaintingSceneCapture default subobject, the two owner-side prerequisites
	// IsMeshPaintingConfigured() checks.
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
		return World->SpawnActor<AAlsxtCharacter>(
			AAlsxtCharacter::StaticClass(), FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Params);
	}
}

// ── REQUIRED — PIE: a skeletal paintable component recognizes a valid paint setup ─
// Spec prerequisite for "paintable meshes initialize the required dynamic materials …
// on startup". Fails on start/ (IsMeshPaintingConfigured() hardcoded false). Passes on
// solution/ (real evaluation of mesh + material + interface owner + scene capture).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsxtSkeletalPaintConfiguredTest,
	"ALSXT.MeshPainting.skeletal_component_reports_configured",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsxtSkeletalPaintConfiguredTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MeshPaintTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = MeshPaintTests::GetTestWorld();
		AAlsxtCharacter* Character = MeshPaintTests::SpawnAlsxtCharacter(World);
		if (!TestNotNull(TEXT("An AAlsxtCharacter owner must spawn"), Character)) return true;

		// Attach a paintable skeletal component owned by the character.
		UAlsxtPaintableSkeletalMeshComponent* Comp =
			NewObject<UAlsxtPaintableSkeletalMeshComponent>(Character);
		if (!TestNotNull(TEXT("Paintable skeletal component must construct"), Comp)) return true;

		// Give it the two mesh-side prerequisites: a mesh asset and a material at slot 0.
		USkeletalMesh* Mesh = Character->GetMesh() ? Character->GetMesh()->GetSkeletalMeshAsset() : nullptr;
		if (!TestNotNull(TEXT("Character must provide a skeletal mesh asset to paint"), Mesh)) return true;
		Comp->SetSkeletalMeshAsset(Mesh);
		Comp->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));

		TestTrue(TEXT("A fully wired skeletal paintable component must report IsMeshPaintingConfigured()"),
			Comp->IsMeshPaintingConfigured());
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── REQUIRED — PIE: a static paintable component recognizes a valid paint setup ──
// Spec: "Both skeletal and static meshes support the intended painting workflow."
// Same discriminator on the static component (hardcoded false in start/).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsxtStaticPaintConfiguredTest,
	"ALSXT.MeshPainting.static_component_reports_configured",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsxtStaticPaintConfiguredTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MeshPaintTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = MeshPaintTests::GetTestWorld();
		AAlsxtCharacter* Character = MeshPaintTests::SpawnAlsxtCharacter(World);
		if (!TestNotNull(TEXT("An AAlsxtCharacter owner must spawn"), Character)) return true;

		UAlsxtPaintableStaticMeshComponent* Comp =
			NewObject<UAlsxtPaintableStaticMeshComponent>(Character);
		if (!TestNotNull(TEXT("Paintable static component must construct"), Comp)) return true;

		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!TestNotNull(TEXT("Engine cube static mesh must load"), Cube)) return true;
		Comp->SetStaticMesh(Cube);
		Comp->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));

		TestTrue(TEXT("A fully wired static paintable component must report IsMeshPaintingConfigured()"),
			Comp->IsMeshPaintingConfigured());
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
