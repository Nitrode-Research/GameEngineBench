// Copyright 2024 UnrealBench. All Rights Reserved.
// MonsterCorpseSystemTests.cpp
// Automation tests for ue_task_0026_monster_corpse_system.
//
// All PIE-dependent checks are batched into a single PIE session
// (FCorpseSystem_RuntimeBehaviors) to avoid the navmesh-crash that
// occurs when cycling TestLevel PIE sessions rapidly in UE 5.7.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "Containers/SpscQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

// Project-specific headers
#include "World/RogueMonsterCorpse.h"
#include "Subsystems/RogueMonsterCorpseSubsystem.h"
#include "Performance/RogueActorPoolingSubsystem.h"
#include "Core/RogueMonsterData.h"
#include "AI/RogueAICharacter.h"


// ---------------------------------------------------------------------------
// Private-member access via explicit template instantiation (robber trick).
// Works for private and protected non-UPROPERTY fields, and for protected
// non-virtual member functions.
// ---------------------------------------------------------------------------
namespace CorpseSystemPrivate
{
	template <typename Tag, typename Tag::Type Member>
	struct Robber { friend typename Tag::Type Steal(Tag) { return Member; } };

	// URogueMonsterCorpseSubsystem::CurrentCorpseCount (int32, protected)
	struct CorpseCountTag
	{
		using Type = int32 URogueMonsterCorpseSubsystem::*;
		friend Type Steal(CorpseCountTag);
	};
	template struct Robber<CorpseCountTag, &URogueMonsterCorpseSubsystem::CurrentCorpseCount>;
	static int32& GetCorpseCount(URogueMonsterCorpseSubsystem* SS) { return SS->*Steal(CorpseCountTag()); }

	// URogueMonsterCorpseSubsystem::MinimumAge (float, protected)
	struct MinAgeTag
	{
		using Type = float URogueMonsterCorpseSubsystem::*;
		friend Type Steal(MinAgeTag);
	};
	template struct Robber<MinAgeTag, &URogueMonsterCorpseSubsystem::MinimumAge>;
	static float& GetMinimumAge(URogueMonsterCorpseSubsystem* SS) { return SS->*Steal(MinAgeTag()); }

	// URogueMonsterCorpseSubsystem::MaxCorpses (int32, protected)
	struct MaxCorpsesTag
	{
		using Type = int32 URogueMonsterCorpseSubsystem::*;
		friend Type Steal(MaxCorpsesTag);
	};
	template struct Robber<MaxCorpsesTag, &URogueMonsterCorpseSubsystem::MaxCorpses>;
	static int32 GetMaxCorpses(URogueMonsterCorpseSubsystem* SS) { return SS->*Steal(MaxCorpsesTag()); }

	// URogueMonsterCorpseSubsystem::Corpses (TSpscQueue<FMonsterCorpseInfo>, protected)
	struct CorpsesQueueTag
	{
		using Type = TSpscQueue<FMonsterCorpseInfo> URogueMonsterCorpseSubsystem::*;
		friend Type Steal(CorpsesQueueTag);
	};
	template struct Robber<CorpsesQueueTag, &URogueMonsterCorpseSubsystem::Corpses>;
	static TSpscQueue<FMonsterCorpseInfo>& GetCorpsesQueue(URogueMonsterCorpseSubsystem* SS) { return SS->*Steal(CorpsesQueueTag()); }

	// URogueMonsterCorpseSubsystem::CleanupNextAvailableCorpse (void(), protected)
	struct CleanupTag
	{
		using Type = void (URogueMonsterCorpseSubsystem::*)();
		friend Type Steal(CleanupTag);
	};
	template struct Robber<CleanupTag, &URogueMonsterCorpseSubsystem::CleanupNextAvailableCorpse>;
	static void CallCleanup(URogueMonsterCorpseSubsystem* SS) { (SS->*Steal(CleanupTag()))(); }

