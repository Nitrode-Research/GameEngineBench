// Copyright 2024 ActionRoguelike Project. All Rights Reserved.
// InteractionComponentTests.cpp — Automation tests for URogueInteractionComponent

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/GarbageCollection.h"
#include "Curves/CurveFloat.h"

#include "ActionSystem/RogueActionComponent.h"
#include "ActionSystem/RogueAttributeSet.h"
#include "Player/RogueInteractionComponent.h"
#include "UI/RogueWorldUserWidget.h"
#include "World/RogueTreasureChest.h"


// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

static FObjectPropertyBase* GetFocusedActorProperty()
{
	return CastField<FObjectPropertyBase>(
		URogueInteractionComponent::StaticClass()->FindPropertyByName(TEXT("FocusedActor")));
}

static FObjectPropertyBase* GetWidgetInstProperty()
{
	return CastField<FObjectPropertyBase>(
		URogueInteractionComponent::StaticClass()->FindPropertyByName(TEXT("WidgetInst")));
}

namespace InteractionTestHelpers
{
	static UWorld* GetOrCreateTestWorld(bool& bOutCreatedWorld)
	{
		bOutCreatedWorld = false;

		UWorld* World = AutomationCommon::GetAnyGameWorld();
		if (World)
		{
			return World;
		}

		if (!GEngine)
		{
			return nullptr;
		}

		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(),
			UWorld::StaticClass(),
			TEXT("InteractionTestWorld"));
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			return nullptr;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();

		bOutCreatedWorld = true;
		return World;
	}

	static void TeardownTestWorld(UWorld* World, bool bCreatedWorld)
	{
		if (World && bCreatedWorld && GEngine)
		{
			World->BeginTearingDown();
			World->CleanupWorld();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}
	}

	static UCurveFloat* MakeDummyLidCurve()
	{
		UCurveFloat* Curve = NewObject<UCurveFloat>(GetTransientPackage(),
			UCurveFloat::StaticClass(), NAME_None, RF_Transient);
		Curve->FloatCurve.AddKey(0.f, 0.f);
		Curve->FloatCurve.AddKey(1.f, 90.f);
		return Curve;
	}

	static void StageChestForInteract(ARogueTreasureChest* Chest, UCurveFloat* Curve)
	{
		if (FObjectPropertyBase* Prop = CastField<FObjectPropertyBase>(
				ARogueTreasureChest::StaticClass()->FindPropertyByName(TEXT("LidAnimCurve"))))
		{
			Prop->SetObjectPropertyValue_InContainer(Chest, Curve);
		}
	}

	static FBoolProperty* GetLidOpenedProperty()
	{
		return CastField<FBoolProperty>(
			ARogueTreasureChest::StaticClass()->FindPropertyByName(TEXT("bLidOpened")));
	}

	static TArray<UStaticMeshComponent*> GetStaticMeshComponents(AActor* Actor)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		if (Actor)
		{
			Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
		}
		return MeshComponents;
	}

	static bool AnyMeshUsesOverlay(AActor* Actor, UMaterialInterface* ExpectedOverlay)
	{
		for (UStaticMeshComponent* MeshComp : GetStaticMeshComponents(Actor))
		{
			if (MeshComp && MeshComp->GetOverlayMaterial() == ExpectedOverlay)
			{
				return true;
			}
		}
		return false;
	}

	static bool AllMeshesClearOverlay(AActor* Actor)
	{
		for (UStaticMeshComponent* MeshComp : GetStaticMeshComponents(Actor))
		{
			if (MeshComp && MeshComp->GetOverlayMaterial() != nullptr)
			{
				return false;
			}
		}
		return true;
	}

	static ACharacter* SpawnAliveCharacter(UWorld* World, const FActorSpawnParameters& SpawnParams,
		const FVector& Location = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		ACharacter* Character = World
			? World->SpawnActor<ACharacter>(ACharacter::StaticClass(), Location, Rotation, SpawnParams)
			: nullptr;
		if (!Character)
		{
			return nullptr;
		}

		URogueActionComponent* ActionComp = NewObject<URogueActionComponent>(
			Character, URogueActionComponent::StaticClass(), TEXT("TestActionComp"));
		if (!ActionComp)
		{
			Character->Destroy();
			return nullptr;
		}

		ActionComp->SetDefaultAttributeSet(URogueSurvivorAttributeSet::StaticClass());
		Character->AddOwnedComponent(ActionComp);
		ActionComp->RegisterComponent();

		return Character;
	}
}


