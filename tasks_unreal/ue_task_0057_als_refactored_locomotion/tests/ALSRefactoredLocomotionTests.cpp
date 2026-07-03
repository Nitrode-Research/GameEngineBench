#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AlsCharacter.h"
#include "Utility/AlsGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlsLocomotionTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0057 (ALS-Refactored Locomotion Pipeline).
//
// EDITABLE FILES UNDER TEST: AlsCharacter.cpp / AlsCharacterMovementComponent.cpp.
// Two pipeline behaviors are gutted in start/ and restored in the solution:
//   • Stance application: start/ AAlsCharacter::ApplyDesiredStance() is empty {}, so a
//     requested crouch is never applied. The solution crouches/uncrouches based on
//     DesiredStance (when grounded and not in an action).
//   • Effective gait: start/ RefreshGait() does SetGait(DesiredGait) — it copies the
//     desired gait verbatim. The solution early-returns off-ground and derives the
//     ACTUAL gait from real movement speed, so a desired sprint the character is not
//     actually fast enough for resolves to walking.
//
// WHY NOT PIE: an earlier version of this suite drove the character through PIE
// (FStartPIECommand). That CANNOT work in this project: PIE creates the play world and
// then immediately tears it down ("Shutting down PIE online subsystems") because the
// frozen content's EOS/OnlineSubsystem (TVEOS_GameInstance / OnlineSubsystemEIK) fails
// to initialise. By the time a latent command runs, the PIE world is already gone, so
// no character is obtainable and every test fails on the solution too. Instead — like
// the working ue_task_0047 suite — we build a bare C++ Game world (no PIE, no game
// instance, no online subsystem), spawn the project's ALS character Blueprint (which
// carries a skeletal mesh, anim BP and populated Settings/MovementSettings so BeginPlay
// initialises the locomotion pipeline), force MOVE_Walking so ALS resolves Grounded,
// and drive the pipeline by ticking the world directly.
//
// OBSERVABLE PUBLIC CONTRACT (assertions use only these — no private state, no
// Calculate*/Refresh* internals, no exact speeds):
//   • void SetDesiredStance(FGameplayTag) / FGameplayTag GetStance()
//   • void SetDesiredGait(FGameplayTag)   / FGameplayTag GetGait()
//   • FGameplayTag GetLocomotionMode()
//   • UCapsuleComponent::GetScaledCapsuleHalfHeight()
// FGameplayTag is compared with operator== inside TestTrue.
//
// DISCRIMINATION: each test drives a desired state and asserts the resolved observable
// state. start/ either ignores the request (stance) or copies it blindly (gait), so
// each test fails on start/ and passes on the solution while asserting only categories
// / direction-of-change that any correct ALS implementation produces.
//
// NOT COVERED, and why: rotation application (needs sustained movement and yaw
// convergence that is timing-sensitive even when ticked deterministically); networked
// preservation of stance/gait/rotation across client prediction and server correction
// (needs a true multi-client session).
// ─────────────────────────────────────────────────────────────────────────────

namespace AlsLocomotionTests
{
	// Build a bare Game world WITH a physics scene and begun play (so spawned actors run
	// BeginPlay and tick), but no PIE / game instance / online subsystem.
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

	// Advance the simulation deterministically. A bare CreateWorld() world does not run
	// the spawned actor's registered tick functions via World->Tick, so we ALSO tick the
	// movement component (processes crouch/physics) and the character actor (the ALS
	// RefreshGait/RefreshLocomotion pipeline) explicitly, movement first so the character
	// refresh reads the updated movement state.
	static void TickWorld(UWorld* World, AAlsCharacter* Char, float Seconds, int32 Steps = 12)
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

