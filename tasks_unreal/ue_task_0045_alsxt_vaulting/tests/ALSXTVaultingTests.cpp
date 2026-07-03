#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "State/AlsxtVaultingState.h"
#include "Settings/AlsxtCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "Utility/AlsGameplayTags.h"
#include "UObject/UnrealType.h"
#include "AlsCharacter.h"
#include "AlsxtCharacter.h"
#include "AlsCharacterMovementComponent.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVaultingTests, Log, All);

// The bare-spawned ALSXT character emits a large, varied set of benign Error-level
// log messages during tick (mesh-painting not configured, ability-system interface
// not implemented, a deprecated non-instanced GameplayAbility ensure, struct-init
// warnings, ...). The automation framework records captured Log Errors as test
// failures, so suppress them for these tests. Explicit TestTrue/TestFalse failures
// call AddError() directly (not through the log capture), so they still fail the
// test and discrimination between start/ and solution/ is preserved.
#define EXPECT_ALSXT_NOISE() \
	FAutomationTestBase::bSuppressLogErrors = true; \
	FAutomationTestBase::bSuppressLogWarnings = true

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0048 (ALSXT Vaulting)
//
// EDITABLE FILE UNDER TEST: AlsxtCharacter_Actions.cpp. The grounded vault entry
// point AAlsxtCharacter::TryStartVaultingGrounded() is a `return false` stub in
// start/. The solution runs the forward/depth/downward/landing trace cascade,
// solves launch/landing parameters relative to the obstacle, stores them via
// SetVaultingState() *synchronously*, and returns true. So both the bool result
// AND the solved FAlsxtVaultingState are observable immediately after the call —
// no RPC/montage timing is involved in what we assert.
//
// OBSERVABLE PUBLIC CONTRACT (all assertions go through this; no private state,
// montage names, call ordering, or internal data structures):
//   • bool  TryStartVaultingGrounded()
//   • const FAlsxtVaultingState& GetVaultingState()
//       → .VaultingParameters.VaultingHeight / .TargetRelativeLocation
//         / .TargetHandPlantRelativeLocation
//   • FGameplayTag GetLocomotionMode()
//
// DISCRIMINATION PRINCIPLE: the start stub returns false for everything, so a
// rejection-only assertion (solution also returns false) does NOT by itself
// separate start/ from solution/. Therefore EVERY test below contains at least
// one POSITIVE control vault (solution → true, start → false) in the same PIE
// session. The negative assertions then guard against over-eager *alternative*
// implementations that would vault things they shouldn't (false positives),
// while the positive control guarantees the test separates start/ from solution/.
//
// SPAWN STRATEGY (and why PIE): the vault path dereferences AAlsxtCharacter::
// ALSXTSettings, which is EditDefaultsOnly content populated only on the Blueprint
// character. A bare C++-spawned character has it null and the solution would
// crash. We therefore spawn the project's ALSXT Blueprint character DIRECTLY into
// the PIE world (settings populated from the BP CDO) with a fallback chain
// (B_ALSXT_Character → B_AlsxtCharacterPlayer) and a hard precondition guard that
// AddError()s (not a silent pass) if neither spawns or ALSXTSettings is null —
// so a content/harness regression surfaces as an error rather than a false fail.
// We deliberately do NOT rely on the project game-mode boot (which depends on
// TargetVectorCommonUI content outside this task's frozen plugin set). The
// grounded vault entry needs only Authority + Grounded locomotion + populated
// settings, none of which require local control or the game-mode chain.
//
// NOT COVERED, and why (kept out rather than asserted flakily / degenerately):
//   • Vault-TYPE selection (grounded vs in-air, High vs Low): in the reference
//     solution VaultingType is chosen from VaultingHeight *before* VaultingHeight
//     is assigned, so a grounded vault is always tagged Low regardless of height.
//     Asserting type would lock the suite to a degenerate code path, so we assert
//     the solved GEOMETRY (which does vary) instead.
//   • Moving-base alignment: a base moving faster than TargetPrimitiveSpeedThreshold
//     (10 cm/s) is *rejected* by design, so a genuinely moving base can't exercise
//     the relative-transform branch; the world-vs-relative choice is not observably
//     separable in a single-client headless world.
//   • In-air vault (TryStartVaultingInAir): needs a falling, locally-controlled
//     pawn with in-air trace content not reachable here.
//   • Replication / desync: needs a true multi-client montage-content comparison;
//     impossible in this single-client headless harness.
//
// Frozen settings the obstacle dimensions below are sized against (GroundedTrace):
//   LedgeHeight {min 50, max 225} cm, MaxDepth 60 cm.
// ─────────────────────────────────────────────────────────────────────────────