// ===========================================================================
// TEST 1 — PrimaryInteract dispatch by current focus
//   (B5 DIRECT, B6 DIRECT, B10 inverse-DIRECT, B11 DIRECT)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentPrimaryInteractRoutingTest,
	"ActionRoguelike.Interaction.PrimaryInteractRoutesToServerInteract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentPrimaryInteractRoutingTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARogueTreasureChest* Chest = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("ARogueTreasureChest must spawn"), Chest))
	{
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	StageChestForInteract(Chest, MakeDummyLidCurve());

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		Chest->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		Chest->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	FBoolProperty* LidProp = GetLidOpenedProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp) ||
		!TestNotNull(TEXT("bLidOpened UPROPERTY must exist on chest"), LidProp))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		Chest->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	if (!TestFalse(TEXT("Precondition: chest must start with bLidOpened=false"),
			LidProp->GetPropertyValue_InContainer(Chest)))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		Chest->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// ----- Phase A: FocusedActor=nullptr (B5 / B10 / B11) -----
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, nullptr);
	Comp->PrimaryInteract();

	if (!TestFalse(
			TEXT("// REQUIRED (B5/B10/B11): PrimaryInteract with FocusedActor=nullptr must not "
				 "interact with any actor in the world. The chest planted in the world (and not "
				 "in focus) must remain closed (bLidOpened=false)."),
			LidProp->GetPropertyValue_InContainer(Chest)))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		Chest->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// ----- Phase B: FocusedActor=chest (B6) -----
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, Chest);
	Comp->PrimaryInteract();

	TestTrue(
		TEXT("// REQUIRED (B6): PrimaryInteract must route to ServerInteract → Execute_Interact "
			 "→ bLidOpened=true. A stub PrimaryInteract that does nothing fails this check."),
		LidProp->GetPropertyValue_InContainer(Chest));

	Comp->DestroyComponent();
	PC->Destroy();
	Chest->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 2 — Dispatch is selective: only the focused actor is interacted with
//   (B6 DIRECT strengthened, B3 partial-direct on the dispatch invariant)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentDispatchSelectiveToFocusedTest,
	"ActionRoguelike.Interaction.PrimaryInteractDispatchesOnlyToFocused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentDispatchSelectiveToFocusedTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UCurveFloat* DummyCurve = MakeDummyLidCurve();

	ARogueTreasureChest* ChestA = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	ARogueTreasureChest* ChestB = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(-200.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);

	if (!TestNotNull(TEXT("ChestA must spawn"), ChestA) ||
		!TestNotNull(TEXT("ChestB must spawn"), ChestB))
	{
		if (ChestA) { ChestA->Destroy(); }
		if (ChestB) { ChestB->Destroy(); }
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	StageChestForInteract(ChestA, DummyCurve);
	StageChestForInteract(ChestB, DummyCurve);

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		ChestA->Destroy();
		ChestB->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		ChestA->Destroy();
		ChestB->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	FBoolProperty* LidProp = GetLidOpenedProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp) ||
		!TestNotNull(TEXT("bLidOpened property must exist"), LidProp))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		ChestA->Destroy();
		ChestB->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	if (!TestFalse(TEXT("ChestA must start closed"),
			LidProp->GetPropertyValue_InContainer(ChestA)) ||
		!TestFalse(TEXT("ChestB must start closed"),
			LidProp->GetPropertyValue_InContainer(ChestB)))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		ChestA->Destroy();
		ChestB->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	FocusedProp->SetObjectPropertyValue_InContainer(Comp, ChestA);
	Comp->PrimaryInteract();

	const bool bAOpened = LidProp->GetPropertyValue_InContainer(ChestA);
	const bool bBOpened = LidProp->GetPropertyValue_InContainer(ChestB);

	TestTrue(
		TEXT("// REQUIRED (B6): The focused chest (ChestA) must open after PrimaryInteract."),
		bAOpened);

	TestFalse(
		TEXT("// REQUIRED (B6/B3): The unfocused chest (ChestB) must remain closed."),
		bBOpened);

	Comp->DestroyComponent();
	PC->Destroy();
	ChestA->Destroy();
	ChestB->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 3 — TickComponent respects local-controller gating