	// URogueActorPoolingSubsystem::AvailableActorPool (TMap, protected)
	struct ActorPoolTag
	{
		using Type = TMap<TSubclassOf<AActor>, FActorPool> URogueActorPoolingSubsystem::*;
		friend Type Steal(ActorPoolTag);
	};
	template struct Robber<ActorPoolTag, &URogueActorPoolingSubsystem::AvailableActorPool>;
	static TMap<TSubclassOf<AActor>, FActorPool>& GetActorPool(URogueActorPoolingSubsystem* Pooler) { return Pooler->*Steal(ActorPoolTag()); }

	// ARogueMonsterCorpse::ImpulseBoneRemapping (TMap<FName,FName>, protected)
	struct BoneRemapTag
	{
		using Type = TMap<FName, FName> ARogueMonsterCorpse::*;
		friend Type Steal(BoneRemapTag);
	};
	template struct Robber<BoneRemapTag, &ARogueMonsterCorpse::ImpulseBoneRemapping>;
	static TMap<FName, FName>& GetBoneRemapping(ARogueMonsterCorpse* Corpse) { return Corpse->*Steal(BoneRemapTag()); }
}


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace CorpseTestHelpers
{
	static const TCHAR* MapPath = TEXT("/Game/ActionRoguelike/Maps/TestLevel");

	UWorld* GetPIEWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE && Ctx.World() && Ctx.World()->IsGameWorld())
				return Ctx.World();
		}
		return nullptr;
	}

	// Drain the Corpses queue without releasing actors to pool.
	// Used to establish a clean state before a cleanup-logic test.
	void DrainCorpsesQueue(URogueMonsterCorpseSubsystem* SS)
	{
		auto& Queue = CorpseSystemPrivate::GetCorpsesQueue(SS);
		while (!Queue.IsEmpty())
		{
			FMonsterCorpseInfo Temp;
			Queue.Dequeue(Temp);
		}
	}

	// Acquire NumActors corpse actors from pool and enqueue them with TimeAdded=0
	// so they are immediately eligible for cleanup (age >= MinimumAge when MinimumAge==0).
	// Also sets CurrentCorpseCount to NumActors.
	bool EnqueueTestCorpses(UWorld* World, URogueMonsterCorpseSubsystem* SS,
		URogueActorPoolingSubsystem* Pooler, int32 NumActors)
	{
		auto& Queue = CorpseSystemPrivate::GetCorpsesQueue(SS);
		int32 Enqueued = 0;
		for (int32 i = 0; i < NumActors; i++)
		{
			// Place actors deep below the level so WasRecentlyRendered() returns false
			FTransform TM(FRotator::ZeroRotator, FVector(0.f, float(i) * 100.f, -50000.f));
			ARogueMonsterCorpse* TmpCorpse = Pooler->AcquireFromPool<ARogueMonsterCorpse>(
				SS, ARogueMonsterCorpse::StaticClass(), TM);
			if (TmpCorpse)
			{
				Queue.Enqueue(FMonsterCorpseInfo(TmpCorpse, 0.0f));
				Enqueued++;
			}
		}
		CorpseSystemPrivate::GetCorpseCount(SS) = Enqueued;
		return Enqueued == NumActors;
	}
}