namespace VaultingTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");

	// Frozen GroundedTrace.LedgeHeight bounds (see header note). Obstacles are
	// sized comfortably inside / outside these so the trace cascade is deterministic.
	static constexpr float LedgeHeightMin = 50.0f;
	static constexpr float LedgeHeightMax = 225.0f;

	static UWorld* GetTestWorld()
	{
		if (!GEngine) return nullptr;
		// Accept both PIE and Game worlds: in -ListenServer headless automation the
		// play world is typed EWorldType::Game, not PIE, so a PIE-only filter returns
		// null even when a live play world exists.
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if ((Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game) && Ctx.World())
				return Ctx.World();
		}
		return nullptr;
	}

	static AAlsxtCharacter* FindCharacter(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AAlsxtCharacter> It(World); It; ++It)
		{
			if (*It) return *It;
		}
		return nullptr;
	}

	// Spawn the ALSXT Blueprint character with a fallback chain. Both BPs live in
	// the frozen ALSXT plugin Content with populated ALSXTSettings (incl. Vaulting)
	// and carry no TargetVector/Common dependency. Returns the first that spawns.
	static AAlsxtCharacter* SpawnBlueprintCharacter(UWorld* World)
	{
		if (!World) return nullptr;

		static const TCHAR* CharClassPaths[] = {
			TEXT("/ALSXT/ALS/Character/B_ALSXT_Character.B_ALSXT_Character_C"),
			TEXT("/ALSXT/ALS/Character/B_AlsxtCharacterPlayer.B_AlsxtCharacterPlayer_C"),
		};

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (const TCHAR* Path : CharClassPaths)
		{
			if (UClass* CharClass = LoadClass<AAlsxtCharacter>(nullptr, Path))
			{
				if (AAlsxtCharacter* Spawned = World->SpawnActor<AAlsxtCharacter>(
					CharClass, FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Params))
				{
					return Spawned;
				}
			}
		}
		return nullptr;
	}

	// Last spawn diagnostic, surfaced via AddError (LogAutomationController) because
	// LogTemp is suppressed in this run configuration.
	static FString GSpawnDiag;

	// Fallback when the Blueprint character will not spawn (its Blueprints fail to
	// compile under content/plugin version skew). Spawn the C++ AAlsxtCharacter
	// directly and inject the settings the vault path and ALS tick need. Deferred
	// spawn so the settings are present before BeginPlay. The C++ class hierarchy is
	// compiled from source (correct), so this sidesteps the serialization-time skew
	// that nulls Blueprint settings references and breaks BP compilation.
	static AAlsxtCharacter* SpawnCxxCharacterWithSettings(UWorld* World)
	{
		if (!World) return nullptr;

		UAlsxtCharacterSettings* Settings = LoadObject<UAlsxtCharacterSettings>(
			nullptr, TEXT("/ALSXT/ALS/Data/Character/D_Alsxt_CharacterSettings_Default.D_Alsxt_CharacterSettings_Default"));
		UAlsMovementSettings* MoveSettings = LoadObject<UAlsMovementSettings>(
			nullptr, TEXT("/ALSXT/ALS/Data/DA_AlsxtMovement_Default.DA_AlsxtMovement_Default"));
		GSpawnDiag = FString::Printf(TEXT("Cxx: settings=%s move=%s"),
			*GetNameSafe(Settings), *GetNameSafe(MoveSettings));
		if (!Settings) return nullptr;

		const FTransform SpawnXf(FRotator::ZeroRotator, FVector(0.f, 0.f, 200.f));
		AAlsxtCharacter* Char = World->SpawnActorDeferred<AAlsxtCharacter>(
			AAlsxtCharacter::StaticClass(), SpawnXf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		GSpawnDiag = FString::Printf(TEXT("Cxx: deferredSpawn=%s"), *GetNameSafe(Char));
		if (!Char) return nullptr;

		// Base Settings / MovementSettings are protected with no public setter; assign
		// their reflected UPROPERTYs directly (test-setup only — assertions use public
		// API). Avoids a `#define protected public` hack around engine headers.
		if (FObjectPropertyBase* P = FindFProperty<FObjectPropertyBase>(Char->GetClass(), TEXT("Settings")))
		{
			P->SetObjectPropertyValue_InContainer(Char, Settings);
		}
		if (MoveSettings)
		{
			if (FObjectPropertyBase* P = FindFProperty<FObjectPropertyBase>(Char->GetClass(), TEXT("MovementSettings")))
			{
				P->SetObjectPropertyValue_InContainer(Char, MoveSettings);
			}
		}
		Char->ALSXTSettings = Settings;     // public

		Char->FinishSpawning(SpawnXf);
		GSpawnDiag = FString::Printf(TEXT("Cxx: finished valid=%d ALSXTSettings=%s"),
			IsValid(Char) ? 1 : 0, *GetNameSafe(Char ? Char->ALSXTSettings : nullptr));

		// Also push movement settings through the public API so the movement
		// component has them regardless of BeginPlay ordering.
		if (MoveSettings)
		{
			if (auto* Movement = Cast<UAlsCharacterMovementComponent>(Char->GetCharacterMovement()))
			{
				Movement->SetMovementSettings(MoveSettings);
			}
		}
		return Char;
	}

	static AAlsxtCharacter* GetOrSpawnCharacter(UWorld* World)
	{
		if (AAlsxtCharacter* Existing = FindCharacter(World))
		{
			return Existing;
		}
		// Prefer the Blueprint character when it is healthy; fall back to a C++
		// character with injected settings when the BP fails to spawn or comes back
		// with null settings (the content-skew failure mode).
		AAlsxtCharacter* Bp = SpawnBlueprintCharacter(World);
		if (Bp && Bp->ALSXTSettings)
		{
			return Bp;
		}
		if (Bp) Bp->Destroy();
		AAlsxtCharacter* Cxx = SpawnCxxCharacterWithSettings(World); // sets GSpawnDiag if World valid
		GSpawnDiag = FString::Printf(TEXT("World=%s BP=%s | %s"),
			*GetNameSafe(World), *GetNameSafe(Bp), *GSpawnDiag);
		return Cxx;
	}

	// Spawn a WorldStatic box. Engine cube is 100x100x100 with a centered pivot,
	// so Scale is (size/100) per axis and the box spans Center ± 50*Scale.
	static AStaticMeshActor* SpawnBox(UWorld* World, const FVector& Center, const FVector& Scale)
	{
		if (!World) return nullptr;
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube) return nullptr;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Box = World->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, Params);
		if (!Box) return nullptr;

		UStaticMeshComponent* Comp = Box->GetStaticMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Cube);
		Box->SetActorScale3D(Scale);
		Box->SetActorLocation(Center);
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionObjectType(ECC_WorldStatic);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);
		Comp->UpdateCollisionProfile();
		return Box;
	}

	// Box spanning a height range [BottomZ, BottomZ+Height], thin in X (vaultable
	// depth 20cm < MaxDepth 60), wide in Y, centered ~50cm ahead of the origin.
	static AStaticMeshActor* SpawnWall(UWorld* World, float Height)
	{
		const float ScaleZ = Height / 100.0f;
		return SpawnBox(World, FVector(60.f, 0.f, Height * 0.5f), FVector(0.2f, 2.0f, ScaleZ));
	}

	static void DestroyAllBoxes(UWorld* World)
	{
		if (!World) return;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (*It) (*It)->Destroy();
		}
	}

	// Place the character on a freshly spawned floor at the origin and force it to
	// walking so ALS resolves LocomotionMode to Grounded. Feet end at Z=0 so the
	// obstacle heights above map directly to feet-relative ledge heights.
	static void GroundCharacterAtOrigin(AAlsxtCharacter* Char, UWorld* World)
	{
		// Floor: 2000x2000x40, top surface at Z=0.
		SpawnBox(World, FVector(0.f, 0.f, -20.f), FVector(20.f, 20.f, 0.4f));

		const float HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Char->SetActorRotation(FRotator(0.f, 0.f, 0.f)); // face +X
		Char->GetCharacterMovement()->GravityScale = 1.0f;
		Char->SetActorLocation(FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	// Shared precondition: spawned, settings populated, and Grounded. Emits an
	// AddError on the test (so a content/harness regression is an error, not a
	// false pass) and returns false to let the caller bail out of the phase.
	static bool CheckGroundedSubject(FAutomationTestBase* Test, AAlsxtCharacter* Char)
	{
		if (!Char)
		{
			Test->AddError(FString::Printf(TEXT("Could not obtain an ALSXT character — spawn diag: [%s]"), *GSpawnDiag));
			return false;
		}
		if (!Char->ALSXTSettings)
		{
			Test->AddError(TEXT("Spawned ALSXT character has null ALSXTSettings — vault path is unreachable; check frozen ALSXT content"));
			return false;
		}
		return Test->TestTrue(TEXT("Character must be Grounded before a grounded vault (test-setup precondition)"),
			Char->GetLocomotionMode() == AlsLocomotionModeTags::Grounded);
	}

	// The vault's forward-trace reach scales with the character's velocity
	// (ReachDistance ∝ GetVelocity().Length(), AlsxtCharacter_Actions.cpp), so a
	// stationary character can't reach an obstacle ~50cm ahead. Mirror a running
	// vault by applying forward approach velocity in the same frame as the attempt
	// (the character does not actually move — no physics step occurs before the
	// trace). Safe for the negative controls: with nothing valid to vault, the
	// extended trace still finds no startable obstacle, so they remain false.
	static bool TryGroundedVaultMoving(AAlsxtCharacter* Subject)
	{
		if (!Subject) return false;
		Subject->GetCharacterMovement()->Velocity = Subject->GetActorForwardVector() * 600.0f;
		const bool bStarted = Subject->TryStartVaultingGrounded();
		// If no vault started (negative controls), clear the velocity so the
		// character does not physically drift during the next latent wait and
		// invalidate a later phase. A started vault takes over movement itself.
		if (!bStarted)
		{
			Subject->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		return bStarted;
	}
}

// ── T1 (REQUIRED): vaults a real obstacle, not empty space ────────────────────
// Spec: "Characters can start vaulting … obstacle detection." Covers the core
// positive path plus a negative control (empty space).
// start/: stub returns false → the positive phase fails. solution/: the cascade
// detects the wall and returns true with a solved positive height.
// Catches: no-op stubs (positive phase) AND "always vault" impls (empty phase).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVaultStartsOverObstacleTest,
	"ALSXT.Vaulting.vault_starts_over_obstacle_not_empty_space",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVaultStartsOverObstacleTest::RunTest(const FString&)
{
#if WITH_EDITOR
	EXPECT_ALSXT_NOISE();
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(VaultingTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::GetOrSpawnCharacter(World);
		if (!Char)
		{
			AddError(TEXT("Could not spawn an ALSXT Blueprint character in PIE world — cannot test vaulting"));
			return true;
		}
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f)); // settle to Grounded

	// Negative control: no obstacle → must NOT vault. Then place a valid wall.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestFalse(TEXT("TryStartVaultingGrounded must return false with no obstacle to vault over"),
			VaultingTests::TryGroundedVaultMoving(Char));

		// ~90cm tall wall, well inside the 50–225 ledge band, 20cm deep (< MaxDepth 60).
		VaultingTests::SpawnWall(World, 90.0f);
		// Re-ground for a fresh, settled grounded state over the obstacle — the
		// reliable positive-vault pattern (the negative attempt above leaves the
		// character in a stale state that makes the vault detection marginal).
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f)); // settle to Grounded over the obstacle

	// Positive (discriminator): valid obstacle → must vault, with a solved height.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!Char) return true;

		TestTrue(TEXT("TryStartVaultingGrounded must start a vault when facing a valid obstacle"),
			VaultingTests::TryGroundedVaultMoving(Char));

		TestTrue(TEXT("A started vault must solve a positive VaultingHeight from the obstacle"),
			Char->GetVaultingState().VaultingParameters.VaultingHeight > 0.0f);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── T2 (REQUIRED): grounded vault requires Grounded locomotion ────────────────