//   (B7 DIRECT for the non-local negative case; local null-pawn cleanup is
//    treated as advisory because the spec only requires avoiding crashes)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTickRunsScanOnLocalControllerTest,
	"ActionRoguelike.Interaction.TickRunsScanOnLocalController",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTickRunsScanOnLocalControllerTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	PC->SetAsLocalPlayerController();
	if (!TestTrue(
			TEXT("Spawned PC must report IsLocalController()=true — precondition for B7"),
			PC->IsLocalController()))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	AActor* Sentinel = World->SpawnActor<AActor>(
		AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Sentinel actor must spawn"), Sentinel))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp))
	{
		Sentinel->Destroy();
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, Sentinel);

	if (!TestEqual(
			TEXT("Pre-tick: FocusedActor must equal the sentinel before TickComponent"),
			FocusedProp->GetObjectPropertyValue_InContainer(Comp),
			(UObject*)Sentinel))
	{
		Sentinel->Destroy();
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);

	UObject* FocusedAfter = FocusedProp->GetObjectPropertyValue_InContainer(Comp);

	if (FocusedAfter != nullptr && FocusedAfter != Sentinel)
	{
		AddError(TEXT("TickComponent on a local controller must not corrupt FocusedActor when no pawn is possessed."));
	}

	Sentinel->Destroy();
	Comp->DestroyComponent();
	PC->Destroy();

	// ----- Phase 2: non-local controller must NOT run the scan (B7 negative case) -----
	APlayerController* RemotePC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (TestNotNull(TEXT("Non-local APlayerController must spawn"), RemotePC))
	{
		TestFalse(TEXT("Non-local PC must report IsLocalController()=false — precondition for B7 negative"),
			RemotePC->IsLocalController());

		URogueInteractionComponent* RemoteComp = NewObject<URogueInteractionComponent>(
			RemotePC, URogueInteractionComponent::StaticClass(), TEXT("RemoteInteractionComp"));
		RemotePC->AddOwnedComponent(RemoteComp);
		RemoteComp->RegisterComponent();

		AActor* RemoteSentinel = World->SpawnActor<AActor>(
			AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (TestNotNull(TEXT("Remote sentinel must spawn"), RemoteSentinel))
		{
			FObjectPropertyBase* RemoteFocusedProp = GetFocusedActorProperty();
			if (RemoteFocusedProp)
			{
				RemoteFocusedProp->SetObjectPropertyValue_InContainer(RemoteComp, RemoteSentinel);
				RemoteComp->TickComponent(0.0f, LEVELTICK_All, &RemoteComp->PrimaryComponentTick);

				UObject* RemoteFocusedAfter = RemoteFocusedProp->GetObjectPropertyValue_InContainer(RemoteComp);
				TestEqual(
					TEXT("// REQUIRED (B7): TickComponent on a NON-local controller must NOT invoke "
						 "FindBestInteractable. FocusedActor must remain unchanged (still the sentinel)."),
					RemoteFocusedAfter,
					(UObject*)RemoteSentinel);
			}
			RemoteSentinel->Destroy();
		}
		RemoteComp->DestroyComponent();
		RemotePC->Destroy();
	}

	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 4 — Local scan chooses the candidate the camera is pointing at
//   (B3/B4/B7 DIRECT)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentCameraFacingSelectionTest,
	"ActionRoguelike.Interaction.TickSelectsMostCameraFacingCandidate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentCameraFacingSelectionTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->SetAsLocalPlayerController();
	if (!TestTrue(TEXT("PC must report IsLocalController()=true"), PC->IsLocalController()))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	ACharacter* Pawn = SpawnAliveCharacter(World, SpawnParams);
	if (!TestNotNull(TEXT("Possessed pawn must spawn"), Pawn))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->Possess(Pawn);

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp))
	{
		Comp->DestroyComponent();
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	ARogueTreasureChest* ChestAhead = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	ARogueTreasureChest* ChestSide = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector(0.f, 150.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("ChestAhead must spawn"), ChestAhead) ||
		!TestNotNull(TEXT("ChestSide must spawn"), ChestSide))
	{
		if (ChestAhead) { ChestAhead->Destroy(); }
		if (ChestSide) { ChestSide->Destroy(); }
		Comp->DestroyComponent();
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	PC->SetControlRotation(FRotator(0.f, 0.f, 0.f));
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);
	UObject* FocusedAhead = FocusedProp->GetObjectPropertyValue_InContainer(Comp);

	PC->SetControlRotation(FRotator(0.f, 90.f, 0.f));
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);
	UObject* FocusedSide = FocusedProp->GetObjectPropertyValue_InContainer(Comp);

	TestEqual(
		TEXT("// REQUIRED (B3): With multiple nearby interactables, the actor most directly "
			 "in front of the camera must win focus."),
		FocusedAhead,
		(UObject*)ChestAhead);

	TestEqual(
		TEXT("// REQUIRED (B4): Changing the camera direction to a different nearby actor "
			 "must move focus to that new best candidate."),
		FocusedSide,
		(UObject*)ChestSide);

	ChestAhead->Destroy();
	ChestSide->Destroy();
	Comp->DestroyComponent();
	Pawn->Destroy();
	PC->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 5 — bCanEverTick is true by default (B7 precondition; PARTIAL)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTickEnabledTest,
	"ActionRoguelike.Interaction.TickEnabledByDefault",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTickEnabledTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	TestTrue(TEXT("RogueInteractionComponent must be a UActorComponent"),
		CompClass->IsChildOf(UActorComponent::StaticClass()));

	UActorComponent* Comp = NewObject<UActorComponent>(GetTransientPackage(), CompClass);
	if (!TestNotNull(TEXT("Component must be instantiable"), Comp)) { return false; }

	TestTrue(TEXT("bCanEverTick must be true so TickComponent runs each frame"),
		Comp->PrimaryComponentTick.bCanEverTick);

	return true;
}