	// Spawn the project's ALS character Blueprint. The ALS-Refactored demo character is
	// tried first (it is the closest match to the system under test); the native ALSXT
	// player/demo characters are fallbacks. All carry mesh + anim BP + Settings.
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
				UE_LOG(LogAlsLocomotionTests, Warning, TEXT("SpawnCharacter: LoadClass failed for %s"), Path);
				continue;
			}
			if (AAlsCharacter* C = World->SpawnActor<AAlsCharacter>(Cls, Loc, FRotator::ZeroRotator, Params))
			{
				UE_LOG(LogAlsLocomotionTests, Warning, TEXT("SpawnCharacter: spawned from %s"), Path);
				return C;
			}
			UE_LOG(LogAlsLocomotionTests, Warning, TEXT("SpawnCharacter: SpawnActor returned null for %s"), Path);
		}
		UE_LOG(LogAlsLocomotionTests, Warning, TEXT("SpawnCharacter: no character could be spawned"));
		return nullptr;
	}

	// Spawn a floor and a character, place the character on it, force walking so ALS
	// resolves LocomotionMode to Grounded, and tick until it settles. Returns the
	// grounded character, or nullptr if none could be spawned.
	static AAlsCharacter* MakeGroundedCharacter(UWorld* World)
	{
		SpawnFloor(World);
		AAlsCharacter* Char = SpawnCharacter(World, FVector(0.f, 0.f, 150.f));
		if (!Char) return nullptr;

		const float HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Char->SetActorRotation(FRotator::ZeroRotator);
		Char->SetActorLocation(FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		const float PreZ = Char->GetActorLocation().Z;
		TickWorld(World, Char, 1.0f, 12); // settle to Grounded, speed ~0
		UE_LOG(LogAlsLocomotionTests, Warning,
			TEXT("MakeGrounded: preZ=%.1f postZ=%.1f mode=%s moveMode=%d gait=%s halfH=%.1f"),
			PreZ, Char->GetActorLocation().Z, *Char->GetLocomotionMode().ToString(),
			(int32)Char->GetCharacterMovement()->MovementMode.GetValue(),
			*Char->GetGait().ToString(), Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		return Char;
	}
}

// ── REQUIRED: desired stance round-trips (crouch THEN uncrouch) ───────────────
// Spec: "transitions correctly between idle, moving, sprinting, crouching ... using the
// existing ALS state model" — a two-way contract. Requests Crouching and asserts the
// stance/capsule change, then requests Standing and asserts it returns. Fails on start/
// (ApplyDesiredStance is empty {} → the Crouching assertions fail). Passes on solution/
// (Crouch() then UnCrouch()). Robust: asserts the stance category and direction of
// capsule change, never an exact half-height.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsLocomotionDesiredStanceTest,
	"ALSRefactored.Locomotion.desired_stance_crouch_then_uncrouch",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsLocomotionDesiredStanceTest::RunTest(const FString&)
{
	UWorld* World = AlsLocomotionTests::MakeGameWorld();
	if (!TestNotNull(TEXT("Could not create a test world"), World))
	{
		return true;
	}

	AAlsCharacter* Char = AlsLocomotionTests::MakeGroundedCharacter(World);
	if (!Char)
	{
		AddError(TEXT("Could not spawn an AAlsCharacter — cannot test stance"));
		AlsLocomotionTests::TearDownWorld(World);
		return true;
	}

	if (!TestTrue(TEXT("Character must be Grounded before crouching (precondition)"),
		Char->GetLocomotionMode() == AlsLocomotionModeTags::Grounded))
	{
		AlsLocomotionTests::TearDownWorld(World);
		return true;
	}

	const float StandingHalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TestTrue(TEXT("Character must start Standing (precondition)"),
		Char->GetStance() == AlsStanceTags::Standing);

	// Crouch.
	Char->SetDesiredStance(AlsStanceTags::Crouching);
	AlsLocomotionTests::TickWorld(World, Char, 1.0f, 12);

	UE_LOG(LogAlsLocomotionTests, Warning,
		TEXT("AFTER CROUCH: stance=%s mode=%s halfH=%.1f (standing=%.1f) Z=%.1f bIsCrouched=%d"),
		*Char->GetStance().ToString(), *Char->GetLocomotionMode().ToString(),
		Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), StandingHalfHeight,
		Char->GetActorLocation().Z, Char->bIsCrouched ? 1 : 0);

	TestTrue(TEXT("Requesting Crouching must change the stance to Crouching"),
		Char->GetStance() == AlsStanceTags::Crouching);
	TestTrue(TEXT("Crouching must shrink the capsule below its standing half-height"),
		Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() < StandingHalfHeight - 1.0f);

	// Uncrouch (reverse transition: the solution's UnCrouch branch; start/ ignores it).
	Char->SetDesiredStance(AlsStanceTags::Standing);
	AlsLocomotionTests::TickWorld(World, Char, 1.0f, 12);

	TestTrue(TEXT("Requesting Standing again must return the stance to Standing"),
		Char->GetStance() == AlsStanceTags::Standing);
	TestTrue(TEXT("Uncrouching must restore the capsule to its standing half-height"),
		Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() >= StandingHalfHeight - 1.0f);

	AlsLocomotionTests::TearDownWorld(World);
	return true;
}

// ── REQUIRED: effective gait follows actual speed, not the desired gait ───────
// Spec: "Gait limits and effective gait respond to movement input and character state
// the way the rest of ALS expects" (not one universal rule). A stationary, grounded
// character requests Sprinting; the resolved gait must remain Walking because it is not
// actually moving fast enough to sprint. Fails on start/ (RefreshGait copies DesiredGait
// verbatim → GetGait() == Sprinting). Passes on solution/ (actual gait derived from real
// speed → Walking).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAlsLocomotionEffectiveGaitTest,
	"ALSRefactored.Locomotion.effective_gait_follows_actual_speed_not_desired",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAlsLocomotionEffectiveGaitTest::RunTest(const FString&)
{
	UWorld* World = AlsLocomotionTests::MakeGameWorld();
	if (!TestNotNull(TEXT("Could not create a test world"), World))
	{
		return true;
	}

	AAlsCharacter* Char = AlsLocomotionTests::MakeGroundedCharacter(World);
	if (!Char)
	{
		AddError(TEXT("Could not spawn an AAlsCharacter — cannot test gait"));
		AlsLocomotionTests::TearDownWorld(World);
		return true;
	}

	if (!TestTrue(TEXT("Character must be Grounded before gait check (precondition)"),
		Char->GetLocomotionMode() == AlsLocomotionModeTags::Grounded))
	{
		AlsLocomotionTests::TearDownWorld(World);
		return true;
	}

	// Keep the character stationary, then ask it to sprint.
	Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	Char->SetDesiredGait(AlsGaitTags::Sprinting);
	AlsLocomotionTests::TickWorld(World, Char, 1.0f, 12);

	UE_LOG(LogAlsLocomotionTests, Warning,
		TEXT("AFTER GAIT: gait=%s desired=%s mode=%s speed=%.1f"),
		*Char->GetGait().ToString(), *Char->GetDesiredGait().ToString(),
		*Char->GetLocomotionMode().ToString(), Char->GetCharacterMovement()->Velocity.Size());

	// A stationary character cannot actually be sprinting; the effective gait must
	// reflect the real (zero) speed, i.e. Walking.
	TestTrue(TEXT("Effective gait of a stationary character must not be Sprinting"),
		Char->GetGait() != AlsGaitTags::Sprinting);
	TestTrue(TEXT("Effective gait of a stationary character must resolve to Walking"),
		Char->GetGait() == AlsGaitTags::Walking);

	AlsLocomotionTests::TearDownWorld(World);
	return true;
}
