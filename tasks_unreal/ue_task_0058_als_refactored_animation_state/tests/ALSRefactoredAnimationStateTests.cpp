#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "AlsCharacter.h"

// The animation-state members we observe (LocomotionState / ViewState / bPendingUpdate)
// are protected/private with no public getters, so — like the original suite — we open
// them up for this test translation unit only.
#define protected public
#define private public
#include "AlsAnimationInstance.h"
#undef private
#undef protected

DEFINE_LOG_CATEGORY_STATIC(LogAlsAnimStateTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0058 (ALS-Refactored Animation State Evaluation).
//
// EDITABLE FILE UNDER TEST: AlsAnimationInstance.cpp. In start/ the instance is a
// FROZEN / no-op variant: RefreshLocomotionOnGameThread() early-returns (so it never
// copies the movement component's values), the thread-safe update is gutted and never
// clears bPendingUpdate, and RefreshView hard-codes ViewState.HeadBlendAmount = 0. The
// solution derives all of these from the owning character every frame.
//
// WHY NOT PIE: an earlier version drove this through PIE (FStartPIECommand). That CANNOT
// work in this project — PIE creates the play world and immediately tears it down
// ("Shutting down PIE online subsystems") because the frozen content's EOS/OnlineSubsystem
// fails to init, so no character is ever obtainable and every test fails on the solution
// too. Instead we build a bare C++ Game world (no PIE / game instance / online subsystem),
// spawn the project's ALS character Blueprint (mesh + ALS anim instance + populated
// settings), force MOVE_Walking, and drive the pipeline by ticking the movement, the
// character actor AND the skeletal mesh (which runs the anim instance's update) directly.
// The mesh is forced to AlwaysTickPoseAndRefreshBones because a headless mesh is never
// rendered and would otherwise skip its anim tick.
//
// OBSERVABLE (frozen at a default in start/, derived from the character in solution/):
//   • LocomotionState.MaxAcceleration / .WalkableFloorAngleCos — copied straight from the
//     movement component in RefreshLocomotionOnGameThread(), which runs on the GAME THREAD
//     inside NativeUpdateAnimation. start/ early-returns there, so they stay at their 0
//     defaults; the solution derives the positive configured values. (MaxBrakingDeceleration
//     is unusable: ALS brakes from a curve, so it is 0 on the solution too.)
//
// WHY ONLY GAME-THREAD STATE: this harness drives the anim instance via Mesh->TickAnimation,
// which runs NativeUpdateAnimation (and thus the *OnGameThread refreshes) but NOT the
// thread-safe update (NativeThreadSafeUpdateAnimation) — that completes during the mesh's
// full component tick, which cannot be invoked manually (it trips an internal
// high-priority-tick assertion). So thread-safe-derived observables (bPendingUpdate,
// LayeringState/PoseState, the view-curve HeadBlendAmount) sit at their defaults on BOTH
// start/ and solution/ here and do not discriminate — they were measured and dropped. The
// movement-mirrored locomotion scalars are the robust, game-thread discriminator.
//
// Access to these private members is via the file-scoped `#define private public` above.
// ─────────────────────────────────────────────────────────────────────────────

namespace AlsAnimStateTests
{
	static UWorld* MakeGameWorld()
	{
		if (!GEngine) return nullptr;
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (!World) return nullptr;
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		return World;
	}

	static void TearDownWorld(UWorld* World)
	{
		if (!World || !GEngine) return;
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	// A bare CreateWorld() world does not run spawned actors' registered tick functions via
	// World->Tick, so we tick the movement component, the character actor (game-thread ALS
	// refresh) and the skeletal mesh (anim instance update) explicitly each step.
	static void TickWorld(UWorld* World, AAlsCharacter* Char, float Seconds, int32 Steps = 18)
	{
		if (!World) return;
		const float Dt = Seconds / FMath::Max(1, Steps);
		for (int32 i = 0; i < Steps; ++i)
		{
			World->Tick(LEVELTICK_All, Dt);
			if (IsValid(Char))
			{
				if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
				{
					Move->TickComponent(Dt, LEVELTICK_All, &Move->PrimaryComponentTick);
				}
				Char->TickActor(Dt, LEVELTICK_All, Char->PrimaryActorTick);
				if (USkeletalMeshComponent* Mesh = Char->GetMesh())
				{
					// Drive the anim instance update directly. Calling the mesh's
					// TickComponent manually trips an internal high-priority-tick assertion,
					// so use TickAnimation, which runs NativeUpdateAnimation + (with parallel
					// anim disabled) the thread-safe update inline.
					Mesh->TickAnimation(Dt, false);
				}
			}
		}
	}

	static void SpawnFloor(UWorld* World)
	{
		if (!World) return;
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube) return;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(
			FVector(0.f, 0.f, -20.f), FRotator::ZeroRotator, Params); // top at Z=0
		if (!Floor) return;

		UStaticMeshComponent* Comp = Floor->GetStaticMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Cube);
		Floor->SetActorScale3D(FVector(20.f, 20.f, 0.4f));
		Floor->SetActorLocation(FVector(0.f, 0.f, -20.f));
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionObjectType(ECC_WorldStatic);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);
		Comp->UpdateCollisionProfile();
	}

	static AAlsCharacter* SpawnCharacter(UWorld* World, const FVector& Loc)
	{
		if (!World) return nullptr;
		static const TCHAR* Paths[] = {
			TEXT("/ALS/ALS/Character/B_Als_Character.B_Als_Character_C"),
			TEXT("/ALSXT/ALS/Character/B_AlsxtCharacterPlayer.B_AlsxtCharacterPlayer_C"),
			TEXT("/ALSXT/ALS/Character/B_ALSXT_Character.B_ALSXT_Character_C"),
		};

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (const TCHAR* Path : Paths)
		{
			UClass* Cls = LoadClass<AAlsCharacter>(nullptr, Path);
			if (Cls == nullptr)
			{
				UE_LOG(LogAlsAnimStateTests, Warning, TEXT("SpawnCharacter: LoadClass failed for %s"), Path);
				continue;
			}
			if (AAlsCharacter* C = World->SpawnActor<AAlsCharacter>(Cls, Loc, FRotator::ZeroRotator, Params))
			{
				UE_LOG(LogAlsAnimStateTests, Warning, TEXT("SpawnCharacter: spawned from %s"), Path);
				return C;
			}
			UE_LOG(LogAlsAnimStateTests, Warning, TEXT("SpawnCharacter: SpawnActor returned null for %s"), Path);
		}
		UE_LOG(LogAlsAnimStateTests, Warning, TEXT("SpawnCharacter: no character could be spawned"));
		return nullptr;
	}

	static UAlsAnimationInstance* GetAlsAnimInstance(AAlsCharacter* Char)
	{
		if (!Char) return nullptr;
		USkeletalMeshComponent* Mesh = Char->GetMesh();
		if (!Mesh) return nullptr;
		return Cast<UAlsAnimationInstance>(Mesh->GetAnimInstance());
	}

	// Spawn a floor + character, ground it (force walking), force the mesh to tick its pose
	// even though it is never rendered, and tick long enough for the anim instance to run
	// several real update cycles. Returns the grounded character, or nullptr on failure.
	static AAlsCharacter* MakeGroundedCharacter(UWorld* World)
	{
		// Force single-threaded anim evaluation so TickAnimation runs the thread-safe
		// update (which clears bPendingUpdate and derives ViewState) inline on this thread.
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.ParallelAnimUpdate")))
		{
			CVar->Set(0, ECVF_SetByCode);
		}

		SpawnFloor(World);
		AAlsCharacter* Char = SpawnCharacter(World, FVector(0.f, 0.f, 150.f));
		if (!Char) return nullptr;

		if (USkeletalMeshComponent* Mesh = Char->GetMesh())
		{
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			Mesh->bEnableUpdateRateOptimizations = false;
		}

		const float HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Char->SetActorRotation(FRotator::ZeroRotator);
		Char->SetActorLocation(FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		TickWorld(World, Char, 1.5f, 18); // register + run several anim update cycles
		return Char;
	}

	// Build the full fixture; emits an AddError (not a silent pass) on any failure so a
	// content/harness regression surfaces. Caller must TearDownWorld(F.World).
	struct FFixture
	{
		UWorld* World = nullptr;
		AAlsCharacter* Char = nullptr;
		UAlsAnimationInstance* Anim = nullptr;
	};

	static FFixture Build(FAutomationTestBase* Test)
	{
		FFixture F;
		F.World = MakeGameWorld();
		if (!F.World)
		{
			Test->AddError(TEXT("Could not create a bare game world"));
			return F;
		}
		F.Char = MakeGroundedCharacter(F.World);
		if (!F.Char)
		{
			Test->AddError(TEXT("Could not spawn an ALS Blueprint character — cannot evaluate animation state"));
			return F;
		}
		F.Anim = GetAlsAnimInstance(F.Char);
		if (!F.Anim)
		{
			Test->AddError(TEXT("Character mesh is not running a UAlsAnimationInstance — cannot evaluate animation state"));
			return F;
		}
		UE_LOG(LogAlsAnimStateTests, Warning,
			TEXT("ANIM STATE: maxAccel=%.1f walkCos=%.3f pending=%d headBlend=%.3f"),
			F.Anim->LocomotionState.MaxAcceleration, F.Anim->LocomotionState.WalkableFloorAngleCos,
			F.Anim->bPendingUpdate ? 1 : 0, F.Anim->ViewState.HeadBlendAmount);
		return F;
	}
}

