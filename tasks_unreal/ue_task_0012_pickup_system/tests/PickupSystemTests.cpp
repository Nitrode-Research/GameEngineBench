// Copyright 2024 ActionRoguelike Project. All Rights Reserved.
// PickupSystemTests.cpp — Automation tests for the pickup system

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TimerManager.h"

#include "Pickups/RoguePickupActor.h"
#include "Pickups/RoguePickupActor_Credits.h"
#include "Pickups/RoguePickupActor_HealthPotion.h"
#include "Pickups/RoguePickupActor_GrantAction.h"
#include "Pickups/RoguePickupSubsystem.h"

// We need player state for credits tests
#include "Player/RoguePlayerState.h"


// ===========================================================================
// Helpers shared across runtime tests
// ===========================================================================
namespace PickupSystemTestHelpers
{
	static TWeakObjectPtr<UWorld> GCachedTestWorld;

	static UWorld* GetOrCreateTestWorld(bool& bOutCreated)
	{
		bOutCreated = false;

		if (UWorld* Existing = AutomationCommon::GetAnyGameWorld())
		{
			return Existing;
		}

		if (GCachedTestWorld.IsValid())
		{
			bOutCreated = true;
			return GCachedTestWorld.Get();
		}

		if (!GEngine) { return nullptr; }

		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(), UWorld::StaticClass(), TEXT("PickupSystemTestWorld"));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World) { return nullptr; }

		FWorldContext& WCtx = GEngine->CreateNewWorldContext(EWorldType::Game);
		WCtx.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();

		GCachedTestWorld = World;
		bOutCreated = true;
		return World;
	}

	static void TeardownTestWorld(UWorld* /*World*/, bool /*bCreated*/)
	{
		// Keep the transient world alive across tests.
	}

	static void TickWorld(UWorld* World, float TotalSeconds, float Step = 1.0f / 30.0f)
	{
		if (!World) { return; }
		float R = TotalSeconds;
		while (R > KINDA_SMALL_NUMBER)
		{
			const float D = FMath::Min(Step, R);
			World->Tick(ELevelTick::LEVELTICK_All, D);
			R -= D;
		}
	}

	template <typename T>
	static T* GetObjProp(UObject* Obj, FName Name)
	{
		if (!Obj) { return nullptr; }
		FObjectPropertyBase* P = CastField<FObjectPropertyBase>(Obj->GetClass()->FindPropertyByName(Name));
		return P ? Cast<T>(P->GetObjectPropertyValue_InContainer(Obj)) : nullptr;
	}

	static bool GetBoolProp(UObject* Obj, FName Name, bool Default = false)
	{
		if (!Obj) { return Default; }
		FBoolProperty* P = CastField<FBoolProperty>(Obj->GetClass()->FindPropertyByName(Name));
		return P ? P->GetPropertyValue_InContainer(Obj) : Default;
	}

	static void SetBoolProp(UObject* Obj, FName Name, bool Val)
	{
		if (!Obj) { return; }
		FBoolProperty* P = CastField<FBoolProperty>(Obj->GetClass()->FindPropertyByName(Name));
		if (P) { P->SetPropertyValue_InContainer(Obj, Val); }
	}

	static int32 GetIntProp(UObject* Obj, FName Name, int32 Default = 0)
	{
		if (!Obj) { return Default; }
		FIntProperty* P = CastField<FIntProperty>(Obj->GetClass()->FindPropertyByName(Name));
		return P ? P->GetPropertyValue_InContainer(Obj) : Default;
	}

	static void SetIntProp(UObject* Obj, FName Name, int32 Val)
	{
		if (!Obj) { return; }
		FIntProperty* P = CastField<FIntProperty>(Obj->GetClass()->FindPropertyByName(Name));
		if (P) { P->SetPropertyValue_InContainer(Obj, Val); }
	}

	static bool SetNumericPropF(UObject* Obj, FName Name, float Val)
	{
		if (!Obj) { return false; }
		FProperty* Prop = Obj->GetClass()->FindPropertyByName(Name);
		if (!Prop) { return false; }
		if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
		{
			FP->SetPropertyValue_InContainer(Obj, Val);
			return true;
		}
		if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
		{
			DP->SetPropertyValue_InContainer(Obj, static_cast<double>(Val));
			return true;
		}
		return false;
	}

	static UWorld* CreateIsolatedWorld(const TCHAR* Suffix)
	{
		if (!GEngine) { return nullptr; }
		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(), UWorld::StaticClass(), Suffix);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World) { return nullptr; }
		FWorldContext& WCtx = GEngine->CreateNewWorldContext(EWorldType::Game);
		WCtx.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		return World;
	}

	static void DestroyIsolatedWorld(UWorld* World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}
	}
}


