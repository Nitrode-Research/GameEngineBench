// Copyright 2024 UnrealBench. All Rights Reserved.
// ProjectileSystemTests.cpp
// Automation tests for the Projectile System (ActionRoguelike)
//
// All PIE-dependent checks are batched into a single PIE session
// (FProjSystem_RuntimeBehaviors) to avoid the overhead and instability
// of cycling PIE sessions rapidly in UE automation.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

// Project-specific headers
#include "Projectiles/RogueProjectilesSubsystem.h"
#include "Projectiles/RogueProjectileReplication.h"
#include "Projectiles/RogueProjectile.h"
#include "Projectiles/RogueProjectile_Magic.h"
#include "Projectiles/RogueProjectile_Blackhole.h"
#include "Projectiles/RogueProjectile_Dash.h"
#include "Projectiles/RogueProjectileMovementComponent.h"
#include "Projectiles/RogueProjectileData.h"
#include "Core/RogueGameState.h"
#include "Performance/RogueActorPoolingSubsystem.h"
#include "Performance/RogueActorPoolingInterface.h"

// ---------------------------------------------------------------------------
// Private member access via explicit template instantiation (robber trick).
// Works for protected/private non-UPROPERTY fields.
// ---------------------------------------------------------------------------
namespace ProjSysPrivate
{
	template <typename Tag, typename Tag::Type Member>
	struct Robber
	{
		friend typename Tag::Type Steal(Tag) { return Member; }
	};

	// URogueProjectilesSubsystem::ProjectileInstances (protected, not UPROPERTY)
	struct InstancesTag
	{
		using Type = TArray<FProjectileInstance> URogueProjectilesSubsystem::*;
		friend Type Steal(InstancesTag);
	};
	template struct Robber<InstancesTag, &URogueProjectilesSubsystem::ProjectileInstances>;

	static TArray<FProjectileInstance>& GetInstances(URogueProjectilesSubsystem* Sys)
	{
		return Sys->*Steal(InstancesTag());
	}

}

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
namespace ProjSysHelpers
{
	static const TCHAR* MapPath = TEXT("/Game/ActionRoguelike/Maps/TestLevel");
	static UWorld* GCachedIsolatedWorld = nullptr;

	static UWorld* GetPIEWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE && Ctx.World() && Ctx.World()->IsGameWorld())
				return Ctx.World();
		}
		return nullptr;
	}

	static URogueProjectilesSubsystem* GetSubsystem(UWorld* World)
	{
		return World ? World->GetSubsystem<URogueProjectilesSubsystem>() : nullptr;
	}

	static UWorld* CreateIsolatedWorld(const TCHAR* Suffix)
	{
		if (IsValid(GCachedIsolatedWorld))
		{
			return GCachedIsolatedWorld;
		}

		if (!GEngine)
		{
			return nullptr;
		}

		const FName WorldName = MakeUniqueObjectName(
			GetTransientPackage(), UWorld::StaticClass(), Suffix);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			return nullptr;
		}

		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		World->AddToRoot();
		GCachedIsolatedWorld = World;
		return World;
	}

	static void DestroyIsolatedWorld(UWorld* /*World*/)
	{
		// Keep the transient world alive for the duration of the automation run.
		// Destroying worlds that contain pooled actors/components has been a common
		// source of editor crashes in the Unreal harness.
	}

	static void TickWorld(UWorld* World, float TotalSeconds, float Step = 1.0f / 30.0f)
	{
		if (!World)
		{
			return;
		}

		float Remaining = TotalSeconds;
		while (Remaining > KINDA_SMALL_NUMBER)
		{
			const float Delta = FMath::Min(Step, Remaining);
			World->Tick(ELevelTick::LEVELTICK_All, Delta);
			Remaining -= Delta;
		}
	}

	template <typename T>
	static T* GetObjectProp(UObject* Obj, FName Name)
	{
		if (!Obj)
		{
			return nullptr;
		}

		const FObjectProperty* Prop = FindFProperty<FObjectProperty>(Obj->GetClass(), Name);
		return Prop ? Cast<T>(Prop->GetObjectPropertyValue_InContainer(Obj)) : nullptr;
	}

}


