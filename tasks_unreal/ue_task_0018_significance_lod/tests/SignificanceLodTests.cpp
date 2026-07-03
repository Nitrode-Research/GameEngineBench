// Copyright 2024 UnrealBench. All Rights Reserved.
// SignificanceLodTests.cpp
// Automation tests for the Significance / LOD System (ActionRoguelike)
//
// Test 1 (CDOChecks) validates constructor defaults and CVar existence without PIE.
// Test 2 (RuntimeBehaviors) runs a single PIE session covering:
//   Phase 0: B11 — manager accessible
//   Phase 1: B8  — opt-out component is not registered
//   Phase 2: B12 — immediate registration (bWaitOneFrame=false)
//   Phase 3: B13 — EndPlay unregisters component
//   Phase 4: B2/B10 — deferred registration (bWaitOneFrame=true, not registered immediately)
//   Phase 5: B14/B15/B17/B18/B21/B22 — component-side significance pipeline
//
// The benchmark focuses on the significance component's observable behavior:
// registration/unregistration, significance calculation, and first-update
// notification semantics. The custom manager's tag/bucket redistribution path
// is not exercised here because it is not stable for generic transient actors.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "SignificanceManager.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

// Project-specific headers
#include "Performance/RogueSignificanceComponent.h"
#include "Performance/RogueSignificanceManager.h"


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace SigLodHelpers
{
	static const TCHAR* MapPath = TEXT("/Game/ActionRoguelike/Maps/TestLevel");

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

	static URogueSignificanceManager* GetSigManager(UWorld* World)
	{
		return Cast<URogueSignificanceManager>(USignificanceManager::Get(World));
	}

	/** Returns true when Comp appears in the world's significance manager object list. */
	static bool IsRegistered(UWorld* World, URogueSignificanceComponent* Comp)
	{
		USignificanceManager* Mgr = USignificanceManager::Get(World);
		if (!Mgr || !Comp) return false;
		return Mgr->GetManagedObject(Comp) != nullptr;
	}
}