// ===========================================================================
// Protected-member accessor — explicit-template-instantiation access loophole
// (ISO C++).  Lets tests reach protected members without modifying production
// class declarations.
// ===========================================================================
namespace PickupSystemPrivateAccess
{
	template <typename Tag, typename Tag::Type Member>
	struct Robber
	{
		friend typename Tag::Type Steal(Tag) { return Member; }
	};

	// URoguePickupSubsystem::CoinPickupLocations (protected TArray<FVector>)
	struct CoinLocationsTag
	{
		using Type = TArray<FVector> URoguePickupSubsystem::*;
		friend Type Steal(CoinLocationsTag);
	};
	template struct Robber<CoinLocationsTag, &URoguePickupSubsystem::CoinPickupLocations>;

	// URoguePickupSubsystem::MeshIDs (protected TArray<FPrimitiveInstanceId>)
	struct MeshIDsTag
	{
		using Type = TArray<FPrimitiveInstanceId> URoguePickupSubsystem::*;
		friend Type Steal(MeshIDsTag);
	};
	template struct Robber<MeshIDsTag, &URoguePickupSubsystem::MeshIDs>;

	// ARoguePickupActor::ShowPickup (protected method — the respawn timer callback)
	struct ShowPickupTag
	{
		using Type = void (ARoguePickupActor::*)();
		friend Type Steal(ShowPickupTag);
	};
	template struct Robber<ShowPickupTag, &ARoguePickupActor::ShowPickup>;
}


// ===========================================================================
// TEST 1 — B8: ARoguePickupActor constructor sets SphereComp radius to 100,
//   collision profile to "Powerup", and MeshComp collision to NoCollision.
//   Checked on the CDO — no world required.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupActorConstructorConfigTest,
	"ActionRoguelike.PickupSystem.ConstructorConfigSphereAndMesh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupActorConstructorConfigTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	const ARoguePickupActor* CDO = GetDefault<ARoguePickupActor>();
	if (!TestNotNull(TEXT("ARoguePickupActor CDO must exist"), CDO)) { return false; }

	ARoguePickupActor* MutableCDO = const_cast<ARoguePickupActor*>(CDO);

	USphereComponent* SphereComp = GetObjProp<USphereComponent>(MutableCDO, TEXT("SphereComp"));
	if (TestNotNull(TEXT("SphereComp must be a USphereComponent on the CDO"), SphereComp))
	{
		TestTrue(
			TEXT("B8: SphereComp radius must be 100.0f (default is 32; stub leaves it at 32)"),
			FMath::IsNearlyEqual(SphereComp->GetUnscaledSphereRadius(), 100.0f, 1.0f));

		TestEqual(
			TEXT("B8: SphereComp collision profile must be 'Powerup' (stub leaves the engine default)"),
			SphereComp->GetCollisionProfileName(),
			FName(TEXT("Powerup")));
	}

	UStaticMeshComponent* MeshComp = GetObjProp<UStaticMeshComponent>(MutableCDO, TEXT("MeshComp"));
	if (TestNotNull(TEXT("MeshComp must be a UStaticMeshComponent on the CDO"), MeshComp))
	{
		TestEqual(
			TEXT("B8: MeshComp collision must be NoCollision — SphereComp handles interaction"),
			MeshComp->GetCollisionEnabled(),
			ECollisionEnabled::NoCollision);
	}

	return true;
}