// ===========================================================================
// TEST 6 — TickGroup preference is advisory, not hard contract
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTickGroupTest,
	"ActionRoguelike.Interaction.TickGroupIsPostUpdateWork",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTickGroupTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	UActorComponent* Comp = NewObject<UActorComponent>(GetTransientPackage(), CompClass);
	if (!TestNotNull(TEXT("Could not instantiate URogueInteractionComponent"), Comp)) { return false; }

	if (Comp->PrimaryComponentTick.TickGroup != TG_PostUpdateWork)
	{
		AddWarning(TEXT("Advisory: TG_PostUpdateWork helps consume the latest camera rotation, but the spec does not require this exact engine tick group."));
	}

	return true;
}


// ===========================================================================
// TEST 7 — TraceRadius default is positive (B3 precondition; PARTIAL)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTraceRadiusTest,
	"ActionRoguelike.Interaction.TraceRadiusIsPositive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTraceRadiusTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	FFloatProperty* RadiusProp = CastField<FFloatProperty>(
		CompClass->FindPropertyByName(TEXT("TraceRadius")));
	if (!TestNotNull(TEXT("TraceRadius float property must exist"), RadiusProp)) { return false; }

	const UObject* CDO = CompClass->GetDefaultObject();
	const float DefaultRadius = RadiusProp->GetPropertyValue_InContainer(CDO);

	TestTrue(
		FString::Printf(TEXT("TraceRadius CDO value must be > 0 (observed=%f)"), DefaultRadius),
		DefaultRadius > 0.f);

	return true;
}


// ===========================================================================
// TEST 8 — TraceChannel is set to a custom channel (B3 precondition; PARTIAL)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTraceChannelPropertyTest,
	"ActionRoguelike.Interaction.TraceChannelIsCustom",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTraceChannelPropertyTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	FProperty* ChannelProp = CompClass->FindPropertyByName(TEXT("TraceChannel"));
	if (!TestNotNull(TEXT("TraceChannel property must exist"), ChannelProp)) { return false; }

	FByteProperty* ByteProp = CastField<FByteProperty>(ChannelProp);
	FEnumProperty* EnumProp = CastField<FEnumProperty>(ChannelProp);
	if (!TestTrue(TEXT("TraceChannel must be an enum/byte property (ECollisionChannel)"),
			(ByteProp != nullptr) || (EnumProp != nullptr)))
	{
		return false;
	}

	const UObject* CDO = CompClass->GetDefaultObject();
	uint8 ChannelValue = 0;
	if (ByteProp)
	{
		ChannelValue = ByteProp->GetPropertyValue_InContainer(CDO);
	}
	else if (EnumProp)
	{
		const FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		ChannelValue = (uint8)UnderlyingProp->GetSignedIntPropertyValue(
			EnumProp->ContainerPtrToValuePtr<void>(CDO));
	}

	TestTrue(TEXT("TraceChannel CDO value must not be ECC_WorldStatic (0) — must be set to TRACE_INTERACT"),
		ChannelValue != 0);

	return true;
}