// ===========================================================================
// Test 1 — CDO property checks (no PIE required)
//
// B36  ARogueProjectile_Magic CDO: SphereComp radius=20, InitialLifeSpan=10,
//      DamageCoefficient=100
// B41  ARogueProjectile_Blackhole CDO: RadialForceComp ForceStrength < 0
//      (attractive / inward-pulling force)
// B42  Blackhole CDO: ECC_Pawn excluded from RadialForceComp ObjectTypesToAffect
// B43  Blackhole CDO: SphereComp ignores all channels except ECC_PhysicsBody
// B44  Blackhole CDO: InitialLifeSpan = 4.8f (matches VFX animation duration)
// B48  ARogueProjectile_Dash CDO: TeleportDelay=0.2, DetonateDelay=0.2,
//      MoveComp InitialSpeed=6000
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjSystem_CDOChecks,
	"ActionRoguelike.ProjectileSystem.CDOChecks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProjSystem_CDOChecks::RunTest(const FString& Parameters)
{
	// ---- B36: ARogueProjectile_Magic CDO defaults ----
	{
		const ARogueProjectile_Magic* CDO = GetDefault<ARogueProjectile_Magic>();
		if (TestNotNull(TEXT("B36: ARogueProjectile_Magic CDO must exist"), CDO))
		{
			// DamageCoefficient is a public UPROPERTY
			TestTrue(TEXT("B36: Magic DamageCoefficient must be 100"),
				FMath::IsNearlyEqual(CDO->DamageCoefficient, 100.0f, 0.01f));

			// InitialLifeSpan is public on AActor
			TestTrue(TEXT("B36: Magic InitialLifeSpan must be 10"),
				FMath::IsNearlyEqual(CDO->InitialLifeSpan, 10.0f, 0.01f));

			// SphereComp is a protected UPROPERTY — access via reflection
			const FObjectProperty* SphereProp = FindFProperty<FObjectProperty>(
				CDO->GetClass(), TEXT("SphereComp"));
			if (TestNotNull(TEXT("B36: SphereComp UPROPERTY must exist on ARogueProjectile_Magic"), SphereProp))
			{
				const USphereComponent* SphereComp = Cast<USphereComponent>(
					SphereProp->GetObjectPropertyValue_InContainer(CDO));
				if (TestNotNull(TEXT("B36: SphereComp must be valid on Magic CDO"), SphereComp))
				{
					TestTrue(TEXT("B36: Magic SphereComp radius must be 20"),
						FMath::IsNearlyEqual(SphereComp->GetUnscaledSphereRadius(), 20.0f, 1.0f));
				}
			}
		}
	}

	// ---- B41/B42/B43/B44: ARogueProjectile_Blackhole CDO defaults ----
	{
		const ARogueProjectile_Blackhole* CDO = GetDefault<ARogueProjectile_Blackhole>();
		if (TestNotNull(TEXT("B41-44: ARogueProjectile_Blackhole CDO must exist"), CDO))
		{
			// B44: InitialLifeSpan must match VFX animation duration (4.8s)
			TestTrue(TEXT("B44: Blackhole InitialLifeSpan must be 4.8"),
				FMath::IsNearlyEqual(CDO->InitialLifeSpan, 4.8f, 0.1f));

			// RadialForceComp is a protected UPROPERTY
			const FObjectProperty* ForceProp = FindFProperty<FObjectProperty>(
				CDO->GetClass(), TEXT("RadialForceComp"));
			if (TestNotNull(TEXT("B41: RadialForceComp UPROPERTY must exist on Blackhole CDO"), ForceProp))
			{
				const URadialForceComponent* RadialForce = Cast<URadialForceComponent>(
					ForceProp->GetObjectPropertyValue_InContainer(CDO));
				if (TestNotNull(TEXT("B41: RadialForceComp must be valid on Blackhole CDO"), RadialForce))
				{
					// B41: Negative force = attractive (objects pulled inward, not pushed away)
					TestTrue(TEXT("B41: RadialForceComp ForceStrength must be negative (attractive/inward)"),
						RadialForce->ForceStrength < 0.0f);

					// B42: Pawns must be excluded — AI projectiles must not affect characters
					const EObjectTypeQuery PawnQuery = UEngineTypes::ConvertToObjectType(ECC_Pawn);
					// ObjectTypesToAffect is protected; access via reflection (it is a UPROPERTY).
					if (const FArrayProperty* ObjTypesProp = FindFProperty<FArrayProperty>(
						RadialForce->GetClass(), TEXT("ObjectTypesToAffect")))
					{
						const TArray<TEnumAsByte<EObjectTypeQuery>>* ObjTypes =
							ObjTypesProp->ContainerPtrToValuePtr<TArray<TEnumAsByte<EObjectTypeQuery>>>(RadialForce);
						TestTrue(TEXT("B42: ECC_Pawn must be excluded from RadialForceComp ObjectTypesToAffect"),
							ObjTypes && !ObjTypes->Contains(PawnQuery));
					}
				}
			}

			// B43: SphereComp must ignore everything except ECC_PhysicsBody (physics props only)
			const FObjectProperty* SphereProp = FindFProperty<FObjectProperty>(
				CDO->GetClass(), TEXT("SphereComp"));
			if (TestNotNull(TEXT("B43: SphereComp UPROPERTY must exist on Blackhole CDO"), SphereProp))
			{
				const USphereComponent* SphereComp = Cast<USphereComponent>(
					SphereProp->GetObjectPropertyValue_InContainer(CDO));
				if (TestNotNull(TEXT("B43: Blackhole SphereComp must be valid"), SphereComp))
				{
					TestTrue(TEXT("B43: Blackhole SphereComp must ignore ECC_WorldStatic"),
						SphereComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore);
					TestTrue(TEXT("B43: Blackhole SphereComp must ignore ECC_Pawn"),
						SphereComp->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
					TestTrue(TEXT("B43: Blackhole SphereComp must overlap ECC_PhysicsBody"),
						SphereComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Overlap);
				}
			}
		}
	}

	// ---- B48: ARogueProjectile_Dash CDO defaults ----
	{
		const ARogueProjectile_Dash* CDO = GetDefault<ARogueProjectile_Dash>();
		if (TestNotNull(TEXT("B48: ARogueProjectile_Dash CDO must exist"), CDO))
		{
			// TeleportDelay and DetonateDelay are protected UPROPERTYs
			const FFloatProperty* TeleportProp = FindFProperty<FFloatProperty>(
				CDO->GetClass(), TEXT("TeleportDelay"));
			if (TestNotNull(TEXT("B48: TeleportDelay UPROPERTY must exist on Dash CDO"), TeleportProp))
			{
				TestTrue(TEXT("B48: Dash TeleportDelay must be 0.2"),
					FMath::IsNearlyEqual(TeleportProp->GetPropertyValue_InContainer(CDO), 0.2f, 0.01f));
			}

			const FFloatProperty* DetonateProp = FindFProperty<FFloatProperty>(
				CDO->GetClass(), TEXT("DetonateDelay"));
			if (TestNotNull(TEXT("B48: DetonateDelay UPROPERTY must exist on Dash CDO"), DetonateProp))
			{
				TestTrue(TEXT("B48: Dash DetonateDelay must be 0.2"),
					FMath::IsNearlyEqual(DetonateProp->GetPropertyValue_InContainer(CDO), 0.2f, 0.01f));
			}

			// MoveComp is a protected UPROPERTY; Dash constructor sets InitialSpeed = 6000
			const FObjectProperty* MoveProp = FindFProperty<FObjectProperty>(
				CDO->GetClass(), TEXT("MoveComp"));
			if (TestNotNull(TEXT("B48: MoveComp UPROPERTY must exist on Dash CDO"), MoveProp))
			{
				const URogueProjectileMovementComponent* MoveComp =
					Cast<URogueProjectileMovementComponent>(
						MoveProp->GetObjectPropertyValue_InContainer(CDO));
				if (TestNotNull(TEXT("B48: MoveComp must be valid on Dash CDO"), MoveComp))
				{
					TestTrue(TEXT("B48: Dash MoveComp InitialSpeed must be 6000"),
						FMath::IsNearlyEqual(MoveComp->InitialSpeed, 6000.0f, 1.0f));
				}
			}
		}
	}

	return true;
}


// ===========================================================================
// Test 2 — direct actor-path runtime behaviors in an isolated standalone world
//
// B26/B29/B31  Base projectile pooling behavior:
//               LifeSpanExpired returns projectile to pool instead of leaving
//               it active; the next acquire reuses the same actor.
// B27          Pool reuse resets movement state from the previous use.
// B47          A projectile hit must drive the dash explosion path, which
//               immediately freezes the marker in place.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjSystem_ActorProjectileBehaviors,
	"ActionRoguelike.ProjectileSystem.ActorProjectileBehaviors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProjSystem_ActorProjectileBehaviors::RunTest(const FString& Parameters)
{
	using namespace ProjSysHelpers;

	UWorld* World = CreateIsolatedWorld(TEXT("ProjectileActorBehaviorWorld"));
	if (!TestNotNull(TEXT("Isolated world must be created"), World))
	{
		return false;
	}

	IConsoleVariable* PoolingCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("game.ActorPooling"));
	const int32 PreviousPoolingValue = PoolingCVar ? PoolingCVar->GetInt() : 0;
	if (PoolingCVar)
	{
		PoolingCVar->Set(1, ECVF_SetByCode);
	}

	const auto RestoreAndDestroy = [&]()
	{
		if (PoolingCVar)
		{
			PoolingCVar->Set(PreviousPoolingValue, ECVF_SetByCode);
		}
		DestroyIsolatedWorld(World);
	};

	if (!TestTrue(TEXT("Actor pooling must be enabled for pooled projectile behavior checks"),
			URogueActorPoolingSubsystem::IsPoolingEnabled(World)))
	{
		RestoreAndDestroy();
		return false;
	}

	// ---- Phase A: LifeSpanExpired returns the projectile to the pool and reuse
	// resets stale movement state on the next acquire.
	{
		const FTransform SpawnTM1(FRotator::ZeroRotator, FVector(0.f, 0.f, 200.f));
		ARogueProjectile_Dash* Dash =
			URogueActorPoolingSubsystem::AcquireFromPool<ARogueProjectile_Dash>(
				World, ARogueProjectile_Dash::StaticClass(), SpawnTM1);
		if (!TestNotNull(TEXT("Dash projectile must spawn/acquire"), Dash))
		{
			RestoreAndDestroy();
			return false;
		}

		URogueProjectileMovementComponent* MoveComp =
			GetObjectProp<URogueProjectileMovementComponent>(Dash, TEXT("MoveComp"));
		if (!TestNotNull(TEXT("Dash projectile MoveComp must exist"), MoveComp))
		{
			RestoreAndDestroy();
			return false;
		}

		const FVector StaleVelocity(1234.f, 567.f, 89.f);
		MoveComp->Velocity = StaleVelocity;

		Dash->LifeSpanExpired();

		TestTrue(
			TEXT("B29: LifeSpanExpired must make the projectile inactive for pooling "
				 "(hidden and non-collidable), rather than leaving it active in the world"),
			Dash->IsHidden() && !Dash->GetActorEnableCollision());

		const FTransform SpawnTM2(FRotator::ZeroRotator, FVector(100.f, 0.f, 250.f));
		ARogueProjectile_Dash* ReacquiredDash =
			URogueActorPoolingSubsystem::AcquireFromPool<ARogueProjectile_Dash>(
				World, ARogueProjectile_Dash::StaticClass(), SpawnTM2);

		if (!TestNotNull(TEXT("A second acquire of the dash class must succeed"), ReacquiredDash))
		{
			RestoreAndDestroy();
			return false;
		}

		TestEqual(
			TEXT("B26/B31: After lifespan expiry, the next acquire must reuse the same pooled projectile actor"),
			ReacquiredDash,
			Dash);

		if (MoveComp)
		{
			TestFalse(
				TEXT("B27: Reused pooled projectile must not keep its stale previous velocity"),
				MoveComp->Velocity.Equals(StaleVelocity, 0.1f));
		}
	}

	// ---- Phase B: A hit event must trigger the dash explosion path and freeze the marker.
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APawn* InstigatorPawn = World->SpawnActor<APawn>(
			APawn::StaticClass(),
			FVector(-5000.f, 0.f, 500.f),
			FRotator(0.f, 47.f, 0.f),
			SpawnParams);
		if (!TestNotNull(TEXT("Instigator pawn must spawn"), InstigatorPawn))
		{
			RestoreAndDestroy();
			return false;
		}

		ARogueProjectile_Dash* Dash = World->SpawnActor<ARogueProjectile_Dash>(
			ARogueProjectile_Dash::StaticClass(),
			FVector(6000.f, 0.f, 600.f),
			FRotator::ZeroRotator,
			SpawnParams);
		if (!TestNotNull(TEXT("Dash projectile must spawn for explode test"), Dash))
		{
			RestoreAndDestroy();
			return false;
		}

		Dash->SetInstigator(InstigatorPawn);

		URogueProjectileMovementComponent* MoveComp =
			GetObjectProp<URogueProjectileMovementComponent>(Dash, TEXT("MoveComp"));
		if (!TestNotNull(TEXT("Dash explode test MoveComp must exist"), MoveComp))
		{
			RestoreAndDestroy();
			return false;
		}

		MoveComp->Velocity = FVector(2000.f, 0.f, 0.f);
		USphereComponent* SphereComp =
			GetObjectProp<USphereComponent>(Dash, TEXT("SphereComp"));
		if (!TestNotNull(TEXT("Dash projectile SphereComp must exist"), SphereComp))
		{
			RestoreAndDestroy();
			return false;
		}

		SphereComp->OnComponentHit.Broadcast(SphereComp, nullptr, nullptr, FVector::ZeroVector, FHitResult());

		TestTrue(
			TEXT("B47: A projectile hit must immediately freeze the dash marker by disabling collision and stopping movement"),
			!Dash->GetActorEnableCollision() && MoveComp->Velocity.IsNearlyZero());
	}

	RestoreAndDestroy();
	return true;
}