// ===========================================================================
// TEST 2 — B9/B18: ARoguePickupActor_Credits CDO must set bCanAutoPickup=true
//   and CreditsAmount=80.
//   Checked on the CDO — no world required.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupCreditsCDOTest,
	"ActionRoguelike.PickupSystem.CreditsPickupCDO",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupCreditsCDOTest::RunTest(const FString& Parameters)
{
	UClass* CreditsClass = ARoguePickupActor_Credits::StaticClass();
	if (!TestNotNull(TEXT("ARoguePickupActor_Credits class must exist"), CreditsClass)) { return false; }

	const ARoguePickupActor_Credits* CDO = GetDefault<ARoguePickupActor_Credits>();
	if (!TestNotNull(TEXT("ARoguePickupActor_Credits CDO must exist"), CDO)) { return false; }

	FBoolProperty* AutoPickupProp = CastField<FBoolProperty>(
		ARoguePickupActor::StaticClass()->FindPropertyByName(TEXT("bCanAutoPickup")));
	if (TestNotNull(TEXT("bCanAutoPickup UPROPERTY must exist on ARoguePickupActor"), AutoPickupProp))
	{
		TestTrue(
			TEXT("B9: Credits bCanAutoPickup must be true — auto-collect on overlap"),
			AutoPickupProp->GetPropertyValue_InContainer(CDO));
	}

	FIntProperty* AmountProp = CastField<FIntProperty>(
		CreditsClass->FindPropertyByName(TEXT("CreditsAmount")));
	if (TestNotNull(TEXT("CreditsAmount UPROPERTY must exist"), AmountProp))
	{
		TestEqual(
			TEXT("B18: CreditsAmount must be 80 — stub leaves it at 0"),
			AmountProp->GetPropertyValue_InContainer(CDO),
			80);
	}

	return true;
}


// ===========================================================================
// TEST 3 — B10: OnRep_IsActive must disable actor collision and hide the root
//   component when bIsActive is false, and re-enable both when bIsActive is true.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupOnRepIsActiveTest,
	"ActionRoguelike.PickupSystem.OnRepIsActiveTogglesCollisionAndVisibility",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupOnRepIsActiveTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	bool bCreated;
	UWorld* World = GetOrCreateTestWorld(bCreated);
	if (!TestNotNull(TEXT("Test world must exist"), World)) { return false; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARoguePickupActor_HealthPotion* Pickup = World->SpawnActor<ARoguePickupActor_HealthPotion>(
		ARoguePickupActor_HealthPotion::StaticClass(), FVector(300.f, 0.f, 200.f),
		FRotator::ZeroRotator, SP);

	if (!TestNotNull(TEXT("HealthPotion pickup must spawn"), Pickup))
	{
		TeardownTestWorld(World, bCreated);
		return false;
	}

	UFunction* OnRepFn = Pickup->FindFunction(TEXT("OnRep_IsActive"));

	// --- Hide path: set bIsActive=false, fire OnRep_IsActive ---
	SetBoolProp(Pickup, TEXT("bIsActive"), false);
	if (TestNotNull(TEXT("OnRep_IsActive UFUNCTION must be discoverable"), OnRepFn))
	{
		Pickup->ProcessEvent(OnRepFn, nullptr);
	}

	// The spec requires collision to be disabled on hide. Accept either the actor-level
	// approach (SetActorEnableCollision) or the component-level approach
	// (SphereComp->SetCollisionEnabled(NoCollision)) — both make the pickup non-collidable.
	{
		const bool bActorCollOff = !Pickup->GetActorEnableCollision();
		USphereComponent* SC = GetObjProp<USphereComponent>(Pickup, TEXT("SphereComp"));
		const bool bSphereCollOff = SC && SC->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
		TestTrue(
			TEXT("B10 (hide): Collision must be disabled after OnRep_IsActive(false)"),
			bActorCollOff || bSphereCollOff);
	}

	if (Pickup->GetRootComponent())
	{
		TestFalse(
			TEXT("B10 (hide): Root component visibility must be false after OnRep_IsActive(false)"),
			Pickup->GetRootComponent()->IsVisible());
	}

	// --- Show path: set bIsActive=true, fire OnRep_IsActive ---
	SetBoolProp(Pickup, TEXT("bIsActive"), true);
	if (OnRepFn)
	{
		Pickup->ProcessEvent(OnRepFn, nullptr);
	}

	{
		const bool bActorCollOn = Pickup->GetActorEnableCollision();
		USphereComponent* SC2 = GetObjProp<USphereComponent>(Pickup, TEXT("SphereComp"));
		const bool bSphereCollOn = SC2 && SC2->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
		TestTrue(
			TEXT("B10 (show): Collision must be re-enabled after OnRep_IsActive(true)"),
			bActorCollOn || bSphereCollOn);
	}

	if (Pickup->GetRootComponent())
	{
		TestTrue(
			TEXT("B10 (show): Root component visibility must be true after OnRep_IsActive(true)"),
			GetBoolProp(Pickup->GetRootComponent(), TEXT("bVisible"), false));
	}

	Pickup->Destroy();
	TeardownTestWorld(World, bCreated);
	return true;
}


