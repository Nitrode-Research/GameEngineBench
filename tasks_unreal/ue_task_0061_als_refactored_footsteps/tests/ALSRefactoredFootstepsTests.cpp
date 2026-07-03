#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Animation/AnimNotifies/AnimNotify.h"

// The footstep effect knobs (FootstepEffectsSettings, FootBone, bSpawn*) are protected
// DATA members with no setters. `#define protected public` is safe HERE because the test
// only sets data members and calls the PUBLIC Notify() override — it never references a
// protected METHOD symbol (which is what mis-mangles/fails to link under this shim).
#define protected public
#include "Notifies/AlsAnimNotify_FootstepEffects.h"
#undef protected

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAlsFootstepTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0061 (ALS-Refactored Footstep Effects).
//
// EDITABLE FILE UNDER TEST: AlsAnimNotify_FootstepEffects.cpp. In start/ the notify is a
// stub: Notify() only calls Super (no ground trace, no effect resolution) and
// SpawnSound/SpawnDecal/SpawnParticleSystem are empty {}. So firing the notify spawns
// NOTHING. The solution traces from the foot bone to the ground, resolves the effect set
// for the hit surface from the settings, and spawns the configured sound/decal/particle
// at the hit, oriented to the surface.
//
// OBSERVABLE CONTRACT (CPU-only, no GPU/audio device needed): a footstep DECAL component
// (UDecalComponent) is created in the world by SpawnDecalAtLocation. The earlier suite
// asserted CDO defaults / member-function-pointer existence — structural checks the empty
// start stub also passes — and is dropped.
//
// SET-UP (the notify is normally fired by the anim system on a configured character; the
// test reproduces the minimum it needs): a skeletal-mesh actor over a floor collider, and
// a NewObject<UAlsFootstepEffectsSettings> whose Effects map has one entry with a decal
// material. FootLeftZAxis is set to +Z so the foot "up" axis aligns with the floor normal
// — otherwise the solution correctly angle-gates the decal away (default FootLeftZAxis is
// +X, perpendicular to a flat floor). The notify's protected knobs are set directly; only
// the decal is enabled to isolate the observable (sound needs an audio device, particles
// need a Niagara asset — neither is reliable headless, documented).
//
// NOT COVERED, and why: exact decal/particle/sound asset selection per physical surface
// (needs authored content + a GPU/audio device), and the multi-surface physical-material
// routing (needs distinct phys materials on distinct colliders). The asserted behavior —
// firing the notify performs the trace and spawns the configured decal at the hit — is the
// deterministic, content-light discriminator.
// ─────────────────────────────────────────────────────────────────────────────

namespace FootstepTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");
	static const TCHAR* SkeletalCubePath = TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube");

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

	static int32 CountWorldDecals(UWorld* World)
	{
		int32 Count = 0;
		for (TObjectIterator<UDecalComponent> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				++Count;
			}
		}
		return Count;
	}

	static USkeletalMeshComponent* FindFootMesh(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (USkeletalMeshComponent* M = It->FindComponentByClass<USkeletalMeshComponent>())
			{
				return M;
			}
		}
		return nullptr;
	}
}

// ── REQUIRED — PIE: firing the notify traces and spawns the configured decal ──

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsFootstepNotifySpawnsDecalTest,
	"ALSRefactored.Footsteps.notify_traces_and_spawns_configured_decal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsFootstepNotifySpawnsDecalTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(FootstepTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// Build the floor + a skeletal-mesh actor above it.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = FootstepTests::GetTestWorld();
		if (!TestNotNull(TEXT("PIE world must be live"), World)) return true;

		// Floor: a WorldStatic box, top at z=25, under the foot trace.
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator, P);
			UStaticMeshComponent* FC = Floor->GetStaticMeshComponent();
			FC->SetMobility(EComponentMobility::Movable);
			FC->SetStaticMesh(Cube);
			Floor->SetActorScale3D(FVector(10.f, 10.f, 0.5f)); // top at z = -25 + 50*0.5 = 0... keep top near 0
			Floor->SetActorLocation(FVector(0, 0, -25));
			FC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			FC->SetCollisionObjectType(ECC_WorldStatic);
			FC->SetCollisionResponseToAllChannels(ECR_Block);
			FC->UpdateCollisionProfile();
		}

		// Skeletal-mesh actor at z=100 (foot trace will start here and go down).
		FActorSpawnParameters MP;
		MP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* MeshActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator, MP);
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(MeshActor);
		MeshActor->SetRootComponent(Mesh);
		if (USkeletalMesh* CubeMesh = LoadObject<USkeletalMesh>(nullptr, FootstepTests::SkeletalCubePath))
		{
			Mesh->SetSkeletalMeshAsset(CubeMesh);
		}
		Mesh->RegisterComponent();
		MeshActor->SetActorLocation(FVector(0, 0, 100));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f)); // let collision register

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = FootstepTests::GetTestWorld();
		USkeletalMeshComponent* Mesh = FootstepTests::FindFootMesh(World);
		if (!TestNotNull(TEXT("Skeletal mesh must be present"), Mesh)) return true;

		// Build footstep settings with one decal-bearing effect entry.
		UAlsFootstepEffectsSettings* Settings = NewObject<UAlsFootstepEffectsSettings>(GetTransientPackage());
		Settings->SurfaceTraceDistance = 200.0f;
		Settings->SurfaceTraceChannel = ECC_Visibility;
		Settings->FootLeftZAxis = FVector3f(0.0f, 0.0f, 1.0f); // foot up aligned with floor normal
		Settings->FootLeftYAxis = FVector3f(0.0f, 0.0f, 1.0f);

		FAlsFootstepEffectSettings Effect;
		Effect.Decal.DecalMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		Effect.Decal.SpawnMode = EAlsFootstepDecalSpawnMode::SpawnAtTraceHitLocation;
		Settings->Effects.Add(TEnumAsByte<EPhysicalSurface>(SurfaceType_Default), Effect);

		UAlsAnimNotify_FootstepEffects* Notify = NewObject<UAlsAnimNotify_FootstepEffects>(GetTransientPackage());
		Notify->FootstepEffectsSettings = Settings;
		Notify->FootBone = EAlsFootBone::Left;
		Notify->bSpawnDecal = true;
		Notify->bSpawnSound = false;          // audio device not reliable headless
		Notify->bSpawnParticleSystem = false; // needs a Niagara asset

		const int32 Before = FootstepTests::CountWorldDecals(World);
		FAnimNotifyEventReference EventRef;
		Notify->Notify(Mesh, nullptr, EventRef);
		const int32 After = FootstepTests::CountWorldDecals(World);

		UE_LOG(LogAlsFootstepTests, Display, TEXT("[footstep-diag] decals before=%d after=%d"), Before, After);

		TestTrue(TEXT("Firing the footstep notify must spawn a decal from the configured settings"),
			After > Before);

		// The spawned footstep decal must carry the configured non-default sort order (7).
		// Stock ALS leaves UDecalComponent::SortOrder at the engine default (0), so a
		// reconstruction that never sets it produces no decal with the configured order.
		bool bFoundSortedDecal = false;
		for (TObjectIterator<UDecalComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->SortOrder == 7)
			{
				bFoundSortedDecal = true;
				break;
			}
		}
		TestTrue(TEXT("The spawned footstep decal must use the configured sort order (7), not the engine default (0)"),
			bFoundSortedDecal);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