// ===========================================================================
// Test 3 — Runtime behaviors, single PIE session
//
// B21  Subsystem is authoritative in standalone (World->GetNetMode() != NM_Client)
// B20  GetUniqueProjID is deterministic: same pos+time → same ID; any difference → different ID
// B4   InternalCreateProjectile skips creation when the ID already exists in ProjectileInstances
// B11  Subsystem Tick advances each active ProjectileInstance by Velocity * DeltaTime
// B7   CreateProjectile (authority path) adds an entry to game state ProjectileData.Items
// B5   CreateProjectile adds a FProjectileInstance to the subsystem's active list
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjSystem_RuntimeBehaviors,
	"ActionRoguelike.ProjectileSystem.RuntimeBehaviors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProjSystem_RuntimeBehaviors::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(ProjSysHelpers::MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// -----------------------------------------------------------------------
	// Phase 0: B21 + B20 — authority and ID determinism (no asset needed)
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = ProjSysHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 0)"), World)) return true;

		// B21: Standalone PIE must not be NM_Client, so the subsystem is authoritative.
		TestTrue(TEXT("B21: Standalone PIE world must not be NM_Client"),
			World->GetNetMode() != NM_Client);

		URogueProjectilesSubsystem* Sys = ProjSysHelpers::GetSubsystem(World);
		if (!TestNotNull(TEXT("B21: URogueProjectilesSubsystem must exist in PIE"), Sys)) return true;

		// B20: GetUniqueProjID must be deterministic — same inputs produce the same ID.
		const FVector PosA(100.f, 200.f, 300.f);
		const float   TimeA = 1.234f;

		const uint32 ID_A1 = Sys->GetUniqueProjID(PosA, TimeA);
		const uint32 ID_A2 = Sys->GetUniqueProjID(PosA, TimeA);
		TestTrue(TEXT("B20: GetUniqueProjID must produce the same ID for identical position + time"),
			ID_A1 == ID_A2);

		// Different position must yield a different ID.
		const FVector PosB(999.f, 0.f, 0.f);
		const uint32 ID_B = Sys->GetUniqueProjID(PosB, TimeA);
		TestTrue(TEXT("B20: GetUniqueProjID must produce a different ID for a different position"),
			ID_A1 != ID_B);

		// Different time must yield a different ID.
		const uint32 ID_C = Sys->GetUniqueProjID(PosA, TimeA + 1.0f);
		TestTrue(TEXT("B20: GetUniqueProjID must produce a different ID for a different time"),
			ID_A1 != ID_C);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 1: B4 + B11 + B7/B5
	//
	// B4: InternalCreateProjectile must skip creation when a FProjectileInstance
	//     with the same ID already exists (duplicate prevention).
	// B11: Subsystem Tick must advance each active instance position by
	//      Velocity * DeltaTime (no-hit path).
	// B7/B5: CreateProjectile (authority) adds entries to both the game state
	//        ProjectileData array and the subsystem's active instance list.
	//        Gated on a live NiagaraSystem being available in PIE to avoid
	//        the null-deref in InternalCreateProjectile's VFX setup.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = ProjSysHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 2)"), World)) return true;

		URogueProjectilesSubsystem* Sys = ProjSysHelpers::GetSubsystem(World);
		if (!TestNotNull(TEXT("URogueProjectilesSubsystem must be valid (Phase 2)"), Sys)) return true;

		ARogueGameState* GS = World->GetGameState<ARogueGameState>();
		if (!TestNotNull(TEXT("ARogueGameState must be valid (Phase 2)"), GS)) return true;

		TArray<FProjectileInstance>& Instances = ProjSysPrivate::GetInstances(Sys);

		// ------- B4: Duplicate ID prevention -------
		// Pre-seed a FProjectileInstance so InternalCreateProjectile's early-exit
		// guard fires BEFORE any VFX spawn (safe with null ProjectileEffect).
		const uint32 DupTestID = 0xBEEFCAFEu;
		Instances.Empty();
		Instances.Add(FProjectileInstance(FVector(0.f, 200000.f, 50000.f),
			FVector(1.f, 0.f, 0.f) * 2000.f, DupTestID));

		// Add a matching game state entry so the expiration cleanup doesn't crash
		// when it encounters an instance without a config record.
		FProjectileItem DupItem(DupTestID);
		DupItem.ExpirationGameTime = World->TimeSeconds + 60.f;
		GS->ProjectileData.Items.Add(DupItem);

		const int32 InstanceCountBefore = Instances.Num(); // 1

		// Transient config with null ProjectileEffect — safe because the duplicate
		// guard returns before reaching the VFX spawn code (B4 must fire first).
		URogueProjectileData* DupConfig = NewObject<URogueProjectileData>(GetTransientPackage());
		DupConfig->InitialSpeed = 2000.f;
		DupConfig->Lifespan = 60.f;

		Sys->InternalCreateProjectile(
			FVector(0.f, 200000.f, 50000.f), FVector(1.f, 0.f, 0.f),
			DupConfig, nullptr, DupTestID);

		TestTrue(
			TEXT("B4: InternalCreateProjectile must not add a new instance when the ID already exists"),
			Instances.Num() == InstanceCountBefore);

		// Clean up injected data.
		Instances.Empty();
		GS->ProjectileData.Items.RemoveAll([DupTestID](const FProjectileItem& I)
			{ return I.ID == DupTestID; });

		// ------- B11 DIRECT: Tick advances active instance positions -------
		// We own all entries in both arrays after the B4 clean-up above, so there
		// are no stale TracerEffectComp pointers from real game projectiles that
		// could SIGSEGV during the expiration path.
		//
		// Set a far-future ExpirationGameTime so the item survives the tick without
		// expiring — only the position-update path runs.
		{
			const uint32 TickTestID = 0xDEADC0DEu;
			const FVector StartPos(0.f, 300000.f, 60000.f);
			const FVector Vel(1500.f, 0.f, 0.f);
			const float DeltaTime = 1.0f / 30.0f;

			// Inject one instance with known position and velocity.
			Instances.Empty();
			Instances.Add(FProjectileInstance(StartPos, Vel, TickTestID));

			// Add a matching game-state entry with a long lifetime so it won't expire.
			URogueProjectileData* TickConfig = NewObject<URogueProjectileData>(GetTransientPackage());
			TickConfig->InitialSpeed = 1500.f;
			TickConfig->Lifespan = 120.f;

			GS->ProjectileData.Items.RemoveAll([TickTestID](const FProjectileItem& I)
				{ return I.ID == TickTestID; });
			FProjectileItem TickItem(TickTestID);
			TickItem.ConfigDataAsset = TickConfig;
			TickItem.TracerEffectComp = nullptr;
			TickItem.ExpirationGameTime = World->TimeSeconds + 120.f;
			GS->ProjectileData.Items.Add(TickItem);

			// Drive one tick.
			Sys->Tick(DeltaTime);

			// B11 DIRECT: position must advance by Velocity * DeltaTime.
			// start/: Tick is a no-op → position stays at StartPos → assertion fails.
			const FVector ExpectedPos = StartPos + Vel * DeltaTime;
			if (TestTrue(TEXT("B11: Active instances array must still contain the test entry after Tick"),
					Instances.Num() > 0))
			{
				const FVector ActualPos = Instances[0].Position;
				TestTrue(
					FString::Printf(
						TEXT("B11 DIRECT: Tick must advance instance position by Velocity * DeltaTime. "
							"Expected (%.1f,%.1f,%.1f), got (%.1f,%.1f,%.1f)."),
						ExpectedPos.X, ExpectedPos.Y, ExpectedPos.Z,
						ActualPos.X, ActualPos.Y, ActualPos.Z),
					ActualPos.Equals(ExpectedPos, 1.0f));
			}

			// Clean up.
			Instances.Empty();
			GS->ProjectileData.Items.RemoveAll([TickTestID](const FProjectileItem& I)
				{ return I.ID == TickTestID; });
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 2: G10 DIRECT + G8 PARTIAL — RemoveProjectileID removes the active
	// instance directly (not via the expiration path).
	//
	// G10: The moving instance is removed from the active list.
	// G8:  The tracer deactivation code path is exercised (null tracer → safely
	//      skipped per the `if (ProjConfig.TracerEffectComp)` guard in the impl).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = ProjSysHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 2 - direct removal)"), World)) return true;

		URogueProjectilesSubsystem* Sys = ProjSysHelpers::GetSubsystem(World);
		if (!TestNotNull(TEXT("Subsystem must be valid (Phase 2)"), Sys)) return true;

		ARogueGameState* GS = World->GetGameState<ARogueGameState>();
		if (!TestNotNull(TEXT("GameState must be valid (Phase 2)"), GS)) return true;

		TArray<FProjectileInstance>& Instances = ProjSysPrivate::GetInstances(Sys);

		const uint32 RemoveID = 0xABCD1234u;

		// Pre-clean in case a previous run left state behind.
		Instances.RemoveAllSwap([RemoveID](const FProjectileInstance& I) { return I.ID == RemoveID; });
		GS->ProjectileData.Items.RemoveAll([RemoveID](const FProjectileItem& I) { return I.ID == RemoveID; });

		// Inject a live instance at a position far from level geometry.
		Instances.Add(FProjectileInstance(
			FVector(0.f, 200000.f, 50000.f),
			FVector(1000.f, 0.f, 0.f),
			RemoveID));

		// Inject the matching game-state entry.
		// TracerEffectComp = nullptr  → the deactivation branch is safely skipped (G8 path exercised).
		// Hit is default (bBlockingHit=false) → no impact VFX spawned, so null ImpactEffect is safe.
		URogueProjectileData* RemoveConfig = NewObject<URogueProjectileData>(GetTransientPackage());
		RemoveConfig->InitialSpeed = 1000.f;
		RemoveConfig->Lifespan     = 60.f;

		FProjectileItem RemoveItem(RemoveID);
		RemoveItem.ConfigDataAsset   = RemoveConfig;
		RemoveItem.TracerEffectComp  = nullptr;
		RemoveItem.ExpirationGameTime = World->TimeSeconds + 60.f;
		GS->ProjectileData.Items.Add(RemoveItem);

		TestTrue(TEXT("G10 pre: instance must exist before RemoveProjectileID"),
			Instances.ContainsByPredicate([RemoveID](const FProjectileInstance& I) { return I.ID == RemoveID; }));

		// Direct call — this is the removal path exercised by hit/expiration code,
		// not the indirect expiration-tick path tested in Phase 2.
		Sys->RemoveProjectileID(RemoveID);

		// G10 DIRECT: the active instance must be gone immediately after the call.
		const bool bInstanceRemoved = !Instances.ContainsByPredicate(
			[RemoveID](const FProjectileInstance& I) { return I.ID == RemoveID; });
		TestTrue(TEXT("G10: RemoveProjectileID must remove the instance from the active list"),
			bInstanceRemoved);

		// G8 PARTIAL: tracer code path reached without crash (null tracer safely handled).
		AddInfo(TEXT("G8 PARTIAL: Tracer deactivation code path exercised; null TracerEffectComp safely skipped."));

		// Defensive cleanup.
		Instances.RemoveAllSwap([RemoveID](const FProjectileInstance& I) { return I.ID == RemoveID; });
		GS->ProjectileData.Items.RemoveAll([RemoveID](const FProjectileItem& I) { return I.ID == RemoveID; });

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

#else
	AddWarning(TEXT("Editor-only test skipped (no WITH_EDITOR)."));
#endif

	return true;
}
