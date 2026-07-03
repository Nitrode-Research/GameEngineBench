#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"

#include "AlsCharacter.h"
#include "AlsxtCharacter.h"
#include "AlsxtCharacterAdvanced.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtTargetLockInterface.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"

// The test pokes component internals (CombatSettings) that ship protected.
// This is test-only access, scoped to this header.
#define protected public
#include "Components/Character/AlsxtCombatComponent.h"
#undef protected
#include "AlsCameraComponent.h"
#include "AlsCameraSettings.h"
#include "Utility/AlsxtStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral coverage notes for ue_task_0047 (ALSXT Combat Lock-On and Melee)
//
// These tests exercise the lock-on TARGET ACQUISITION behavior of the spec:
//   "While in a valid state, the component can FIND and LOCK ONTO suitable nearby
//    targets ... cannot lock targets outside a practical range ... produces the
//    per-target data lock-on relies on (validity, distance, angle)."
//
// They are constructed entirely in C++ (no level, PIE, GameMode, or content) and
// assert only OBSERVABLE BEHAVIOR through the public TraceForTargets() contract —
// never private members, montage names, or call sequencing. A compiling stub
// (TraceForTargets => Targets.Reset()) returns an empty array and therefore fails
// every acquisition assertion, so each test discriminates start/ from solution/.
//
// Why the other spec clauses (cycling, highlight, attack montage/state, melee
// routing, once-per-swing) are NOT covered here: in this bare-C++ harness they
// are unreachable without crashing. Cycling/closest-target are gated behind
// IsDesiredAiming()+overlay-mode state; the attack path dereferences the BP
// combat-settings/montage CONTENT asset (null here); highlight needs a skeletal
// mesh with materials; and the obstruction-unlock clause is not even
// discriminable because IsTartgetObstructed() is identical in start/ and
// solution/. Covering those requires a PIE test driving the project's Blueprint
// character with its combat content, not a headless component harness.
// ─────────────────────────────────────────────────────────────────────────────

