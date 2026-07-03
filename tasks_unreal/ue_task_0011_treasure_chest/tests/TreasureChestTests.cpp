// Copyright 2024 ActionRoguelike Project. All Rights Reserved.
// TreasureChestTests.cpp — Automation tests for ARogueTreasureChest

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"

#include "World/RogueTreasureChest.h"
#include "Animation/RogueCurveAnimSubsystem.h"


// ===========================================================================
// Runtime — B1: VFX and sound must not be active immediately after the chest
//   spawns into a live world.  IsActive() is the behavioral observable: if the
//   constructor forgets bAutoActivate=false the component activates on
//   BeginPlay and IsActive() returns true here.
//   Also covers G2 (bAutoManageAttachment) and G3 (BodyInstance physics flag)
//   on the live spawned instance as partial checks.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestSpawnStateTest,
	"ActionRoguelike.TreasureChest.VFXAndSoundNotAutoActivateOnSpawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestSpawnStateTest::RunTest(const FString& Parameters)
{
	// Bootstrap the shared test world (defined below helpers; forward-declare usage here).
	// We create the world inline because helpers aren't declared yet at this point in the
	// translation unit — replicate the minimal bootstrap logic directly.
	if (!GEngine) { return false; }

	const FName WorldName = MakeUniqueObjectName(
		GetTransientPackage(), UWorld::StaticClass(), TEXT("TreasureChestSpawnStateWorld"));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!TestNotNull(TEXT("Test world must be created"), World)) { return false; }

	FWorldContext& WCtx = GEngine->CreateNewWorldContext(EWorldType::Game);
	WCtx.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARogueTreasureChest* Chest = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, SP);

	if (!TestNotNull(TEXT("Chest must spawn in test world"), Chest))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}

	UClass* ChestClass = Chest->GetClass();

	// B1 — behavioral: components must NOT be active right after spawn.
	if (FObjectPropertyBase* EffP = CastField<FObjectPropertyBase>(ChestClass->FindPropertyByName(TEXT("OpenChestEffect"))))
	{
		if (UNiagaraComponent* Fx = Cast<UNiagaraComponent>(EffP->GetObjectPropertyValue_InContainer(Chest)))
		{
			TestFalse(TEXT("B1: OpenChestEffect must not be active immediately after spawn"),
				Fx->IsActive());
		}
	}
	if (FObjectPropertyBase* SndP = CastField<FObjectPropertyBase>(ChestClass->FindPropertyByName(TEXT("OpenChestSound"))))
	{
		if (UAudioComponent* Snd = Cast<UAudioComponent>(SndP->GetObjectPropertyValue_InContainer(Chest)))
		{
			TestFalse(TEXT("B1: OpenChestSound must not be active immediately after spawn"),
				Snd->IsActive());
		}
	}

	// B2 — partial: bAutoManageAttachment on live instance.
	if (FObjectPropertyBase* EffP2 = CastField<FObjectPropertyBase>(ChestClass->FindPropertyByName(TEXT("OpenChestEffect"))))
	{
		if (UNiagaraComponent* Fx2 = Cast<UNiagaraComponent>(EffP2->GetObjectPropertyValue_InContainer(Chest)))
		{
			TestTrue(TEXT("B2: OpenChestEffect->bAutoManageAttachment must be true on spawned instance"),
				Fx2->bAutoManageAttachment);
		}
	}

	// B3 — partial: physics flag on live instance.
	if (FObjectPropertyBase* MshP = CastField<FObjectPropertyBase>(ChestClass->FindPropertyByName(TEXT("BaseMesh"))))
	{
		if (UStaticMeshComponent* BM = Cast<UStaticMeshComponent>(MshP->GetObjectPropertyValue_InContainer(Chest)))
		{
			TestTrue(TEXT("B3: BaseMesh->BodyInstance.bSimulatePhysics must be true on spawned instance"),
				BM->BodyInstance.bSimulatePhysics);
		}
	}

	Chest->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}