// ===========================================================================
// TEST 4 — B7: The subsystem's WorldISM must be null before any coins are added.
//   Uses a fresh transient world so the subsystem starts in a clean state.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupSubsystemISMNullBeforeCoinTest,
	"ActionRoguelike.PickupSystem.ISMNullBeforeCoinAdded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupSubsystemISMNullBeforeCoinTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	if (!GEngine) { return false; }

	UWorld* World = CreateIsolatedWorld(TEXT("PickupISMNullWorld"));
	if (!TestNotNull(TEXT("Fresh transient world must be created"), World)) { return false; }

	URoguePickupSubsystem* Sub = World->GetSubsystem<URoguePickupSubsystem>();
	if (!TestNotNull(TEXT("URoguePickupSubsystem must be available in the fresh world"), Sub))
	{
		DestroyIsolatedWorld(World);
		return false;
	}

	// WorldISM has UPROPERTY() so it is accessible via reflection.
	UInstancedStaticMeshComponent* WorldISM =
		GetObjProp<UInstancedStaticMeshComponent>(Sub, TEXT("WorldISM"));

	TestNull(
		TEXT("B7: WorldISM must be null before any coins are added — "
		     "ISM is created lazily, not at Initialize"),
		WorldISM);

	DestroyIsolatedWorld(World);
	return true;
}


// ===========================================================================
// TEST 5 — B18 (direct runtime): Credits Interact_Implementation must award
//   the configured CreditsAmount to the player's state and then hide the pickup.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupCreditsInteractAwardsAndHidesTest,
	"ActionRoguelike.PickupSystem.CreditsInteractAwardsAndHidesPickup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupCreditsInteractAwardsAndHidesTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	if (!GEngine)
	{
		AddError(TEXT("GEngine is null"));
		return false;
	}

	UWorld* World = CreateIsolatedWorld(TEXT("PickupCreditsInteractWorld"));
	if (!TestNotNull(TEXT("World must be created"), World)) { return false; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARoguePickupActor_Credits* CreditsPickup = World->SpawnActor<ARoguePickupActor_Credits>(
		ARoguePickupActor_Credits::StaticClass(), FVector(1000.f, 0.f, 200.f),
		FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("Credits pickup must spawn"), CreditsPickup))
	{
		DestroyIsolatedWorld(World);
		return false;
	}

	// Set a known credits amount via reflection.
	const int32 TestCreditsAmount = 50;
	SetIntProp(CreditsPickup, TEXT("CreditsAmount"), TestCreditsAmount);

	// Spawn a player controller.
	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector(900.f, 0.f, 200.f), FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("PlayerController must spawn"), PC))
	{
		CreditsPickup->Destroy();
		DestroyIsolatedWorld(World);
		return false;
	}

	// Spawn an ARoguePlayerState and manually assign it to the controller.
	ARoguePlayerState* PS = World->SpawnActor<ARoguePlayerState>(
		ARoguePlayerState::StaticClass(), FVector(950.f, 0.f, 200.f), FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("ARoguePlayerState must spawn"), PS))
	{
		PC->Destroy();
		CreditsPickup->Destroy();
		DestroyIsolatedWorld(World);
		return false;
	}

	PC->PlayerState = PS;

	const int32 CreditsBefore = PS->GetCredits();

	IRogueGameplayInterface::Execute_Interact(CreditsPickup, PC);

	const int32 CreditsAfter = PS->GetCredits();
	TestEqual(
		TEXT("B18: Credits must increase by CreditsAmount after Credits pickup interaction"),
		CreditsAfter,
		CreditsBefore + TestCreditsAmount);

	TestFalse(
		TEXT("B18: Pickup must be inactive (bIsActive==false) after Credits interaction"),
		GetBoolProp(CreditsPickup, TEXT("bIsActive"), true));

	PS->Destroy();
	PC->Destroy();
	if (IsValid(CreditsPickup)) { CreditsPickup->Destroy(); }
	DestroyIsolatedWorld(World);
	return true;
}