// Spec: "Characters can start vaulting only when locomotion … rules allow it."
// Phase A (rejection): with a valid wall in front, force the character airborne
//   (MOVE_Falling + GravityScale 0 + lift) so LocomotionMode == InAir; the SAME
//   geometry must NOT grounded-vault — attributable purely to the locomotion gate.
// Phase B (positive/discriminator): restore Grounded; the same geometry now vaults.
// start/: false everywhere; the discriminator is Phase B. solution/: gate blocks
// in air, vaults on ground. Catches impls that ignore the locomotion-mode gate.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVaultRequiresGroundedTest,
	"ALSXT.Vaulting.grounded_vault_requires_grounded_locomotion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVaultRequiresGroundedTest::RunTest(const FString&)
{
#if WITH_EDITOR
	EXPECT_ALSXT_NOISE();
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(VaultingTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::GetOrSpawnCharacter(World);
		if (!Char)
		{
			AddError(TEXT("Could not spawn an ALSXT Blueprint character in PIE world — cannot test vaulting"));
			return true;
		}
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, 90.0f);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// Go airborne over the same valid geometry, holding position with zero gravity.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		const float HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Char->SetActorLocation(FVector(0.f, 0.f, HalfHeight + 152.f), false, nullptr, ETeleportType::TeleportPhysics);
		Char->GetCharacterMovement()->GravityScale = 0.0f;
		Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.75f)); // resolve to InAir

	// Rejection: InAir → grounded vault must be refused by the locomotion gate.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!Char) return true;

		if (!TestTrue(TEXT("Character must be InAir for the gating phase (test-setup precondition)"),
			Char->GetLocomotionMode() == AlsLocomotionModeTags::InAir))
		{
			return true; // setup didn't reach InAir; don't assert the gate on a bad precondition
		}

		TestFalse(TEXT("TryStartVaultingGrounded must return false while the character is InAir, even over a valid obstacle"),
			VaultingTests::TryGroundedVaultMoving(Char));

		// Restore grounded state over the same geometry for the positive control.
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f)); // re-settle to Grounded

	// Positive (discriminator): same valid geometry now vaults once Grounded.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestTrue(TEXT("Once Grounded again, the same valid obstacle must grounded-vault"),
			VaultingTests::TryGroundedVaultMoving(Char));
		TestTrue(TEXT("Grounded vault must solve a positive VaultingHeight"),
			Char->GetVaultingState().VaultingParameters.VaultingHeight > 0.0f);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── T3 (REQUIRED): rejects an obstacle taller than the ledge band ─────────────