// ---------------------------------------------------------------------------
// Test 1 — CDO / structural checks (no PIE required)
//
// B1  CorpseAnimInstance UPROPERTY on URogueMonsterData (SetCorpseProperties reads it)
// B4  ARogueMonsterCorpse constructor creates MeshComp (physics target)
// B5  HitFlashMaterial UPROPERTY on URogueMonsterData
// B6  ImpulseBoneRemapping UPROPERTY on both ARogueMonsterCorpse and URogueMonsterData
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCorpseSystem_CDOChecks,
	"ActionRoguelike.CorpseSystem.CDOChecks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCorpseSystem_CDOChecks::RunTest(const FString& Parameters)
{
	// ARogueMonsterCorpse CDO — constructor must create MeshComp
	const ARogueMonsterCorpse* CorpseCDO = GetDefault<ARogueMonsterCorpse>();
	if (!TestNotNull(TEXT("ARogueMonsterCorpse CDO must exist"), CorpseCDO)) return false;

	// B4: constructor creates MeshComp (physics simulation target in SetCorpseProperties)
	TestNotNull(TEXT("B4: ARogueMonsterCorpse CDO must have a valid MeshComp (created in constructor)"),
		CorpseCDO->GetMesh());

	// B6: ImpulseBoneRemapping UPROPERTY must be present on ARogueMonsterCorpse
	{
		const FMapProperty* RemapProp = FindFProperty<FMapProperty>(
			CorpseCDO->GetClass(), TEXT("ImpulseBoneRemapping"));
		TestNotNull(TEXT("B6: ImpulseBoneRemapping UPROPERTY must exist on ARogueMonsterCorpse"), RemapProp);
	}

	// URogueMonsterData CDO — must expose the fields that SetCorpseProperties and
	// AddImpulseAtLocationCustom read at runtime.
	const URogueMonsterData* DataCDO = GetDefault<URogueMonsterData>();
	if (TestNotNull(TEXT("URogueMonsterData CDO must exist"), DataCDO))
	{
		// B1: CorpseAnimInstance UPROPERTY required for anim-instance copy
		TestNotNull(TEXT("B1: CorpseAnimInstance UPROPERTY must exist on URogueMonsterData"),
			FindFProperty<FObjectPropertyBase>(DataCDO->GetClass(), TEXT("CorpseAnimInstance")));

		// B5: HitFlashMaterial UPROPERTY required for overlay material copy
		TestNotNull(TEXT("B5: HitFlashMaterial UPROPERTY must exist on URogueMonsterData"),
			FindFProperty<FObjectPropertyBase>(DataCDO->GetClass(), TEXT("HitFlashMaterial")));

		// B6: ImpulseBoneRemapping UPROPERTY required for bone-remap copy into corpse
		TestNotNull(TEXT("B6: ImpulseBoneRemapping UPROPERTY must exist on URogueMonsterData"),
			FindFProperty<FMapProperty>(DataCDO->GetClass(), TEXT("ImpulseBoneRemapping")));
	}

	return true;
}