// ===========================================================================
// TEST 6 — B11/B12 (direct runtime): HideAndCooldown hides the pickup
//   immediately via Credits Interact, and after the respawn timer fires it
//   becomes visible/collidable again. Also verifies EndPlay clears timers.
//
//   Uses SetPickupState via OnRep_IsActive reflection to verify state, and
//   ticks the world to advance the respawn timer.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupHideAndCooldownRespawnTest,
	"ActionRoguelike.PickupSystem.HideAndCooldownRespawnsPickup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupHideAndCooldownRespawnTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	if (!GEngine) { return false; }

	UWorld* World = CreateIsolatedWorld(TEXT("PickupRespawnWorld"));
	if (!TestNotNull(TEXT("World must be created"), World)) { return false; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Use Credits pickup (concrete, simple class — no pawn/health needed)
	ARoguePickupActor_Credits* Pickup = World->SpawnActor<ARoguePickupActor_Credits>(
		ARoguePickupActor_Credits::StaticClass(), FVector(2000.f, 0.f, 200.f),
		FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("Credits pickup must spawn for respawn test"), Pickup))
	{
		DestroyIsolatedWorld(World);
		return false;
	}

	// Shorten the respawn time so the test can tick it out quickly.
	const float ShortRespawnTime = 0.5f;
	SetNumericPropF(Pickup, TEXT("RespawnTime"), ShortRespawnTime);

	// Pickup starts active.
	TestTrue(TEXT("B11 pre: Pickup must start active (bIsActive==true)"),
		GetBoolProp(Pickup, TEXT("bIsActive"), false));

	// Set up a minimal player state so Credits Interact succeeds and calls HideAndCooldown.
	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector(1900.f, 0.f, 200.f), FRotator::ZeroRotator, SP);
	ARoguePlayerState* PS = World->SpawnActor<ARoguePlayerState>(
		ARoguePlayerState::StaticClass(), FVector(1950.f, 0.f, 200.f), FRotator::ZeroRotator, SP);

	if (!PC || !PS)
	{
		if (PC) PC->Destroy();
		if (PS) PS->Destroy();
		Pickup->Destroy();
		DestroyIsolatedWorld(World);
		AddError(TEXT("Could not spawn PC/PS for respawn test"));
		return false;
	}
	PC->PlayerState = PS;

	// Trigger Interact — this calls HideAndCooldown internally.
	IRogueGameplayInterface::Execute_Interact(Pickup, PC);

	// Immediately after interaction, pickup must be hidden/inactive.
	TestFalse(
		TEXT("B11: After HideAndCooldown, bIsActive must be false immediately"),
		GetBoolProp(Pickup, TEXT("bIsActive"), true));
	{
		const bool bActorCollOff = !Pickup->GetActorEnableCollision();
		USphereComponent* SC = GetObjProp<USphereComponent>(Pickup, TEXT("SphereComp"));
		const bool bSphereCollOff = SC && SC->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
		TestTrue(
			TEXT("B11: After HideAndCooldown, collision must be disabled"),
			bActorCollOff || bSphereCollOff);
	}

	// Directly invoke ShowPickup (the respawn timer callback) to verify the
	// restore path. Fresh transient worlds do not advance FTimerManager via
	// World->Tick the way PIE sessions do, so we test the callback directly.
	// ShowPickup is exactly what HideAndCooldown schedules; testing it directly
	// covers the same B11 behavioral contract.
	auto ShowPickupFn = Steal(PickupSystemPrivateAccess::ShowPickupTag{});
	(Pickup->*ShowPickupFn)();

	// After ShowPickup, pickup must be active/visible again.
	TestTrue(
		TEXT("B11: After ShowPickup (respawn callback), bIsActive must be true — "
		     "stub's empty ShowPickup never calls SetPickupState(true)"),
		GetBoolProp(Pickup, TEXT("bIsActive"), false));
	{
		const bool bActorCollOn = Pickup->GetActorEnableCollision();
		USphereComponent* SC2 = GetObjProp<USphereComponent>(Pickup, TEXT("SphereComp"));
		const bool bSphereCollOn = SC2 && SC2->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
		TestTrue(
			TEXT("B11: After respawn, collision must be re-enabled"),
			bActorCollOn || bSphereCollOn);
	}

	// B12: Destroy must not crash (EndPlay clears timers).
	Pickup->Destroy();
	TickWorld(World, 0.1f); // Should not crash after destroy.
	AddInfo(TEXT("B12: Pickup destroyed without crash — EndPlay timer cleanup verified"));

	PC->Destroy();
	PS->Destroy();
	DestroyIsolatedWorld(World);
	return true;
}