namespace CombatTestHelpers
{
	// Build a minimal Game world WITH a physics scene but WITHOUT calling
	// World->BeginPlay(). Spawned actors still register their components (so
	// collision/physics is live and sweeps work), but never run BeginPlay —
	// which is what avoids the ALS locomotion-init null deref (the character's
	// BP-assigned Settings/MovementSettings are unset in a bare C++ spawn).
	static UWorld* MakePhysicsWorld()
	{
		if (!GEngine) return nullptr;
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (!World) return nullptr;
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	static void TearDownWorld(UWorld* World)
	{
		if (!World || !GEngine) return;
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	static bool SetObjectProperty(UObject* Object, const TCHAR* PropertyName, UObject* Value)
	{
		if (!IsValid(Object))
		{
			return false;
		}

		FObjectPropertyBase* Property =
			FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
		if (Property == nullptr)
		{
			return false;
		}

		Property->SetObjectPropertyValue_InContainer(Object, Value);
		return true;
	}

	template <typename TCharacter>
	static TCharacter* SpawnAlsCharacterAt(UWorld* World, const FVector& Loc)
	{
		FTransform SpawnTransform(FRotator::ZeroRotator, Loc);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		TCharacter* Character = World->SpawnActorDeferred<TCharacter>(TCharacter::StaticClass(), SpawnTransform);
		if (!IsValid(Character))
		{
			return nullptr;
		}

		SetObjectProperty(Character, TEXT("Settings"), NewObject<UAlsCharacterSettings>(Character));
		SetObjectProperty(Character, TEXT("MovementSettings"), NewObject<UAlsMovementSettings>(Character));

		UGameplayStatics::FinishSpawningActor(Character, SpawnTransform);
		return Character;
	}

	static bool InjectCameraSettings(UAlsCameraComponent* Camera)
	{
		if (!IsValid(Camera))
		{
			return false;
		}

		FObjectPropertyBase* SettingsProperty =
			FindFProperty<FObjectPropertyBase>(UAlsCameraComponent::StaticClass(), TEXT("Settings"));
		if (SettingsProperty == nullptr)
		{
			return false;
		}

		SettingsProperty->SetObjectPropertyValue_InContainer(
			Camera, NewObject<UAlsCameraSettings>(Camera));
		return true;
	}

	// A ready-to-trace attacker: bare physics world, an AAlsxtCharacterAdvanced
	// whose UAlsxtCombatComponent is built in its C++ ctor, camera settings
	// injected, a permissive Pawn-object trace, and the exact forward sweep ray
	// the solution computes from the camera. bOk is false if any step failed.
	struct FCombatFixture
	{
		UWorld* World = nullptr;
		AAlsxtCharacterAdvanced* Attacker = nullptr;
		UAlsxtCombatComponent* Combat = nullptr;
		UAlsCameraComponent* Cam = nullptr;
		FVector RayStart = FVector::ZeroVector;
		FVector RayEnd = FVector::ZeroVector;
		bool bOk = false;

		FVector RayMid() const { return (RayStart + RayEnd) * 0.5f; }
	};

	static FCombatFixture BuildCombatFixture()
	{
		FCombatFixture F;
		F.World = MakePhysicsWorld();
		if (!F.World) return F;

		F.Attacker = SpawnAlsCharacterAt<AAlsxtCharacterAdvanced>(F.World, FVector::ZeroVector);
		if (!IsValid(F.Attacker)) return F;

		F.Combat = F.Attacker->FindComponentByClass<UAlsxtCombatComponent>();
		if (!IsValid(F.Combat)) return F;

		// GetFirstPersonCameraLocation() dereferences the camera's BP Settings
		// (null on a bare C++ spawn); inject a default so the trace's camera read
		// is safe.
		F.Cam = IAlsxtCharacterInterface::Execute_GetCharacterCamera(F.Attacker);
		if (!IsValid(F.Cam)) return F;
		if (!InjectCameraSettings(F.Cam)) return F;

		// Make the trace permissive and well-defined: query Pawn objects, large
		// lock distance. TraceAreaHalfSize keeps its generous default.
		F.Combat->CombatSettings.TargetTraceObjectTypes.Empty();
		F.Combat->CombatSettings.TargetTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		F.Combat->CombatSettings.MaxLockDistance = 1.0e6f;

		const FVector Fwd = F.Attacker->GetActorForwardVector();
		const FVector CamLoc = F.Cam->GetFirstPersonCameraLocation();
		F.RayStart = Fwd * 150.f + CamLoc;
		F.RayEnd = Fwd * 200.f + F.RayStart;
		F.bOk = true;
		return F;
	}

	// A lock-on-capable target (AAlsxtCharacter implements IAlsxtTargetLockInterface)
	// with a Pawn-collision capsule guaranteed to answer the object sweep.
	static AAlsxtCharacter* SpawnTarget(UWorld* World, const FVector& Loc)
	{
		AAlsxtCharacter* Target = SpawnAlsCharacterAt<AAlsxtCharacter>(World, Loc);
		if (IsValid(Target))
		{
			if (UCapsuleComponent* Capsule = Target->GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Capsule->SetCollisionObjectType(ECC_Pawn);
				Capsule->SetCollisionResponseToAllChannels(ECR_Block);
				Capsule->UpdateOverlaps();
			}
		}
		return Target;
	}

	static const FTargetHitResultEntry* FindEntryForActor(
		const TArray<FTargetHitResultEntry>& Entries, const AActor* Actor)
	{
		for (const FTargetHitResultEntry& Entry : Entries)
		{
			if (Entry.HitResult.GetActor() == Actor)
			{
				return &Entry;
			}
		}
		return nullptr;
	}
}

// ── Behavior: the component finds and acquires a suitable nearby target ───────
//
// A lock-on target placed on the forward sweep ray must be collected by
// TraceForTargets. The stub returns an empty array, so an unimplemented combat
// system cannot acquire the target.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTraceFindsTargetTest,
	"ALSXT.CombatLockOnMelee.trace_for_targets_finds_placed_character",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCombatTraceFindsTargetTest::RunTest(const FString&)
{
	CombatTestHelpers::FCombatFixture F = CombatTestHelpers::BuildCombatFixture();
	if (!TestTrue(TEXT("FIXTURE: combat attacker/world could not be built"), F.bOk))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	AAlsxtCharacter* Target = CombatTestHelpers::SpawnTarget(F.World, F.RayMid());
	if (!TestNotNull(TEXT("FIXTURE: failed to spawn lock-on target"), Target))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	TArray<FTargetHitResultEntry> OutHits;
	F.Combat->TraceForTargets(OutHits);

	const FTargetHitResultEntry* Entry =
		CombatTestHelpers::FindEntryForActor(OutHits, Target);

	TestNotNull(TEXT("TraceForTargets must acquire the suitable lock-on target placed in "
		"front of the character. The solution sweeps forward from the camera and collects "
		"valid targets; the stub returns an empty array, so an unimplemented combat system "
		"fails here."), Entry);

	CombatTestHelpers::TearDownWorld(F.World);
	return true;
}

// ── Behavior: targets are only acquired within the practical lock range ───────
//
// The same target is rejected when it sits beyond MaxLockDistance and accepted
// when it is within it. The "accepted within range" half cannot pass on the
// empty stub, so the test discriminates; together the two halves prove the
// distance rule is actually applied rather than every overlap being locked.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTraceRespectsRangeTest,
	"ALSXT.CombatLockOnMelee.trace_respects_max_lock_distance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCombatTraceRespectsRangeTest::RunTest(const FString&)
{
	CombatTestHelpers::FCombatFixture F = CombatTestHelpers::BuildCombatFixture();
	if (!TestTrue(TEXT("FIXTURE: combat attacker/world could not be built"), F.bOk))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	AAlsxtCharacter* Target = CombatTestHelpers::SpawnTarget(F.World, F.RayMid());
	if (!TestNotNull(TEXT("FIXTURE: failed to spawn lock-on target"), Target))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	// Within range: target on the ray, generous lock distance -> acquired.
	F.Combat->CombatSettings.MaxLockDistance = 1.0e6f;
	TArray<FTargetHitResultEntry> InRangeHits;
	F.Combat->TraceForTargets(InRangeHits);
	TestNotNull(TEXT("A target within MaxLockDistance must be acquired."),
		CombatTestHelpers::FindEntryForActor(InRangeHits, Target));

	// Beyond range: same overlap, tiny lock distance -> rejected. The target sits
	// hundreds of units ahead, so a 1cm lock range must exclude it.
	F.Combat->CombatSettings.MaxLockDistance = 1.0f;
	TArray<FTargetHitResultEntry> OutOfRangeHits;
	F.Combat->TraceForTargets(OutOfRangeHits);
	TestNull(TEXT("A target beyond MaxLockDistance must NOT be acquired — lock-on respects "
		"the practical range from settings rather than locking every overlap."),
		CombatTestHelpers::FindEntryForActor(OutOfRangeHits, Target));

	CombatTestHelpers::TearDownWorld(F.World);
	return true;
}

// ── Behavior: acquired targets carry usable lock-on data ──────────────────────
//
// Lock-on cycling and rotation downstream consume each entry's validity,
// distance, and angle. An acquired entry must be marked valid and carry a real
// (non-sentinel) distance and a bounded angle. The empty stub produces no entry,
// so this fails on an unimplemented system.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatTraceProducesDataTest,
	"ALSXT.CombatLockOnMelee.trace_produces_valid_lockon_data",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCombatTraceProducesDataTest::RunTest(const FString&)
{
	CombatTestHelpers::FCombatFixture F = CombatTestHelpers::BuildCombatFixture();
	if (!TestTrue(TEXT("FIXTURE: combat attacker/world could not be built"), F.bOk))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	AAlsxtCharacter* Target = CombatTestHelpers::SpawnTarget(F.World, F.RayMid());
	if (!TestNotNull(TEXT("FIXTURE: failed to spawn lock-on target"), Target))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	TArray<FTargetHitResultEntry> OutHits;
	F.Combat->TraceForTargets(OutHits);

	const FTargetHitResultEntry* Entry =
		CombatTestHelpers::FindEntryForActor(OutHits, Target);
	if (!TestNotNull(TEXT("TraceForTargets must acquire the target before its data can be checked."),
		Entry))
	{
		CombatTestHelpers::TearDownWorld(F.World);
		return true;
	}

	TestTrue(TEXT("An acquired lock-on entry must be marked valid."), Entry->Valid);

	TestTrue(TEXT("An acquired entry must carry a real measured distance to the player "
		"(positive and finite), not the cleared sentinel."),
		Entry->DistanceFromPlayer > 0.0f && Entry->DistanceFromPlayer < 1.0e7f);

	TestTrue(TEXT("An acquired entry must carry a bounded angle-from-center used by "
		"left/right cycling, not the cleared 361-degree sentinel."),
		FMath::Abs(Entry->AngleFromCenter) <= 180.0f);

	CombatTestHelpers::TearDownWorld(F.World);
	return true;
}