// ===========================================================================
// Test 1 — CDO / structural checks (no PIE required)
//
// B1  Tick is disabled on the component.
// B2  bWaitOneFrame defaults to true (deferred by one frame).
// B3  The component defaults to actively managing significance.
// B4  CurrentSignificance initialized to the Invalid sentinel.
// B5  A default distance threshold is added for the Highest tier.
// B6  Hidden actors are treated as insignificant by default.
// B7  Particle systems on the owner are managed by default.
// B14 SigMan.ForceSignificance CVar exists and is disabled by default.  (PARTIAL)
// B20 GetSignificanceByDistance: if threshold table is empty, highest tier returned.
//     (PARTIAL — verified structurally: CDO always has at least one threshold entry,
//      confirming the default path never falls into the empty-table branch.)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSignificanceLOD_CDOChecks,
	"ActionRoguelike.SignificanceLOD.CDOChecks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSignificanceLOD_CDOChecks::RunTest(const FString& Parameters)
{
	const URogueSignificanceComponent* CDO = GetDefault<URogueSignificanceComponent>();
	if (!TestNotNull(TEXT("URogueSignificanceComponent CDO must exist"), CDO))
		return false;

	const UClass* Class = URogueSignificanceComponent::StaticClass();

	// B1 — tick disabled so the component does not run its own tick logic
	TestFalse(TEXT("B1: PrimaryComponentTick.bCanEverTick must be false"),
		CDO->PrimaryComponentTick.bCanEverTick);

	// B3 — significance management is active by default; clearing the flag makes it inert
	TestTrue(TEXT("B3: bManageSignificance must default to true"),
		CDO->bManageSignificance);

	// B4 — sentinel value ensures the first update always triggers a change notification
	TestEqual(TEXT("B4: CurrentSignificance must be initialized to ESignificanceValue::Invalid"),
		CDO->CurrentSignificance, ESignificanceValue::Invalid);

	// B5 — at least one default threshold entry; first entry maps the Highest tier
	//      to a moderate world-unit radius (spec: "moderate world-unit radius")
	{
		const TArray<FSignificanceDistance>& T = CDO->Thresholds;
		TestTrue(TEXT("B5: Thresholds must have at least one default entry"), T.Num() >= 1);
		if (T.Num() >= 1)
		{
			TestEqual(TEXT("B5: Default threshold significance tier must be Highest"),
				T[0].Significance, ESignificanceValue::Highest);
			// "moderate" interpreted as [1 000, 10 000] UU
			TestTrue(TEXT("B5: Default MaxDistance must be a moderate radius (1 000 – 10 000 UU)"),
				T[0].MaxDistance >= 1000.f && T[0].MaxDistance <= 10000.f);
		}
	}

	// B20 (PARTIAL) — because the CDO always has at least one threshold, the empty-table
	//                  warning/fallback path is never taken in the default configuration.
	//                  This confirms the non-empty branch works; the empty-branch is a
	//                  defensive guard that cannot safely be triggered via PIE for this task.
	TestTrue(TEXT("B20 (partial): Threshold table is non-empty in CDO, confirming the default path is not the empty-table branch"),
		CDO->Thresholds.Num() > 0);

	// B2 — deferred registration flag (protected UPROPERTY — accessed via reflection)
	{
		const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Class, TEXT("bWaitOneFrame"));
		if (TestNotNull(TEXT("B2: bWaitOneFrame UPROPERTY must exist on the component"), Prop))
			TestTrue(TEXT("B2: bWaitOneFrame must default to true (deferred registration by one frame)"),
				Prop->GetPropertyValue_InContainer(CDO));
	}

	// B6 — hidden-actor significance flag (protected UPROPERTY — reflection)
	{
		const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Class, TEXT("bInsignificantWhenOwnerIsHidden"));
		if (TestNotNull(TEXT("B6: bInsignificantWhenOwnerIsHidden UPROPERTY must exist"), Prop))
			TestTrue(TEXT("B6: bInsignificantWhenOwnerIsHidden must default to true"),
				Prop->GetPropertyValue_InContainer(CDO));
	}

	// B7 — particle-management flag (protected UPROPERTY — reflection)
	{
		const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Class, TEXT("bManageOwnerParticleSignificance"));
		if (TestNotNull(TEXT("B7: bManageOwnerParticleSignificance UPROPERTY must exist"), Prop))
			TestTrue(TEXT("B7: bManageOwnerParticleSignificance must default to true"),
				Prop->GetPropertyValue_InContainer(CDO));
	}

	// B14 (PARTIAL) — global debug-override CVar must exist and be disabled by default
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("SigMan.ForceSignificance"));
		if (TestNotNull(TEXT("B14: SigMan.ForceSignificance CVar must be registered"), CVar))
			TestTrue(TEXT("B14: SigMan.ForceSignificance must default to < 0 (disabled, no forced override)"),
				CVar->GetFloat() < 0.f);
	}

	return true;
}