// ===========================================================================
// CDO — B2: VFX component must have bAutoManageAttachment=true.
//   This detaches the component while inactive, skipping transform updates
//   when the chest moves. Default is false; stub leaves it false.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestVFXAutoManageAttachmentTest,
	"ActionRoguelike.TreasureChest.VFXAutoManagesAttachment",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestVFXAutoManageAttachmentTest::RunTest(const FString& Parameters)
{
	UClass* ChestClass = ARogueTreasureChest::StaticClass();
	if (!TestNotNull(TEXT("ARogueTreasureChest class must exist"), ChestClass)) { return false; }

	const ARogueTreasureChest* CDO = GetDefault<ARogueTreasureChest>();
	if (!TestNotNull(TEXT("ARogueTreasureChest CDO must exist"), CDO)) { return false; }

	FObjectPropertyBase* EffectProp = CastField<FObjectPropertyBase>(
		ChestClass->FindPropertyByName(TEXT("OpenChestEffect")));
	if (!TestNotNull(TEXT("OpenChestEffect UPROPERTY must exist"), EffectProp)) { return false; }

	UNiagaraComponent* Effect = Cast<UNiagaraComponent>(
		EffectProp->GetObjectPropertyValue_InContainer(CDO));
	if (!TestNotNull(TEXT("OpenChestEffect must be a UNiagaraComponent"), Effect)) { return false; }

	TestTrue(
		TEXT("B2: OpenChestEffect->bAutoManageAttachment must be true — detach while inactive for perf"),
		Effect->bAutoManageAttachment);

	return true;
}


// ===========================================================================
// CDO — B3: BaseMesh must be configured to simulate physics.
//   Default bSimulatePhysics is false; stub leaves it false.
//   BodyInstance.bSimulatePhysics is checked directly because IsSimulatingPhysics()
//   returns false in a transient world with no physics body instantiated.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestBaseMeshSimulatesPhysicsTest,
	"ActionRoguelike.TreasureChest.BaseMeshSimulatesPhysics",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestBaseMeshSimulatesPhysicsTest::RunTest(const FString& Parameters)
{
	UClass* ChestClass = ARogueTreasureChest::StaticClass();
	if (!TestNotNull(TEXT("ARogueTreasureChest class must exist"), ChestClass)) { return false; }

	const ARogueTreasureChest* CDO = GetDefault<ARogueTreasureChest>();
	if (!TestNotNull(TEXT("ARogueTreasureChest CDO must exist"), CDO)) { return false; }

	FObjectPropertyBase* MeshProp = CastField<FObjectPropertyBase>(
		ChestClass->FindPropertyByName(TEXT("BaseMesh")));
	if (!TestNotNull(TEXT("BaseMesh UPROPERTY must exist"), MeshProp)) { return false; }

	UStaticMeshComponent* BaseMesh = Cast<UStaticMeshComponent>(
		MeshProp->GetObjectPropertyValue_InContainer(CDO));
	if (!TestNotNull(TEXT("BaseMesh must be a UStaticMeshComponent"), BaseMesh)) { return false; }

	TestTrue(
		TEXT("B3: BaseMesh->BodyInstance.bSimulatePhysics must be true — chest must simulate physics"),
		BaseMesh->BodyInstance.bSimulatePhysics);

	return true;
}


// ===========================================================================
// Runtime test helpers — shared by the remaining tests.
// ===========================================================================
namespace TreasureChestTestHelpers
{
	// Cached to avoid repeated world teardown + GC, which is a common source
	// of editor crashes in the automation harness (mirrors ExplosiveBarrel approach).
	static TWeakObjectPtr<UWorld> GCachedTestWorld;