// ── REQUIRED — locomotion state is derived from the character after ticking ───────
// MaxAcceleration / WalkableFloorAngleCos are copied from the movement component in
// RefreshLocomotionOnGameThread(). start/ early-returns there, so they stay 0; solution/
// derives non-zero values. Catches the gutted locomotion refresh / a frozen instance.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsAnimLocomotionDerivedTest,
	"ALSRefactored.AnimationState.locomotion_state_derived_from_character_after_tick",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsAnimLocomotionDerivedTest::RunTest(const FString&)
{
	AlsAnimStateTests::FFixture F = AlsAnimStateTests::Build(this);
	if (F.Anim)
	{
		// Both are copied straight from the movement component in
		// RefreshLocomotionOnGameThread(); start/ early-returns there, so they stay at their
		// 0 defaults; the solution derives the positive configured values. Asserting > 0
		// (not exact values) stays robust to alternative settings. (MaxBrakingDeceleration
		// is NOT used: ALS drives braking from a curve, so GetMaxBrakingDeceleration() is 0
		// even on the solution — it would not discriminate.)
		TestTrue(TEXT("LocomotionState.MaxAcceleration must be derived from the movement component (> 0), not frozen at its default"),
			F.Anim->LocomotionState.MaxAcceleration > 0.0f);
		TestTrue(TEXT("LocomotionState.WalkableFloorAngleCos must be derived from the movement component (> 0), not frozen at its default"),
			F.Anim->LocomotionState.WalkableFloorAngleCos > 0.0f);
	}
	AlsAnimStateTests::TearDownWorld(F.World);
	return true;
}