// ===========================================================================
// Test 2 — PIE runtime behaviors (single standalone PIE session)
//
// Phase 0: B11 — significance manager accessible via USignificanceManager::Get.
// Phase 1: B8  — component with bManageSignificance=false is not registered.
// Phase 2: B12 — component with bManageSignificance=true (bWaitOneFrame=false)
//               IS registered immediately after BeginPlay; actor is destroyed
//               synchronously before Update() runs.
// Phase 3: B13 — after actor destruction EndPlay fires and the component must
//               no longer appear in the manager's managed-object list.
// Phase 4: B2/B10 — with bWaitOneFrame=true (default) the component must NOT
//               be registered immediately after BeginPlay (deferred by one tick).
// Phase 5: B14/B15/B17/B18/B21/B22 — component-side significance pipeline.
//               Calls the base USignificanceManager::Update() path directly
//               so the component's registered significance / post-significance
//               callbacks can be exercised in a stable way.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSignificanceLOD_RuntimeBehaviors,
	"ActionRoguelike.SignificanceLOD.RuntimeBehaviors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSignificanceLOD_RuntimeBehaviors::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(SigLodHelpers::MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	// false = standalone — consistent with other tasks in this suite
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// -----------------------------------------------------------------------
	// Phase 0: B11 — manager accessible
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 0)"), World)) return true;

		USignificanceManager* Mgr = USignificanceManager::Get(World);
		TestNotNull(TEXT("B11: USignificanceManager::Get must return a valid manager"), Mgr);
		TestNotNull(TEXT("B11: Manager must be a URogueSignificanceManager"),
			SigLodHelpers::GetSigManager(World));
		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 1: B8 — component with bManageSignificance=false is not registered.
	// Safe: BeginPlay does nothing when bManageSignificance=false so the component
	// is never added to the manager and Update() never processes it.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 1)"), World)) return true;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* InertActor = World->SpawnActor<AActor>(AActor::StaticClass(),
			FTransform(FVector(1000.f, 0.f, 0.f)), SP);
		if (!TestNotNull(TEXT("Phase 1: InertActor must spawn"), InertActor)) return true;

		// bManageSignificance is public — set it to false before RegisterComponent()
		// so BeginPlay does nothing (B8: "if not opted in, do nothing")
		URogueSignificanceComponent* InertComp = NewObject<URogueSignificanceComponent>(
			InertActor, NAME_None, RF_Transient);
		InertComp->bManageSignificance = false;
		InertComp->RegisterComponent(); // BeginPlay fires with bManageSignificance=false

		TestFalse(TEXT("B8: Component with bManageSignificance=false must not register with the significance manager"),
			SigLodHelpers::IsRegistered(World, InertComp));

		InertActor->Destroy();
		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 2: B12 — component with bManageSignificance=true registers with
	// the manager during BeginPlay when bWaitOneFrame is false.
	//
	// Safety: bWaitOneFrame is set to false via reflection so registration
	// happens synchronously inside RegisterComponent()/BeginPlay, not deferred.
	// The actor is destroyed immediately after the check; EndPlay unregisters
	// the component synchronously, so it is gone before the next Update() tick.
	// On start/ the stub BeginPlay does nothing → IsRegistered returns false →
	// TestTrue fails (correct). On solution/ BeginPlay calls RegisterWithManager
	// → IsRegistered returns true → TestTrue passes (correct).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 2)"), World)) return true;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* TestActor = World->SpawnActor<AActor>(AActor::StaticClass(),
			FTransform(FVector(2000.f, 0.f, 0.f)), SP);
		if (!TestNotNull(TEXT("Phase 2: TestActor must spawn"), TestActor)) return true;

		URogueSignificanceComponent* Comp = NewObject<URogueSignificanceComponent>(
			TestActor, NAME_None, RF_Transient);
		Comp->bManageSignificance = true;

		// Disable deferred registration so BeginPlay registers synchronously.
		// bWaitOneFrame is a protected UPROPERTY — access via reflection.
		const FBoolProperty* WaitProp = FindFProperty<FBoolProperty>(
			URogueSignificanceComponent::StaticClass(), TEXT("bWaitOneFrame"));
		if (WaitProp)
			WaitProp->SetPropertyValue_InContainer(Comp, false);

		// BeginPlay fires inside RegisterComponent(); on solution/ this calls
		// RegisterWithManager() immediately (bWaitOneFrame is now false).
		Comp->RegisterComponent();

		const bool bRegistered = SigLodHelpers::IsRegistered(World, Comp);

		// Destroy synchronously — EndPlay unregisters the component (B13 on
		// solution/) before the next Update() tick, preventing a null-deref crash.
		TestActor->Destroy();

		TestTrue(TEXT("B12: Component with bManageSignificance=true must be registered with the significance manager after BeginPlay"),
			bRegistered);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 3: B13 — EndPlay unregisters the component from the manager.
	//
	// Safety: identical to Phase 2 — the actor is destroyed synchronously
	// inside the latent command, before the next Update() tick, so the
	// null-deref in URogueSignificanceManager::Update() is never reached.
	// Comp is pending-kill after Destroy() but not yet GC'd; USignificanceManager
	// uses it only as a map key (no dereference), so the lookup is safe.
	//
	// solution/: EndPlay calls UnregisterObject → IsRegistered returns false
	//             after Destroy() → TestFalse passes.
	// start/   : BeginPlay does nothing → bWasRegistered is false → the
	//             TestFalse is skipped (Phase 2 already catches start/ via B12).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 3)"), World)) return true;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* TestActor = World->SpawnActor<AActor>(AActor::StaticClass(),
			FTransform(FVector(3000.f, 0.f, 0.f)), SP);
		if (!TestNotNull(TEXT("Phase 3: TestActor must spawn"), TestActor)) return true;

		URogueSignificanceComponent* Comp = NewObject<URogueSignificanceComponent>(
			TestActor, NAME_None, RF_Transient);
		Comp->bManageSignificance = true;

		// Disable deferred registration so BeginPlay registers synchronously.
		const FBoolProperty* WaitProp = FindFProperty<FBoolProperty>(
			URogueSignificanceComponent::StaticClass(), TEXT("bWaitOneFrame"));
		if (WaitProp)
			WaitProp->SetPropertyValue_InContainer(Comp, false);

		Comp->RegisterComponent(); // BeginPlay registers with manager on solution/

		const bool bWasRegistered = SigLodHelpers::IsRegistered(World, Comp);

		// Destroy triggers EndPlay on all components synchronously.
		// On solution/ EndPlay calls USignificanceManager::UnregisterObject(this).
		TestActor->Destroy();

		const bool bStillRegistered = SigLodHelpers::IsRegistered(World, Comp);

		if (bWasRegistered)
		{
			TestFalse(TEXT("B13: Component must be removed from the significance manager after EndPlay (actor destroyed)"),
				bStillRegistered);
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 4: B2/G2/G10 — deferred registration (bWaitOneFrame=true).
	//
	// With the default bWaitOneFrame=true, BeginPlay queues RegisterWithManager
	// for the next tick rather than calling it immediately. Immediately after
	// RegisterComponent() the component must NOT appear in the manager.
	//
	// Safety: the actor is destroyed synchronously before any tick fires the
	// deferred callback. After Destroy(), the component is pending-kill and
	// GetWorld() returns null, so RegisterWithManager() exits early (no-op).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 4)"), World)) return true;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* TestActor = World->SpawnActor<AActor>(AActor::StaticClass(),
			FTransform(FVector(4000.f, 0.f, 0.f)), SP);
		if (!TestNotNull(TEXT("Phase 4: TestActor must spawn"), TestActor)) return true;

		URogueSignificanceComponent* Comp = NewObject<URogueSignificanceComponent>(
			TestActor, NAME_None, RF_Transient);
		Comp->bManageSignificance = true;
		// bWaitOneFrame left at its default (true) — do NOT override it.

		// BeginPlay fires here; with bWaitOneFrame=true registration is deferred.
		Comp->RegisterComponent();

		const bool bRegisteredImmediately = SigLodHelpers::IsRegistered(World, Comp);

		// Destroy before the deferred tick fires; GetWorld() becomes null on the
		// pending-kill component, so RegisterWithManager() will be a no-op.
		TestActor->Destroy();

		TestFalse(TEXT("B2/G2: Component with bWaitOneFrame=true must NOT be registered immediately after BeginPlay (registration deferred by one frame)"),
			bRegisteredImmediately);

		return true;
	}));

	// -----------------------------------------------------------------------