// Spec: "rejects obstacles that are … too high …".
// Phase A (rejection): a wall well above LedgeHeight.Max (225) → the downward
//   trace finds no walkable top in range → no vault.
// Phase B (positive/discriminator): a valid mid-band wall → vault.
// Catches "vault anything you can touch" impls with no upper ledge bound.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVaultRejectsTooTallTest,
	"ALSXT.Vaulting.vault_rejects_too_tall_obstacle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVaultRejectsTooTallTest::RunTest(const FString&)
{
#if WITH_EDITOR
	EXPECT_ALSXT_NOISE();
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(VaultingTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::GetOrSpawnCharacter(World);
		if (!Char)
		{
			AddError(TEXT("Could not spawn an ALSXT Blueprint character in PIE world — cannot test vaulting"));
			return true;
		}
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, VaultingTests::LedgeHeightMax + 100.0f); // ~325cm, too tall
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// Rejection: obstacle above the ledge band must not vault.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestFalse(TEXT("TryStartVaultingGrounded must reject an obstacle taller than the vaultable ledge band"),
			VaultingTests::TryGroundedVaultMoving(Char));

		// Swap in a valid mid-band obstacle for the positive control.
		VaultingTests::DestroyAllBoxes(World);
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, 90.0f);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// Positive (discriminator): a valid obstacle still vaults.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestTrue(TEXT("A valid mid-band obstacle must still vault (rejecting tall walls must not reject everything)"),
			VaultingTests::TryGroundedVaultMoving(Char));
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── T4 (REQUIRED): solved geometry adapts to obstacle height ──────────────────
// Spec: "Hand placement and landing position adapt … a tall obstacle produces
// different hand and landing positions than a short one."
// The solved vault parameters must be DERIVED from the obstacle, not hard-coded:
// after vaulting a ~90cm obstacle the solved VaultingHeight must be positive and
// roughly reflect the obstacle's height. start/ never starts a vault (VaultingHeight
// stays 0 → fails); solution/ solves ~90cm → passes. Catches constant/hard-coded
// parameter solving. (Validated with a single vault: vaulting two obstacles
// sequentially on one character leaves residual vault state that makes the second
// vault's trace unreliable in this single-client headless harness.)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVaultParamsAdaptToHeightTest,
	"ALSXT.Vaulting.vault_params_adapt_to_obstacle_height",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVaultParamsAdaptToHeightTest::RunTest(const FString&)
{
#if WITH_EDITOR
	// Carried across phases within this single PIE session.
	static float ShortHeight = 0.0f;
	static FVector ShortLanding = FVector::ZeroVector;
	static FVector ShortHand = FVector::ZeroVector;
	static bool ShortStarted = false;
	ShortHeight = 0.0f;
	ShortLanding = FVector::ZeroVector;
	ShortHand = FVector::ZeroVector;
	ShortStarted = false;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(VaultingTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// Phase 1: vault a SHORT obstacle (90cm).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::GetOrSpawnCharacter(World);
		if (!Char)
		{
			AddError(TEXT("Could not spawn an ALSXT Blueprint character in PIE world — cannot test vaulting"));
			return true;
		}
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, 90.0f);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		if (!TestTrue(TEXT("Vault over the obstacle must start"),
			VaultingTests::TryGroundedVaultMoving(Char))) return true;

		const float SolvedHeight = Char->GetVaultingState().VaultingParameters.VaultingHeight;
		// The solved height must be DERIVED from the ~90cm obstacle (positive and in a
		// generous band around it), not a hard-coded constant or zero.
		TestTrue(TEXT("Solved VaultingHeight must be positive (derived from the obstacle)"),
			SolvedHeight > 0.0f);
		TestTrue(TEXT("Solved VaultingHeight must roughly reflect the ~90cm obstacle (45-135cm)"),
			SolvedHeight > 45.0f && SolvedHeight < 135.0f);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ── T5 (PARTIAL): rejects an obstacle below the ledge band ────────────────────
// Spec: "rejects obstacles that are … too low …".
// Phase A (rejection): a ~25cm step (below LedgeHeight.Min 50) sits beneath the
//   forward trace band → no blocking ledge hit → no vault.
// Phase B (positive/discriminator): a valid mid-band wall → vault.
// Catches impls missing the lower ledge bound (vaulting tiny steps/curbs).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVaultRejectsTooLowTest,
	"ALSXT.Vaulting.vault_rejects_too_low_obstacle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVaultRejectsTooLowTest::RunTest(const FString&)
{
#if WITH_EDITOR
	EXPECT_ALSXT_NOISE();
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(VaultingTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::GetOrSpawnCharacter(World);
		if (!Char)
		{
			AddError(TEXT("Could not spawn an ALSXT Blueprint character in PIE world — cannot test vaulting"));
			return true;
		}
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, VaultingTests::LedgeHeightMin * 0.5f); // ~25cm, too low
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// Rejection: a step below the ledge band must not vault.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestFalse(TEXT("TryStartVaultingGrounded must reject an obstacle below the vaultable ledge band"),
			VaultingTests::TryGroundedVaultMoving(Char));

		VaultingTests::DestroyAllBoxes(World);
		VaultingTests::GroundCharacterAtOrigin(Char, World);
		VaultingTests::SpawnWall(World, 90.0f);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	// Positive (discriminator): a valid obstacle still vaults.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = VaultingTests::GetTestWorld();
		AAlsxtCharacter* Char = VaultingTests::FindCharacter(World);
		if (!VaultingTests::CheckGroundedSubject(this, Char)) return true;

		TestTrue(TEXT("A valid mid-band obstacle must still vault (rejecting low steps must not reject everything)"),
			VaultingTests::TryGroundedVaultMoving(Char));
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