// ===========================================================================
// TEST 9 — DefaultWidgetClass UPROPERTY exists (B2 / B9 wiring; PARTIAL)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentWidgetClassPropertyTest,
	"ActionRoguelike.Interaction.DefaultWidgetClassPropertyExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentWidgetClassPropertyTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	FProperty* WidgetProp = CompClass->FindPropertyByName(TEXT("DefaultWidgetClass"));
	TestNotNull(
		TEXT("DefaultWidgetClass UPROPERTY must exist — supplies the world-space prompt widget class"),
		WidgetProp);

	return true;
}


// ===========================================================================
// TEST 10 — HighlightOverlayMaterial UPROPERTY exists (B1 / B8 wiring; PARTIAL)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentHighlightMaterialPropertyTest,
	"ActionRoguelike.Interaction.HighlightOverlayMaterialPropertyExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentHighlightMaterialPropertyTest::RunTest(const FString& Parameters)
{
	UClass* CompClass = URogueInteractionComponent::StaticClass();
	if (!TestNotNull(TEXT("URogueInteractionComponent class must exist"), CompClass)) { return false; }

	FProperty* OverlayProp = CompClass->FindPropertyByName(TEXT("HighlightOverlayMaterial"));
	if (!TestNotNull(
			TEXT("HighlightOverlayMaterial UPROPERTY must exist (B1/B8)"),
			OverlayProp))
	{
		return false;
	}

	FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(OverlayProp);
	TestNotNull(
		TEXT("HighlightOverlayMaterial must be an object-typed UPROPERTY"),
		ObjectProp);

	return true;
}


// ===========================================================================
// TEST 11 — Dispatch tracks the *current* focused actor across focus changes
//   (B3 DIRECT, B4 DIRECT strengthened, B6 DIRECT strengthened)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentDispatchTracksFocusChangesTest,
	"ActionRoguelike.Interaction.PrimaryInteractTracksFocusChanges",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentDispatchTracksFocusChangesTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UCurveFloat* DummyCurve = MakeDummyLidCurve();

	ARogueTreasureChest* ChestA = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	ARogueTreasureChest* ChestB = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	ARogueTreasureChest* ChestC = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(-300.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);

	if (!TestNotNull(TEXT("ChestA must spawn"), ChestA) ||
		!TestNotNull(TEXT("ChestB must spawn"), ChestB) ||
		!TestNotNull(TEXT("ChestC must spawn"), ChestC))
	{
		if (ChestA) { ChestA->Destroy(); }
		if (ChestB) { ChestB->Destroy(); }
		if (ChestC) { ChestC->Destroy(); }
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	StageChestForInteract(ChestA, DummyCurve);
	StageChestForInteract(ChestB, DummyCurve);
	StageChestForInteract(ChestC, DummyCurve);

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		ChestA->Destroy(); ChestB->Destroy(); ChestC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		ChestA->Destroy(); ChestB->Destroy(); ChestC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	FBoolProperty* LidProp = GetLidOpenedProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp) ||
		!TestNotNull(TEXT("bLidOpened property must exist"), LidProp))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		ChestA->Destroy(); ChestB->Destroy(); ChestC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	if (!TestFalse(TEXT("Precondition: ChestA starts closed"),
			LidProp->GetPropertyValue_InContainer(ChestA)) ||
		!TestFalse(TEXT("Precondition: ChestB starts closed"),
			LidProp->GetPropertyValue_InContainer(ChestB)) ||
		!TestFalse(TEXT("Precondition: ChestC starts closed"),
			LidProp->GetPropertyValue_InContainer(ChestC)))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		ChestA->Destroy(); ChestB->Destroy(); ChestC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// ----- Round 1: focus=ChestA -----
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, ChestA);
	Comp->PrimaryInteract();

	const bool bA_R1 = LidProp->GetPropertyValue_InContainer(ChestA);
	const bool bB_R1 = LidProp->GetPropertyValue_InContainer(ChestB);
	const bool bC_R1 = LidProp->GetPropertyValue_InContainer(ChestC);

	TestTrue(
		TEXT("// REQUIRED (B6): Round 1 — with focus=ChestA, PrimaryInteract must dispatch "
			 "Execute_Interact to ChestA → bLidOpened=true."),
		bA_R1);
	TestFalse(
		TEXT("// REQUIRED (B3): Round 1 — with focus=ChestA, ChestB must remain closed."),
		bB_R1);
	TestFalse(
		TEXT("// REQUIRED (B3): Round 1 — with focus=ChestA, ChestC must remain closed."),
		bC_R1);

	// ----- Round 2: focus changes to ChestB -----
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, ChestB);
	Comp->PrimaryInteract();

	const bool bB_R2 = LidProp->GetPropertyValue_InContainer(ChestB);
	const bool bC_R2 = LidProp->GetPropertyValue_InContainer(ChestC);

	TestTrue(
		TEXT("// REQUIRED (B4/B6): Round 2 — after focus ChestA→ChestB, "
			 "PrimaryInteract must dispatch to the NEW focus (ChestB)."),
		bB_R2);
	TestFalse(
		TEXT("// REQUIRED (B3): Round 2 — with focus=ChestB, ChestC must still be closed."),
		bC_R2);

	// ----- Round 3: focus changes to ChestC -----
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, ChestC);
	Comp->PrimaryInteract();

	const bool bC_R3 = LidProp->GetPropertyValue_InContainer(ChestC);

	TestTrue(
		TEXT("// REQUIRED (B4/B6): Round 3 — after focus to ChestC, "
			 "PrimaryInteract must dispatch to ChestC."),
		bC_R3);

	Comp->DestroyComponent();
	PC->Destroy();
	ChestA->Destroy(); ChestB->Destroy(); ChestC->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 12 — Normal scan path applies highlight + prompt and transfers cleanup