// Phase 5: B14/B15/B17/B18/B21/B22 — significance calculation pipeline.
//
// Uses the base significance-manager update path to drive the component's
// registered calculation/post-update callbacks without depending on any
// custom tag-bucket redistribution behavior.
//
// Sequence:
//   a) viewpoint at actor location (distance=0)   → Highest tier  (B17/B22)
//      first update fires the notification path                     (B18)
//   b) global debug override forces a fixed tier                    (B14)
//   c) viewpoint 1 000 000 UU away (>> all thresholds) → Lowest    (B21)
//   d) actor set hidden, viewpoint at origin       → Hidden tier    (B15)
//
// Actor is destroyed after all checks so registration/unregistration stays
// self-contained inside this phase.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = SigLodHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be valid (Phase 5)"), World)) return true;

		URogueSignificanceManager* RogueMgr = SigLodHelpers::GetSigManager(World);
		if (!TestNotNull(TEXT("Phase 5: RogueSignificanceManager must exist"), RogueMgr)) return true;

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// Spawn at origin so distance calculations are predictable.
		AActor* TestActor = World->SpawnActor<AActor>(AActor::StaticClass(),
			FTransform(FVector::ZeroVector), SP);
		if (!TestNotNull(TEXT("Phase 5: TestActor must spawn"), TestActor)) return true;

		URogueSignificanceComponent* Comp = NewObject<URogueSignificanceComponent>(
			TestActor, NAME_None, RF_Transient);
		Comp->bManageSignificance = true;

		// Capture the constructor-initialized sentinel value before any registration or update.
		// B18: constructor sets CurrentSignificance=Invalid; first Update() must change it.
		const ESignificanceValue BeforeUpdate = Comp->CurrentSignificance;

		// Disable deferred registration so BeginPlay registers synchronously.
		const FBoolProperty* WaitProp = FindFProperty<FBoolProperty>(
			URogueSignificanceComponent::StaticClass(), TEXT("bWaitOneFrame"));
		if (WaitProp)
			WaitProp->SetPropertyValue_InContainer(Comp, false);

		// BeginPlay fires; registers synchronously on solution/.
		Comp->RegisterComponent();

		const bool bRegistered = SigLodHelpers::IsRegistered(World, Comp);
		if (bRegistered)
		{

			TArray<FTransform> Viewpoints;

			// --- (a) distance = 0: within Highest-tier threshold ---
			Viewpoints.Add(FTransform(FVector::ZeroVector)); // viewpoint at actor location
			RogueMgr->USignificanceManager::Update(Viewpoints);

			// B18: first update (stored value was Invalid sentinel) must trigger the
			// post-significance notification path, updating CurrentSignificance away from Invalid.
			TestTrue(TEXT("B18: CurrentSignificance must change from sentinel (Invalid) on first update, confirming the notification path fired"),
				BeforeUpdate == ESignificanceValue::Invalid && Comp->CurrentSignificance != ESignificanceValue::Invalid);

			// B17/B22: distance 0 is covered by the Highest-tier threshold.
			TestEqual(TEXT("B17/B22: Actor at distance 0 from viewpoint must receive Highest significance tier"),
				Comp->CurrentSignificance, ESignificanceValue::Highest);

			// --- (b) global debug override forces a fixed significance tier ---
			if (IConsoleVariable* ForceSigCVar =
				IConsoleManager::Get().FindConsoleVariable(TEXT("SigMan.ForceSignificance")))
			{
				ForceSigCVar->Set(static_cast<float>(ESignificanceValue::Medium));
				RogueMgr->USignificanceManager::Update(Viewpoints);

				TestEqual(TEXT("B14: SigMan.ForceSignificance must override distance-based calculation"),
					Comp->CurrentSignificance, ESignificanceValue::Medium);

				// Reset to the default disabled state before exercising normal distance mapping again.
				ForceSigCVar->Set(-1.0f);
			}

			// --- (c) distance = 1 000 000 UU: exceeds every configured threshold ---
			Viewpoints[0] = FTransform(FVector(1000000.f, 0.f, 0.f));
			RogueMgr->USignificanceManager::Update(Viewpoints);

			// B21: distance beyond all thresholds must yield the Lowest tier.
			TestEqual(TEXT("B21: Actor distance exceeding all thresholds must receive Lowest significance tier"),
				Comp->CurrentSignificance, ESignificanceValue::Lowest);

			// --- (d) actor hidden: hidden flag overrides distance calculation ---
			TestActor->SetActorHiddenInGame(true);
			Viewpoints[0] = FTransform(FVector::ZeroVector);
			RogueMgr->USignificanceManager::Update(Viewpoints);

			// B15: hidden actor + bInsignificantWhenOwnerIsHidden=true → Hidden tier.
			TestEqual(TEXT("B15: Hidden actor with bInsignificantWhenOwnerIsHidden=true must receive Hidden significance tier"),
				Comp->CurrentSignificance, ESignificanceValue::Hidden);

			// --- (e) multi-tier threshold mapping ---
			// Inject two explicit thresholds: Highest at 500 UU, Medium at 2000 UU.
			// Un-hide the actor so distance governs significance.
			// Viewpoint at 1000 UU is beyond the Highest band (500) but within the
			// Medium band (2000), so the result must be Medium — not Highest, not Lowest.
			//
			// A model that only checks threshold[0] and returns Lowest for everything
			// beyond it fails this assertion. The correct implementation iterates ALL
			// thresholds in order and returns the tier of the first that covers the
			// input distance.
			TestActor->SetActorHiddenInGame(false);

			Comp->Thresholds.Empty();
			Comp->Thresholds.Emplace(ESignificanceValue::Highest, 500.f);
			Comp->Thresholds.Emplace(ESignificanceValue::Medium, 2000.f);

			Viewpoints[0] = FTransform(FVector(1000.f, 0.f, 0.f)); // distance = 1000 UU from origin
			RogueMgr->USignificanceManager::Update(Viewpoints);

			// B20/GetSignificanceByDistance: 1000 > 500 (Highest exhausted), 1000 <= 2000 (Medium covers it).
			TestEqual(
				TEXT("// REQUIRED (B20): Distance 1000 UU must map to the Medium tier when thresholds are "
					 "[Highest@500, Medium@2000]. A model that short-circuits after the first threshold "
					 "and returns Lowest for any distance beyond it fails here."),
				Comp->CurrentSignificance, ESignificanceValue::Medium);
		}

		// Destroy synchronously; EndPlay unregisters the component before the
		// next natural Update() tick (which would crash for non-AICharacter tags).
		TestActor->Destroy();

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

#else
	AddWarning(TEXT("Editor-only test skipped (requires WITH_EDITOR)."));
#endif
	return true;
}