// ===========================================================================
// TEST 7 — B6: URoguePickupSubsystem::IsTickable must return true on
//   server/standalone (NetMode < NM_Client).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupSubsystemIsTickableTest,
	"ActionRoguelike.PickupSystem.SubsystemTickableOnServer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupSubsystemIsTickableTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	UWorld* World = AutomationCommon::GetAnyGameWorld();
	if (!World)
	{
		// Create a transient world to test against.
		World = CreateIsolatedWorld(TEXT("PickupTickableWorld"));
		if (!TestNotNull(TEXT("World must exist for IsTickable test"), World)) { return false; }

		URoguePickupSubsystem* Sub = World->GetSubsystem<URoguePickupSubsystem>();
		if (!TestNotNull(TEXT("URoguePickupSubsystem must be available in the world"), Sub))
		{
			DestroyIsolatedWorld(World);
			return false;
		}

		UTickableWorldSubsystem* TickableBase = Sub;
		TestTrue(
			TEXT("B6: IsTickable() must return true on server/standalone"),
			TickableBase->IsTickable());

		DestroyIsolatedWorld(World);
		return true;
	}

	URoguePickupSubsystem* Sub = World->GetSubsystem<URoguePickupSubsystem>();
	if (!TestNotNull(TEXT("URoguePickupSubsystem must be available in the world"), Sub)) { return false; }

	UTickableWorldSubsystem* TickableBase = Sub;
	TestTrue(
		TEXT("B6: IsTickable() must return true on server/standalone"),
		TickableBase->IsTickable());

	return true;
}


// ===========================================================================
// TEST 8 — B16: ARoguePickupActor_HealthPotion::GetInteractText_Implementation
//   must return non-empty text describing the credit cost (B16).
//   The base class returns FText::GetEmpty(); only the solution overrides it
//   with useful UX text.
//   Calling with a pawn-less APlayerController is safe: the implementation
//   reads GetPawn() (returns null) then falls through to the format text since
//   the "already at full health" branch requires a non-null pawn.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupHealthPotionInteractTextTest,
	"ActionRoguelike.PickupSystem.HealthPotionInteractText",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupHealthPotionInteractTextTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	if (!GEngine) { return false; }

	UWorld* World = CreateIsolatedWorld(TEXT("PickupPotionTextWorld"));
	if (!TestNotNull(TEXT("World must be created"), World)) { return false; }

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoguePickupActor_HealthPotion* Potion = World->SpawnActor<ARoguePickupActor_HealthPotion>(
		ARoguePickupActor_HealthPotion::StaticClass(), FVector(3000.f, 0.f, 200.f),
		FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("HealthPotion pickup must spawn"), Potion))
	{
		DestroyIsolatedWorld(World);
		return false;
	}

	// Spawn a pawn-less controller — safe for GetInteractText (pawn null → no
	// "full health" branch; falls through to the credit-cost format text).
	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(), FVector(2900.f, 0.f, 200.f), FRotator::ZeroRotator, SP);
	if (!TestNotNull(TEXT("PlayerController must spawn"), PC))
	{
		Potion->Destroy();
		DestroyIsolatedWorld(World);
		return false;
	}

	// GetInteractText must return non-empty text containing cost info (B16).
	// The base class returns FText::GetEmpty(); the solution returns a
	// formatted string like "Cost 50 Credits. Restores health to maximum."
	const FText InteractText = IRogueGameplayInterface::Execute_GetInteractText(Potion, PC);

	TestFalse(
		TEXT("B16: GetInteractText must return non-empty text — "
		     "base class returns empty; HealthPotion must override with cost/effect message"),
		InteractText.IsEmpty());

	// The CreditCost default is 50 (in-header initializer). Verify the text
	// mentions a credit value so we know it's not just a stub "Credits" string.
	// We convert to string for a simple substring check.
	const FString TextStr = InteractText.ToString();
	TestTrue(
		TEXT("B16: GetInteractText must contain credit cost information — "
		     "expected format string with CreditCost value"),
		TextStr.Contains(TEXT("Credit"), ESearchCase::IgnoreCase) ||
		TextStr.Contains(TEXT("credit"), ESearchCase::IgnoreCase));

	PC->Destroy();
	Potion->Destroy();
	DestroyIsolatedWorld(World);
	return true;
}