	static UWorld* GetOrCreateTestWorld(bool& bOutCreatedWorld)
	{
		bOutCreatedWorld = false;

		if (UWorld* Existing = AutomationCommon::GetAnyGameWorld())
		{
			return Existing;
		}

		if (GCachedTestWorld.IsValid())
		{
			bOutCreatedWorld = true;
			return GCachedTestWorld.Get();
		}

		if (!GEngine) { return nullptr; }

		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(), UWorld::StaticClass(), TEXT("TreasureChestTestWorld"));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World) { return nullptr; }

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();

		GCachedTestWorld = World;
		bOutCreatedWorld = true;
		return World;
	}

	static void TeardownTestWorld(UWorld* /*World*/, bool /*bCreatedWorld*/)
	{
		// Keep the transient world alive across tests. Repeated world teardown + GC
		// is the most common source of automation harness crashes.
	}

	// Linear curve: (0.0s, 0.0°) → (1.0s, 90.0°).
	// After ticking 0.5s, lid pitch should be ~45°, clearly non-zero.
	static UCurveFloat* MakeTestCurve()
	{
		UCurveFloat* Curve = NewObject<UCurveFloat>(
			GetTransientPackage(), UCurveFloat::StaticClass(), NAME_None, RF_Transient);
		Curve->FloatCurve.AddKey(0.0f, 0.0f);
		Curve->FloatCurve.AddKey(1.0f, 90.0f);
		return Curve;
	}

	template <typename T>
	static T* GetSubobjectByProp(AActor* Actor, FName PropName)
	{
		FObjectPropertyBase* Prop = CastField<FObjectPropertyBase>(
			Actor->GetClass()->FindPropertyByName(PropName));
		if (!Prop) { return nullptr; }
		return Cast<T>(Prop->GetObjectPropertyValue_InContainer(Actor));
	}

	static void SetChestCurve(ARogueTreasureChest* Chest, UCurveFloat* Curve)
	{
		if (FObjectPropertyBase* Prop = CastField<FObjectPropertyBase>(
			Chest->GetClass()->FindPropertyByName(TEXT("LidAnimCurve"))))
		{
			Prop->SetObjectPropertyValue_InContainer(Chest, Curve);
		}
	}

	static void SetLidOpenedFlag(ARogueTreasureChest* Chest, bool bValue)
	{
		if (FBoolProperty* Prop = CastField<FBoolProperty>(
			Chest->GetClass()->FindPropertyByName(TEXT("bLidOpened"))))
		{
			Prop->SetPropertyValue_InContainer(Chest, bValue);
		}
	}

	static bool GetLidOpenedFlag(const ARogueTreasureChest* Chest)
	{
		FBoolProperty* Prop = CastField<FBoolProperty>(
			Chest->GetClass()->FindPropertyByName(TEXT("bLidOpened")));
		return Prop && Prop->GetPropertyValue_InContainer(Chest);
	}

	static ARogueTreasureChest* SpawnChest(UWorld* World, const FVector& Location)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ARogueTreasureChest>(
			ARogueTreasureChest::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
	}
}