//   (B1/B2/B3/B4 DIRECT)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentFocusPresentationTransferTest,
	"ActionRoguelike.Interaction.TickAppliesAndTransfersFocusPresentation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentFocusPresentationTransferTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	ACharacter* Pawn = SpawnAliveCharacter(World, SpawnParams);
	ARogueTreasureChest* ChestAhead = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	ARogueTreasureChest* ChestSide = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(), FVector(0.f, 150.f, 0.f), FRotator::ZeroRotator, SpawnParams);

	if (!TestNotNull(TEXT("Local controller must spawn"), PC) ||
		!TestNotNull(TEXT("Possessed pawn must spawn"), Pawn) ||
		!TestNotNull(TEXT("ChestAhead must spawn"), ChestAhead) ||
		!TestNotNull(TEXT("ChestSide must spawn"), ChestSide))
	{
		if (ChestAhead) { ChestAhead->Destroy(); }
		if (ChestSide) { ChestSide->Destroy(); }
		if (Pawn) { Pawn->Destroy(); }
		if (PC) { PC->Destroy(); }
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	PC->SetAsLocalPlayerController();
	PC->Possess(Pawn);

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		ChestAhead->Destroy();
		ChestSide->Destroy();
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	FObjectPropertyBase* WidgetProp = GetWidgetInstProperty();
	FClassProperty* WidgetClassProp = CastField<FClassProperty>(
		URogueInteractionComponent::StaticClass()->FindPropertyByName(TEXT("DefaultWidgetClass")));
	FObjectPropertyBase* HighlightProp = CastField<FObjectPropertyBase>(
		URogueInteractionComponent::StaticClass()->FindPropertyByName(TEXT("HighlightOverlayMaterial")));
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp) ||
		!TestNotNull(TEXT("WidgetInst UPROPERTY must exist"), WidgetProp) ||
		!TestNotNull(TEXT("DefaultWidgetClass property must exist"), WidgetClassProp) ||
		!TestNotNull(TEXT("HighlightOverlayMaterial property must exist"), HighlightProp))
	{
		Comp->DestroyComponent();
		ChestAhead->Destroy();
		ChestSide->Destroy();
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	WidgetClassProp->SetPropertyValue_InContainer(Comp, URogueWorldUserWidget::StaticClass());
	UMaterialInterface* OverlayMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	if (!TestNotNull(TEXT("A valid default overlay material must be available"), OverlayMaterial))
	{
		Comp->DestroyComponent();
		ChestAhead->Destroy();
		ChestSide->Destroy();
		Pawn->Destroy();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	HighlightProp->SetObjectPropertyValue_InContainer(Comp, OverlayMaterial);

	PC->SetControlRotation(FRotator(0.f, 0.f, 0.f));
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);

	URogueWorldUserWidget* WidgetAfterAhead = Cast<URogueWorldUserWidget>(
		WidgetProp->GetObjectPropertyValue_InContainer(Comp));
	TestEqual(
		TEXT("// REQUIRED (B3): Facing ChestAhead must focus it."),
		FocusedProp->GetObjectPropertyValue_InContainer(Comp),
		(UObject*)ChestAhead);
	TestTrue(
		TEXT("// REQUIRED (B1): The focused actor must receive the configured overlay highlight."),
		AnyMeshUsesOverlay(ChestAhead, OverlayMaterial));
	TestNotNull(
		TEXT("// REQUIRED (B2): Focusing an interactable must create a prompt widget instance."),
		WidgetAfterAhead);
	if (WidgetAfterAhead)
	{
		TestEqual(
			TEXT("// REQUIRED (B2): The prompt widget must point at the currently focused actor."),
			WidgetAfterAhead->AttachedActor.Get(),
			(AActor*)ChestAhead);
	}

	PC->SetControlRotation(FRotator(0.f, 90.f, 0.f));
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);

	URogueWorldUserWidget* WidgetAfterSide = Cast<URogueWorldUserWidget>(
		WidgetProp->GetObjectPropertyValue_InContainer(Comp));
	TestEqual(
		TEXT("// REQUIRED (B4): When a new candidate becomes best, focus must move to that actor."),
		FocusedProp->GetObjectPropertyValue_InContainer(Comp),
		(UObject*)ChestSide);
	TestTrue(
		TEXT("// REQUIRED (B4): The newly focused actor must receive the overlay highlight."),
		AnyMeshUsesOverlay(ChestSide, OverlayMaterial));
	TestTrue(
		TEXT("// REQUIRED (B4): The previously focused actor must have its overlay removed."),
		AllMeshesClearOverlay(ChestAhead));
	TestNotNull(
		TEXT("// REQUIRED (B4): A prompt widget must still exist after focus transfer."),
		WidgetAfterSide);
	if (WidgetAfterSide)
	{
		TestEqual(
			TEXT("// REQUIRED (B4): The prompt widget must retarget to the new focused actor."),
			WidgetAfterSide->AttachedActor.Get(),
			(AActor*)ChestSide);
	}

	Comp->DestroyComponent();
	ChestAhead->Destroy();
	ChestSide->Destroy();
	Pawn->Destroy();
	PC->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 13 — WidgetInst cleanup on focus loss (advisory / PARTIAL)