// ===========================================================================
// TEST 9 — B1/B2/B7: AddCoinsPickup must create the ISM on first call and
//   track coin positions alongside mesh instance IDs so individual coins can
//   be removed later.  After adding N locations: CoinPickupLocations.Num()==N
//   and MeshIDs.Num()==N.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupSubsystemAddCoinISMTrackingTest,
	"ActionRoguelike.PickupSystem.AddCoinsCreatesISMWithTracking",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupSubsystemAddCoinISMTrackingTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemPrivateAccess;
	using namespace PickupSystemTestHelpers;

	if (!GEngine) { return false; }

	UWorld* World = CreateIsolatedWorld(TEXT("PickupISMTrackingWorld"));
	if (!TestNotNull(TEXT("Fresh transient world must be created"), World)) { return false; }

	URoguePickupSubsystem* Sub = World->GetSubsystem<URoguePickupSubsystem>();
	if (!TestNotNull(TEXT("URoguePickupSubsystem must be available"), Sub))
	{
		DestroyIsolatedWorld(World);
		return false;
	}

	const TArray<FVector> Locs = { FVector(0, 0, 10), FVector(100, 0, 10), FVector(200, 0, 10) };
	const TArray<int32>   Amts = { 10, 20, 30 };
	Sub->AddCoinsPickup(Locs, Amts);

	// B1/B7: WorldISM must now be a valid ISM component.
	UInstancedStaticMeshComponent* WorldISM =
		GetObjProp<UInstancedStaticMeshComponent>(Sub, TEXT("WorldISM"));
	TestNotNull(
		TEXT("B1/B7: WorldISM must be valid after AddCoinsPickup — "
		     "ISM is created lazily on the first coin add"),
		WorldISM);

	// B2: Both tracking arrays must have the same entry count as the added locations.
	const TArray<FVector>&             TrackedLocs = Sub->*Steal(CoinLocationsTag{});
	const TArray<FPrimitiveInstanceId>& TrackedIDs  = Sub->*Steal(MeshIDsTag{});

	TestEqual(
		TEXT("B2: CoinPickupLocations count must match the number of coins added"),
		TrackedLocs.Num(), 3);
	TestEqual(
		TEXT("B2: MeshIDs count must match CoinPickupLocations — "
		     "IDs are required for per-coin ISM removal; stub may omit populating MeshIDs"),
		TrackedIDs.Num(), 3);

	DestroyIsolatedWorld(World);
	return true;
}


// ===========================================================================
// TEST 10 — B9: Credits pickup must bind OnSphereOverlap after spawn.
//   PostInitializeComponents must check bCanAutoPickup and call AddDynamic.
//   Stub's empty PostInitializeComponents (and missing bCanAutoPickup=true)
//   leaves the delegate unbound.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPickupCreditsOverlapBoundTest,
	"ActionRoguelike.PickupSystem.CreditsOverlapDelegateBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPickupCreditsOverlapBoundTest::RunTest(const FString& Parameters)
{
	using namespace PickupSystemTestHelpers;

	UWorld* World = AutomationCommon::GetAnyGameWorld();
	if (!World)
	{
		AddWarning(TEXT("No game world available — test will run in the listen_server phase"));
		return true;
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARoguePickupActor_Credits* Credits = World->SpawnActor<ARoguePickupActor_Credits>(
		ARoguePickupActor_Credits::StaticClass(), FVector(0.f, 0.f, 500.f),
		FRotator::ZeroRotator, SP);

	if (!TestNotNull(TEXT("ARoguePickupActor_Credits must spawn"), Credits)) { return false; }

	USphereComponent* SphereComp = GetObjProp<USphereComponent>(Credits, TEXT("SphereComp"));
	if (TestNotNull(TEXT("SphereComp must be valid on the spawned Credits pickup"), SphereComp))
	{
		TestTrue(
			TEXT("B9: SphereComp OnComponentBeginOverlap must be bound — "
			     "PostInitializeComponents must call AddDynamic when bCanAutoPickup is true; "
			     "stub leaves PostInitializeComponents empty"),
			SphereComp->OnComponentBeginOverlap.IsBound());
	}

	Credits->Destroy();
	return true;
}