// ===========================================================================
// Runtime — B4 + B5 + B6: Interact sets the open flag, runs the open sequence,
//   and the curve anim subsystem drives the lid's pitch rotation.
//
//   UNiagaraComponent / UAudioComponent IsActive() cannot be observed without
//   real assets bound at runtime, so the lid mesh pitch is the load-bearing
//   observable for B5/B6. The subsystem is ticked directly (FTickableGameObject)
//   rather than through a full world tick for speed and isolation.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestInteractTriggersOpenSequenceTest,
	"ActionRoguelike.TreasureChest.InteractTriggersOpenSequence",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestInteractTriggersOpenSequenceTest::RunTest(const FString& Parameters)
{
	using namespace TreasureChestTestHelpers;

	bool bCreated = false;
	UWorld* World = GetOrCreateTestWorld(bCreated);
	if (!TestNotNull(TEXT("Test world must exist"), World)) { return false; }

	ARogueTreasureChest* Chest = SpawnChest(World, FVector(0.f, 0.f, 200.f));
	if (!TestNotNull(TEXT("Chest must spawn"), Chest))
	{
		TeardownTestWorld(World, bCreated);
		return false;
	}

	SetChestCurve(Chest, MakeTestCurve());

	URogueCurveAnimSubsystem* AnimSub = World->GetSubsystem<URogueCurveAnimSubsystem>();
	if (!TestNotNull(TEXT("URogueCurveAnimSubsystem must exist in world"), AnimSub))
	{
		Chest->Destroy();
		TeardownTestWorld(World, bCreated);
		return false;
	}

	UStaticMeshComponent* LidMesh = GetSubobjectByProp<UStaticMeshComponent>(Chest, TEXT("LidMesh"));
	if (!TestNotNull(TEXT("LidMesh component must exist"), LidMesh))
	{
		Chest->Destroy();
		TeardownTestWorld(World, bCreated);
		return false;
	}

	// B4: chest starts closed.
	TestFalse(TEXT("B4: bLidOpened must be false before any Interact"), GetLidOpenedFlag(Chest));

	// B4: first Interact must mark it as opened.
	Chest->Interact_Implementation(nullptr);
	TestTrue(TEXT("B4: Interact must set bLidOpened=true"), GetLidOpenedFlag(Chest));

	// B5 / B6: advancing the curve anim subsystem must rotate the lid on its pitch axis.
	// (VFX activate / sound play are also part of B5 but are not observable without assets.)
	static_cast<FTickableGameObject*>(AnimSub)->Tick(0.5f);

	const FRotator LidRot = LidMesh->GetRelativeRotation();
	TestFalse(TEXT("B5/B6: Lid pitch must rotate — curve anim subsystem must drive SetRelativeRotation each frame"),
		FMath::IsNearlyZero(LidRot.Pitch, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("B6: Lid rotation must be on the pitch axis only — yaw must be zero"),
		FMath::IsNearlyZero(LidRot.Yaw, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("B6: Lid rotation must be on the pitch axis only — roll must be zero"),
		FMath::IsNearlyZero(LidRot.Roll, KINDA_SMALL_NUMBER));

	// B4 (idempotent end-state): second Interact must leave the chest open.
	Chest->Interact_Implementation(nullptr);
	TestTrue(TEXT("B4: bLidOpened must remain true after a second Interact"),
		GetLidOpenedFlag(Chest));

	Chest->Destroy();
	TeardownTestWorld(World, bCreated);
	return true;
}


// ===========================================================================
// Runtime — B4 guard: ConditionalOpenChest must be a no-op when bLidOpened is
//   false.  Both OnRep_LidOpened and OnActorLoaded delegate to
//   ConditionalOpenChest, so an unguarded implementation would play the open
//   sequence even for a chest that was never opened (wrong solution #5).
//   Observable: after calling OnRep with bLidOpened=false, ticking the subsystem
//   must NOT rotate the lid.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestConditionalOpenGuardTest,
	"ActionRoguelike.TreasureChest.ConditionalOpenGuardBlocksWhenClosed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestConditionalOpenGuardTest::RunTest(const FString& Parameters)
{
	using namespace TreasureChestTestHelpers;

	bool bCreated = false;
	UWorld* World = GetOrCreateTestWorld(bCreated);
	if (!TestNotNull(TEXT("Test world must exist"), World)) { return false; }

	ARogueTreasureChest* Chest = SpawnChest(World, FVector(800.f, 0.f, 200.f));
	if (!TestNotNull(TEXT("Chest must spawn"), Chest))
	{
		TeardownTestWorld(World, bCreated);
		return false;
	}

	// Provide a valid curve so an unguarded ConditionalOpenChest doesn't crash.
	SetChestCurve(Chest, MakeTestCurve());
	// bLidOpened defaults to false — do NOT set it true.

	URogueCurveAnimSubsystem* AnimSub = World->GetSubsystem<URogueCurveAnimSubsystem>();
	if (!TestNotNull(TEXT("URogueCurveAnimSubsystem must exist"), AnimSub))
	{
		Chest->Destroy();
		TeardownTestWorld(World, bCreated);
		return false;
	}

	UStaticMeshComponent* LidMesh = GetSubobjectByProp<UStaticMeshComponent>(Chest, TEXT("LidMesh"));
	if (!TestNotNull(TEXT("LidMesh must exist"), LidMesh))
	{
		Chest->Destroy();
		TeardownTestWorld(World, bCreated);
		return false;
	}

	// Dispatch OnRep_LidOpened with bLidOpened=false — simulates initial prop sync
	// on a client before any open has occurred.
	UFunction* OnRepFn = Chest->FindFunction(TEXT("OnRep_LidOpened"));
	if (TestNotNull(TEXT("OnRep_LidOpened UFUNCTION must be discoverable"), OnRepFn))
	{
		Chest->ProcessEvent(OnRepFn, nullptr);
	}

	static_cast<FTickableGameObject*>(AnimSub)->Tick(0.5f);

	// If ConditionalOpenChest has no bLidOpened guard, the lid pitch will be non-zero here.
	TestTrue(
		TEXT("B4 guard: Lid pitch must remain zero — ConditionalOpenChest must return early when bLidOpened is false"),
		FMath::IsNearlyZero(LidMesh->GetRelativeRotation().Pitch, KINDA_SMALL_NUMBER));

	// Positive assertion: with bLidOpened=true, OnRep must trigger the open sequence and rotate the lid.
	// This fails on start/ (ConditionalOpenChest is an empty stub) and passes on solution/.
	SetLidOpenedFlag(Chest, true);
	UFunction* OnRepFnOpen = Chest->FindFunction(TEXT("OnRep_LidOpened"));
	if (TestNotNull(TEXT("OnRep_LidOpened UFUNCTION must be discoverable (positive path)"), OnRepFnOpen))
	{
		Chest->ProcessEvent(OnRepFnOpen, nullptr);
	}
	static_cast<FTickableGameObject*>(AnimSub)->Tick(0.5f);

	TestFalse(
		TEXT("B4 guard (positive): ConditionalOpenChest must run the open sequence when bLidOpened is true — lid pitch must rotate"),
		FMath::IsNearlyZero(LidMesh->GetRelativeRotation().Pitch, KINDA_SMALL_NUMBER));

	Chest->Destroy();
	TeardownTestWorld(World, bCreated);
	return true;
}


// ===========================================================================
// Runtime — B7 + B8: replication callback and save/load both replay the open
//   sequence.
//
//   B7: Simulate the client-side replication event — bLidOpened is set true
//       (as if the property just arrived from the server) and OnRep_LidOpened
//       is dispatched.  The open sequence must run so the client matches the
//       server.
//
//   B8: Simulate save/load restore — bLidOpened is true in the save data.
//       OnActorLoaded_Implementation must call ConditionalOpenChest so a
//       previously-opened chest reappears open after loading.
//
//   Both phases are observed via the lid's pitch rotation after a subsystem tick.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTreasureChestRepAndLoadReplayTest,
	"ActionRoguelike.TreasureChest.RepAndLoadReplayOpenSequence",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTreasureChestRepAndLoadReplayTest::RunTest(const FString& Parameters)
{
	using namespace TreasureChestTestHelpers;

	bool bCreated = false;
	UWorld* World = GetOrCreateTestWorld(bCreated);
	if (!TestNotNull(TEXT("Test world must exist"), World)) { return false; }

	URogueCurveAnimSubsystem* AnimSub = World->GetSubsystem<URogueCurveAnimSubsystem>();
	if (!TestNotNull(TEXT("URogueCurveAnimSubsystem must exist"), AnimSub))
	{
		TeardownTestWorld(World, bCreated);
		return false;
	}

	// Spawn helper: pre-stages bLidOpened=true without running the open sequence,
	// as would be the case on a client (rep) or after deserialization (load).
	auto SpawnPreOpenedChest = [&](const FVector& Location) -> ARogueTreasureChest*
	{
		ARogueTreasureChest* C = SpawnChest(World, Location);
		if (C)
		{
			SetChestCurve(C, MakeTestCurve());
			SetLidOpenedFlag(C, true);
		}
		return C;
	};

	// ---- B7: OnRep_LidOpened must replay the open sequence on a simulated client ----
	{
		ARogueTreasureChest* RepChest = SpawnPreOpenedChest(FVector(0.f, 1000.f, 200.f));
		if (!TestNotNull(TEXT("B7: Replicated chest must spawn"), RepChest))
		{
			TeardownTestWorld(World, bCreated);
			return false;
		}

		UStaticMeshComponent* RepLid =
			GetSubobjectByProp<UStaticMeshComponent>(RepChest, TEXT("LidMesh"));
		UFunction* OnRepFn = RepChest->FindFunction(TEXT("OnRep_LidOpened"));

		if (TestNotNull(TEXT("B7: OnRep_LidOpened UFUNCTION must be discoverable"), OnRepFn)
			&& TestNotNull(TEXT("B7: LidMesh must exist on replicated chest"), RepLid))
		{
			RepChest->ProcessEvent(OnRepFn, nullptr);
			static_cast<FTickableGameObject*>(AnimSub)->Tick(0.5f);

			TestFalse(
				TEXT("B7: OnRep_LidOpened must replay the open sequence — lid pitch must rotate after subsystem tick"),
				FMath::IsNearlyZero(RepLid->GetRelativeRotation().Pitch, KINDA_SMALL_NUMBER));
		}

		RepChest->Destroy();
	}

	// ---- B8: OnActorLoaded must restore the open visual state after save/load ----
	{
		ARogueTreasureChest* LoadChest = SpawnPreOpenedChest(FVector(0.f, 2000.f, 200.f));
		if (!TestNotNull(TEXT("B8: Loaded chest must spawn"), LoadChest))
		{
			TeardownTestWorld(World, bCreated);
			return false;
		}

		UStaticMeshComponent* LoadLid =
			GetSubobjectByProp<UStaticMeshComponent>(LoadChest, TEXT("LidMesh"));

		if (TestNotNull(TEXT("B8: LidMesh must exist on loaded chest"), LoadLid))
		{
			LoadChest->OnActorLoaded_Implementation();
			static_cast<FTickableGameObject*>(AnimSub)->Tick(0.5f);

			TestFalse(
				TEXT("B8: OnActorLoaded must replay the open sequence — lid pitch must rotate after subsystem tick"),
				FMath::IsNearlyZero(LoadLid->GetRelativeRotation().Pitch, KINDA_SMALL_NUMBER));
		}

		LoadChest->Destroy();
	}

	TeardownTestWorld(World, bCreated);
	return true;
}