// ---------------------------------------------------------------------------
// Test 2 — Runtime behaviors (single PIE session)
//
// B7   AddImpulseAtLocationCustom returns true                                (DIRECT)
// B8   CustomPrimitiveData[0] set to non-zero (hit flash time stamp)          (DIRECT)
// B6   Bone remapping applied before impulse dispatch                         (PARTIAL)
// B9   Hit-flash overlay timer starts without crashing                        (PARTIAL)
// B10  Actor pool pre-warmed with MaxCorpses ARogueMonsterCorpse entries      (DIRECT)
// B14  FetchCorpse returns non-null corpse                                    (DIRECT)
// B11  Corpse placed at dead actor's world location                           (DIRECT)
// B13  CurrentCorpseCount incremented by FetchCorpse                         (DIRECT)
// B1   Corpse skeletal mesh matches source character mesh                     (DIRECT)
// B3   Pose snapshot 'InitialPose' present on corpse AnimInstance             (DIRECT)
// B4   Full-body physics simulation enabled after SetCorpseProperties         (DIRECT)
// B17  CleanupNextAvailableCorpse removes exactly one entry per call          (DIRECT)
// B15  Oldest entry is peeked and conditionally removed (age + screen gate)   (DIRECT)
// B16  Eligible corpse is released to pool and removed from queue             (DIRECT)
// B18  Tick calls CleanupNextAvailableCorpse once per frame                   (DIRECT)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCorpseSystem_RuntimeBehaviors,
	"ActionRoguelike.CorpseSystem.RuntimeBehaviors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCorpseSystem_RuntimeBehaviors::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	static ARogueMonsterCorpse* DirectCorpse = nullptr;
	static ARogueAICharacter*   FoundAIChar = nullptr;
	static URogueMonsterData*   FoundMonsterData = nullptr;
	static int32 CountBeforeCleanupCall = 0;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(CorpseTestHelpers::MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));

	// Short wait for subsystems to initialize before any gameplay consumes pool actors.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// -----------------------------------------------------------------------
	// Phase 1: B10 — Pool pre-warming
	// Initialize must call PrimeActorPool(ARogueMonsterCorpse::StaticClass(), MaxCorpses).
	// Start stub: Initialize only calls Super::Initialize → pool entry never created.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 1 — pool pre-warming)"), World)) return true;

		URogueActorPoolingSubsystem* Pooler = World->GetSubsystem<URogueActorPoolingSubsystem>();
		if (!TestNotNull(TEXT("URogueActorPoolingSubsystem must be valid"), Pooler)) return true;

		URogueMonsterCorpseSubsystem* CorpseSS = World->GetSubsystem<URogueMonsterCorpseSubsystem>();
		if (!TestNotNull(TEXT("URogueMonsterCorpseSubsystem must be valid"), CorpseSS)) return true;

		const int32 MaxCorpses = CorpseSystemPrivate::GetMaxCorpses(CorpseSS);

		// B10: pool map check only meaningful when pooling is enabled.
		// When disabled, AcquireFromPool bypasses AcquireFromPool_Internal so
		// FindOrAdd is never called and AvailableActorPool stays empty — that is
		// expected behaviour, not a solution bug.
		if (!URogueActorPoolingSubsystem::IsPoolingEnabled(World))
		{
			AddWarning(TEXT("B10: Actor pooling disabled in this environment; pool map check skipped."));
			return true;
		}

		auto& ActorPoolMap = CorpseSystemPrivate::GetActorPool(Pooler);
		FActorPool* CorpsePool = ActorPoolMap.Find(ARogueMonsterCorpse::StaticClass());

		// B10 DIRECT: pool entry for ARogueMonsterCorpse must exist.
		// Start stub (no PrimeActorPool call): CorpsePool is null → fails.
		if (!TestNotNull(
			TEXT("B10: AvailableActorPool must have an entry for ARogueMonsterCorpse after Initialize"),
			CorpsePool))
			return true;

		// B10 DIRECT: pool must contain at least MaxCorpses pre-warmed actors.
		TestTrue(
			FString::Printf(TEXT("B10: Pool for ARogueMonsterCorpse must have >= MaxCorpses (%d) free actors after Initialize"), MaxCorpses),
			CorpsePool->FreeActors.Num() >= MaxCorpses);

		return true;
	}));

	// Allow game mode and world subsystems to finish initializing (bot spawner fires at ~2s).
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// -----------------------------------------------------------------------
	// Phase 2: B7 — AddImpulseAtLocationCustom returns true
	// Spawn a corpse actor directly and call the function.
	// Start stub: returns false unconditionally → assertion fails.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 2 — impulse return value)"), World)) return true;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DirectCorpse = World->SpawnActor<ARogueMonsterCorpse>(
			ARogueMonsterCorpse::StaticClass(),
			FVector(0.f, 0.f, -50000.f), FRotator::ZeroRotator, Params);

		if (!TestNotNull(TEXT("ARogueMonsterCorpse must be spawnable for impulse test"), DirectCorpse))
			return true;

		// B7 DIRECT: solution applies impulse and returns true.
		// Start stub returns false.
		const bool bHandled = DirectCorpse->AddImpulseAtLocationCustom(
			FVector(1000.f, 0.f, 0.f),
			DirectCorpse->GetActorLocation(),
			NAME_None);
		TestTrue(TEXT("B7: AddImpulseAtLocationCustom must return true (impulse was handled)"), bHandled);

		// B8 DIRECT: solution calls SetCustomPrimitiveDataFloat(0, World->TimeSeconds) to
		// trigger the hit flash shader.  WorldTimeSeconds > 0 at this point (~2 s into PIE).
		// Start stub returns false without touching primitive data → Data[0] stays 0.
		if (USkeletalMeshComponent* CM = DirectCorpse->GetMesh())
		{
			const FCustomPrimitiveData& CPD = CM->GetCustomPrimitiveData();
			TestTrue(
				TEXT("B8: CustomPrimitiveData[0] must be non-zero after AddImpulseAtLocationCustom (hit flash time stamp set)"),
				CPD.Data.IsValidIndex(0) && CPD.Data[0] != 0.f);
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 3: B6 — Bone remapping applied without breaking dispatch
	// Inject a remap entry, call with the source bone, verify still returns true.
	// B9 PARTIAL: The timer started inside AddImpulseAtLocationCustom must not crash.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		if (!DirectCorpse) return true; // Phase 2 failed; already warned

		// Inject remap: "test_bone" → "pelvis"
		TMap<FName, FName>& Remapping = CorpseSystemPrivate::GetBoneRemapping(DirectCorpse);
		Remapping.Add(FName("test_bone"), FName("pelvis"));

		// B6 PARTIAL: with remap injected, calling with the source bone must still succeed.
		// The solution looks up the remap, re-routes to "pelvis", applies the impulse,
		// and returns true.  Start stub returns false.
		const bool bHandledRemap = DirectCorpse->AddImpulseAtLocationCustom(
			FVector(500.f, 0.f, 200.f),
			DirectCorpse->GetActorLocation(),
			FName("test_bone"));
		TestTrue(
			TEXT("B6: AddImpulseAtLocationCustom must return true when bone remapping is configured"),
			bHandledRemap);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 4: B14, B11, B13, B1, B2, B3, B4, B5, B12
	// Requires a live ARogueAICharacter with valid MonsterConfig in the PIE world.
	// Wait a little longer so the bot spawner has had time to place AI characters.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 4 — FetchCorpse)"), World)) return true;

		URogueMonsterCorpseSubsystem* CorpseSS = World->GetSubsystem<URogueMonsterCorpseSubsystem>();
		if (!TestNotNull(TEXT("URogueMonsterCorpseSubsystem must be valid (Phase 4)"), CorpseSS)) return true;

		// Find a live ARogueAICharacter whose MonsterConfig has a valid CorpseAnimInstance
		// and whose mesh has an active AnimInstance (required by SetCorpseProperties).
		FoundAIChar = nullptr;
		FoundMonsterData = nullptr;
		for (TActorIterator<ARogueAICharacter> It(World); It; ++It)
		{
			ARogueAICharacter* Candidate = *It;
			if (!IsValid(Candidate) || Candidate->IsActorBeingDestroyed()) continue;

			const FObjectProperty* ConfigProp = FindFProperty<FObjectProperty>(
				Candidate->GetClass(), TEXT("MonsterConfig"));
			if (!ConfigProp) continue;

			URogueMonsterData* Data = Cast<URogueMonsterData>(
				ConfigProp->GetObjectPropertyValue_InContainer(Candidate));
			if (!Data || !Data->CorpseAnimInstance) continue;

			USkeletalMeshComponent* Mesh = Candidate->GetMesh();
			if (!Mesh || !Mesh->GetAnimInstance()) continue;

			FoundAIChar = Candidate;
			FoundMonsterData = Data;
			break;
		}

		if (!FoundAIChar)
		{
			AddError(TEXT("Phase 4: No ARogueAICharacter with valid MonsterConfig found in PIE world. "
				"B14/B11/B13/B1/B4 require at least one live AI. "
				"Ensure the TestLevel game mode spawns bots before Phase 4 runs (the test waits 3 s)."));
			return true;
		}

		// Reset count so the post-call delta is unambiguous
		CorpseSystemPrivate::GetCorpseCount(CorpseSS) = 0;

		const FVector AILocation = FoundAIChar->GetActorLocation();

		// B14 DIRECT: FetchCorpse must return a non-null ARogueMonsterCorpse.
		// Start stub returns nullptr.
		ARogueMonsterCorpse* Corpse = CorpseSS->FetchCorpse(FoundAIChar, FoundMonsterData);
		if (!TestNotNull(TEXT("B14: FetchCorpse must return a non-null ARogueMonsterCorpse"), Corpse))
			return true;

		// B11 DIRECT: corpse must be placed at the dead actor's world location.
		const FVector CorpseLoc = Corpse->GetActorLocation();
		TestTrue(
			FString::Printf(
				TEXT("B11: Corpse location (%.0f,%.0f,%.0f) must be within 150 cm of actor location (%.0f,%.0f,%.0f)"),
				CorpseLoc.X, CorpseLoc.Y, CorpseLoc.Z,
				AILocation.X, AILocation.Y, AILocation.Z),
			FVector::Dist(CorpseLoc, AILocation) < 150.f);

		// B13 DIRECT: FetchCorpse must increment CurrentCorpseCount.
		// Start stub: returns nullptr, count never incremented.
		TestTrue(TEXT("B13: CurrentCorpseCount must be > 0 after FetchCorpse"),
			CorpseSystemPrivate::GetCorpseCount(CorpseSS) > 0);

		// B1 DIRECT: SetCorpseProperties must copy the skeletal mesh from the source character.
		// Start stub: SetCorpseProperties is a no-op → mesh stays null.
		USkeletalMeshComponent* RefMesh  = FoundAIChar->GetMesh();
		USkeletalMeshComponent* CorpseMesh = Corpse->GetMesh();
		if (TestNotNull(TEXT("B1: Corpse MeshComp must be valid"), CorpseMesh) &&
			TestNotNull(TEXT("B1: Source mesh asset must be non-null"), RefMesh->GetSkeletalMeshAsset()))
		{
			TestEqual(
				TEXT("B1: Corpse skeletal mesh asset must match the source character's mesh asset"),
				CorpseMesh->GetSkeletalMeshAsset(),
				RefMesh->GetSkeletalMeshAsset());
		}

		// B4 DIRECT: SetCorpseProperties must call SetAllBodiesSimulatePhysics(true).
		// Start stub: empty → physics never enabled.
		if (CorpseMesh)
		{
			TestTrue(TEXT("B4: Corpse mesh must be simulating physics after SetCorpseProperties"),
				CorpseMesh->IsSimulatingPhysics());
		}

		// B3 DIRECT: SetCorpseProperties must snapshot the live character's pose and store it
		// on the corpse AnimInstance under the name "InitialPose" so it starts in the death
		// pose rather than a T-pose.
		// Solution: AddPoseSnapshot("InitialPose") then ReferenceMeshComp->SnapshotPose(NewPose).
		// Start stub: SetCorpseProperties is empty → snapshot never created → GetPoseSnapshot
		// returns false.
		if (CorpseMesh)
		{
			if (UAnimInstance* CorpseAnim = CorpseMesh->GetAnimInstance())
			{
				const FPoseSnapshot* SnapResult = CorpseAnim->GetPoseSnapshot(FName("InitialPose"));
				TestNotNull(
					TEXT("B3: Corpse AnimInstance must have a pose snapshot named 'InitialPose' after SetCorpseProperties"),
					SnapResult);
			}
			else
			{
				AddWarning(TEXT("B3: Corpse AnimInstance unavailable after physics enabled — snapshot check skipped."));
			}
		}

		// B2 DIRECT: SetCorpseProperties must copy position and rotation offsets from the source
		// mesh so the corpse mesh aligns with where the character was standing.
		// Solution: AddRelativeLocation(RefMesh->GetRelativeLocation(), TeleportPhysics) and
		//           AddRelativeRotation(RefMesh->GetRelativeRotation(), TeleportPhysics).
		// Start stub: SetCorpseProperties is a no-op → relative rotation stays at identity (0,0,0).
		// AI character meshes always carry a non-trivial relative rotation (typically ≈ -90° yaw),
		// making this a reliable discriminator between stub and solution.
		if (RefMesh && CorpseMesh)
		{
			const FRotator RefRelRot = RefMesh->GetRelativeRotation();
			if (!RefRelRot.IsNearlyZero(1.f))
			{
				// Check immediately after FetchCorpse (same frame) before physics steps.
				const FRotator CorpseRelRot = CorpseMesh->GetRelativeRotation();
				TestTrue(
					FString::Printf(
						TEXT("B2: Corpse mesh relative rotation (P=%.1f Y=%.1f R=%.1f) must match source mesh (P=%.1f Y=%.1f R=%.1f) — rotation offset was not copied"),
						CorpseRelRot.Pitch, CorpseRelRot.Yaw, CorpseRelRot.Roll,
						RefRelRot.Pitch, RefRelRot.Yaw, RefRelRot.Roll),
					RefRelRot.Equals(CorpseRelRot, 5.f));
			}
			else
			{
				AddWarning(TEXT("B2: Source mesh has near-zero relative rotation; rotation-offset check skipped (partial coverage only)."));
			}
		}

		// B5 DIRECT: SetCorpseProperties must call SetOverlayMaterial(MonsterData->HitFlashMaterial).
		// Start stub: SetCorpseProperties is a no-op → overlay material stays null.
		// B12 DIRECT (via B5): FetchCorpse configures the corpse with monster data — confirmed by
		// the overlay material being set from MonsterData->HitFlashMaterial.
		if (CorpseMesh)
		{
			TestNotNull(
				TEXT("B5/B12: Corpse mesh overlay material must be non-null after SetCorpseProperties "
					"(MonsterData->HitFlashMaterial was applied)"),
				CorpseMesh->GetOverlayMaterial());
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 5: B17, B15, B16 — CleanupNextAvailableCorpse removes exactly one
	// entry per call when age and screen conditions are met.
	//
	// Setup: drain any existing queue entries, set MinimumAge=0 (all entries
	// immediately eligible), enqueue MaxCorpses+2 actors placed far below the
	// level (WasRecentlyRendered() == false), set CurrentCorpseCount to exceed
	// the hard limit (> MaxCorpses) so every entry satisfies the removal gate.
	//
	// Then call CleanupNextAvailableCorpse twice via the robber and verify
	// count decrements by exactly 1 each time (not all at once).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 5 — cleanup-one-per-call)"), World)) return true;

		URogueMonsterCorpseSubsystem* CorpseSS = World->GetSubsystem<URogueMonsterCorpseSubsystem>();
		if (!TestNotNull(TEXT("URogueMonsterCorpseSubsystem must be valid (Phase 5)"), CorpseSS)) return true;

		URogueActorPoolingSubsystem* Pooler = World->GetSubsystem<URogueActorPoolingSubsystem>();
		if (!TestNotNull(TEXT("URogueActorPoolingSubsystem must be valid (Phase 5)"), Pooler)) return true;

		// Clean slate: drain any entries left from Phase 4
		CorpseTestHelpers::DrainCorpsesQueue(CorpseSS);

		// MinimumAge = 0 → any entry with TimeAdded <= WorldTime is eligible (always true)
		CorpseSystemPrivate::GetMinimumAge(CorpseSS) = 0.0f;

		const int32 MaxCorpses = CorpseSystemPrivate::GetMaxCorpses(CorpseSS);
		const int32 NumToEnqueue = MaxCorpses + 2; // count > MaxCorpses triggers hard-limit path

		const bool bEnqueued = CorpseTestHelpers::EnqueueTestCorpses(World, CorpseSS, Pooler, NumToEnqueue);
		if (!TestTrue(TEXT("Phase 5 setup: all test corpse actors must be acquired"), bEnqueued))
			return true;

		// Snapshot count before any cleanup
		CountBeforeCleanupCall = CorpseSystemPrivate::GetCorpseCount(CorpseSS);

		// ---- First call: must remove exactly 1 ----
		CorpseSystemPrivate::CallCleanup(CorpseSS);
		const int32 AfterFirst = CorpseSystemPrivate::GetCorpseCount(CorpseSS);

		// B16 DIRECT: eligible corpse (age met + count > MaxCorpses) must be released.
		// Start stub: CleanupNextAvailableCorpse is empty → count unchanged → fails.
		TestTrue(TEXT("B16: At least one eligible corpse must be released after CleanupNextAvailableCorpse"),
			AfterFirst < CountBeforeCleanupCall);

		// B17 DIRECT: exactly one corpse removed, not all.
		// Wrong impl that drains whole queue: AfterFirst == 0 → fails.
		TestEqual(
			TEXT("B17: CleanupNextAvailableCorpse must remove exactly one corpse per call"),
			AfterFirst, CountBeforeCleanupCall - 1);

		// ---- Second call: must remove exactly 1 more ----
		CorpseSystemPrivate::CallCleanup(CorpseSS);
		const int32 AfterSecond = CorpseSystemPrivate::GetCorpseCount(CorpseSS);

		TestEqual(
			TEXT("B17: Second CleanupNextAvailableCorpse call must again remove exactly one corpse"),
			AfterSecond, AfterFirst - 1);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 6: B18 — Tick calls CleanupNextAvailableCorpse once per frame
	// Refill the queue, wait 0.1 s (≥ 3 engine ticks at 30 fps), verify that
	// the count decreased — confirming Tick invokes the cleanup function.
	// Start stub: Tick only calls Super::Tick → count never changes → fails.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!World) return true;

		URogueMonsterCorpseSubsystem* CorpseSS = World->GetSubsystem<URogueMonsterCorpseSubsystem>();
		if (!CorpseSS) return true;

		URogueActorPoolingSubsystem* Pooler = World->GetSubsystem<URogueActorPoolingSubsystem>();
		if (!Pooler) return true;

		// Drain whatever Phase 5 left in the queue
		CorpseTestHelpers::DrainCorpsesQueue(CorpseSS);

		// MinimumAge still 0 from Phase 5 — keep it so
		const int32 MaxCorpses = CorpseSystemPrivate::GetMaxCorpses(CorpseSS);
		const int32 NumToEnqueue = MaxCorpses + 2;

		CorpseTestHelpers::EnqueueTestCorpses(World, CorpseSS, Pooler, NumToEnqueue);

		// Record count so we can measure the delta after the engine ticks
		CountBeforeCleanupCall = CorpseSystemPrivate::GetCorpseCount(CorpseSS);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // let engine call Tick multiple times

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = CorpseTestHelpers::GetPIEWorld();
		if (!World) return true;

		URogueMonsterCorpseSubsystem* CorpseSS = World->GetSubsystem<URogueMonsterCorpseSubsystem>();
		if (!CorpseSS) return true;

		const int32 CountAfterWait = CorpseSystemPrivate::GetCorpseCount(CorpseSS);

		// B18 DIRECT: Tick must call CleanupNextAvailableCorpse so the count goes down.
		// Start stub (Tick only calls Super::Tick): count never changes.
		TestTrue(
			TEXT("B18: CurrentCorpseCount must decrease after waiting multiple frames "
				"(Tick must call CleanupNextAvailableCorpse each frame)"),
			CountAfterWait < CountBeforeCleanupCall);

		// B17 sanity: at 30 fps over 0.1 s we expect at most ~3 removals, not a complete drain.
		// This confirms the subsystem is not batching all cleanups in one Tick.
		const int32 MaxExpectedRemovals = 10; // generous upper bound
		TestTrue(
			FString::Printf(
				TEXT("B17/B18: At most %d cleanups expected in 0.1 s; got %d "
					"(cleanups must not all happen in one frame)"),
				MaxExpectedRemovals, CountBeforeCleanupCall - CountAfterWait),
			(CountBeforeCleanupCall - CountAfterWait) <= MaxExpectedRemovals);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

#else
	AddWarning(TEXT("Editor-only PIE test skipped."));
#endif
	return true;
}