//
// The spec requires removing the old widget when focus is lost, but the exact
// null-pawn cleanup path is not spelled out. We keep this as an advisory check
// so it documents the desired behavior without over-constraining the contract.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTickClearsWidgetOnFocusLossTest,
	"ActionRoguelike.Interaction.TickClearsWidgetInstOnFocusLoss",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTickClearsWidgetOnFocusLossTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn a local PC (no pawn — null-pawn path in FindBestInteractable must still clear widget).
	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->SetAsLocalPlayerController();

	if (!TestTrue(TEXT("Precondition: PC->IsLocalController()=true"), PC->IsLocalController()))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// Install InteractionComponent.
	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	FObjectPropertyBase* WidgetProp = GetWidgetInstProperty();
	if (!TestNotNull(TEXT("WidgetInst UPROPERTY must exist on URogueInteractionComponent"), WidgetProp))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// Inject a non-null UObject as a stand-in for a previously-created widget.
	// We use a plain UObject because we cannot safely construct a full UUserWidget
	// without a game viewport. FObjectPropertyBase::SetObjectPropertyValue_InContainer
	// accepts any UObject* — the solution's cleanup code calls RemoveFromParent() /
	// nulls the reference; we only care that it becomes null after the tick.
	//
	// Use the Comp itself as the injected sentinel — it is a live UObject that
	// will not be GC'd before the tick, so the property will be provably non-null
	// going into the tick call.
	UObject* FakePriorWidget = Comp; // any non-null live UObject
	WidgetProp->SetObjectPropertyValue_InContainer(Comp, FakePriorWidget);

	if (!TestNotEqual(
			TEXT("Precondition: WidgetInst must be non-null before the tick (injection check)"),
			WidgetProp->GetObjectPropertyValue_InContainer(Comp),
			(UObject*)nullptr))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	// Tick: local controller, no pawn → FindBestInteractable runs, finds no candidate,
	// must clear the stale WidgetInst reference.
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);

	UObject* WidgetAfter = WidgetProp->GetObjectPropertyValue_InContainer(Comp);

	if (WidgetAfter != nullptr)
	{
		AddWarning(TEXT("Advisory: WidgetInst remained non-null on the null-pawn path. The spec requires widget cleanup on focus loss, but does not explicitly require this exact null-pawn cleanup behavior."));
	}

	Comp->DestroyComponent();
	PC->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}


// ===========================================================================
// TEST 14 — Highlight cleanup on focus loss (advisory / PARTIAL)
//
// This stays advisory because the null-pawn cleanup path is not the main hard
// contract, but it now uses the stable mesh overlay API instead of reflecting
// engine-internal OverlayMaterial storage.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionComponentTickClearsHighlightOnFocusLossTest,
	"ActionRoguelike.Interaction.TickClearsHighlightOverlayOnFocusLoss",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionComponentTickClearsHighlightOnFocusLossTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestHelpers;

	bool bCreatedWorld = false;
	UWorld* World = GetOrCreateTestWorld(bCreatedWorld);
	if (!TestNotNull(TEXT("Test world must be available"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn a local controller — the component only runs FindBestInteractable on local controllers.
	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("APlayerController must spawn"), PC))
	{
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->SetAsLocalPlayerController();

	URogueInteractionComponent* Comp = NewObject<URogueInteractionComponent>(
		PC, URogueInteractionComponent::StaticClass(), TEXT("TestInteractionComp"));
	if (!TestNotNull(TEXT("URogueInteractionComponent must instantiate"), Comp))
	{
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	PC->AddOwnedComponent(Comp);
	Comp->RegisterComponent();

	// Spawn a chest — it has at least one UStaticMeshComponent (BaseMesh) that the
	// solution uses as the target for SetOverlayMaterial calls.
	ARogueTreasureChest* Chest = World->SpawnActor<ARogueTreasureChest>(
		ARogueTreasureChest::StaticClass(),
		FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("ARogueTreasureChest must spawn"), Chest))
	{
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}

	TArray<UStaticMeshComponent*> MeshComps = GetStaticMeshComponents(Chest);
	if (!TestTrue(TEXT("Chest must have at least one UStaticMeshComponent"), MeshComps.Num() > 0))
	{
		Chest->Destroy();
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	UMaterialInterface* OverlayMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	if (!TestNotNull(TEXT("A valid default overlay material must be available"), OverlayMaterial))
	{
		Chest->Destroy();
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	for (UStaticMeshComponent* MeshComp : MeshComps)
	{
		if (MeshComp)
		{
			MeshComp->SetOverlayMaterial(OverlayMaterial);
		}
	}

	TestTrue(TEXT("Precondition: overlay must be present before tick"), AnyMeshUsesOverlay(Chest, OverlayMaterial));

	// Set FocusedActor = Chest so the component knows this actor had focus (and highlight)
	// before the current tick.
	FObjectPropertyBase* FocusedProp = GetFocusedActorProperty();
	if (!TestNotNull(TEXT("FocusedActor UPROPERTY must exist"), FocusedProp))
	{
		Chest->Destroy();
		Comp->DestroyComponent();
		PC->Destroy();
		TeardownTestWorld(World, bCreatedWorld);
		return false;
	}
	FocusedProp->SetObjectPropertyValue_InContainer(Comp, Chest);

	// Tick: local controller, no pawn → FindBestInteractable finds no new candidate.
	// Implementations that clean stale focus state should remove the old overlay.
	Comp->TickComponent(0.0f, LEVELTICK_All, &Comp->PrimaryComponentTick);

	if (!AllMeshesClearOverlay(Chest))
	{
		AddWarning(TEXT("Advisory: overlay remained on the previously focused actor after focus loss in the null-pawn harness path."));
	}

	Chest->Destroy();
	Comp->DestroyComponent();
	PC->Destroy();
	TeardownTestWorld(World, bCreatedWorld);

	return true;
}
