#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"        // GEngine
#include "Engine/World.h"

#define protected public
#define private public
#include "Actors/RicochetBullet.h"
#include "Actors/RicochetBulletPoolManager.h"
#include "Components/RicochetReceiver.h"
#undef private
#undef protected

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral tests for ue_task_0070 (Ricochet Bullets).
//
// EDITABLE FILES UNDER TEST: RicochetBullet.{cpp,h}, RicochetBulletPoolManager.{cpp,h},
// RicochetReceiver.{cpp,h}. The gutted start/ replaces real method bodies with no-ops/stubs;
// solution/ restores them. These are plain AActor/UActorComponent gameplay classes (no EOS), so
// they can be exercised in a bare C++ game world: a spawned actor in a standalone world has
// authority (HasAuthority() == true), which is what the restored server paths require.
//
// Two discriminators, each on a distinct gutted method:
//  1) ARicochetBullet::ActivateBullet — start calls DeactivateBullet() (leaves bIsActive=false);
//     solution marks the bullet active (bIsActive=true) and starts its movement.
//  2) ARicochetBulletPoolManager::GetAvailableBullet — start is `return nullptr;`; solution scans
//     BulletPool and returns the first inactive bullet, which is how the pool recycles rounds.
//
// The previous suite (CDO defaults + member-function-pointer existence) was dropped: those defaults
// aren't gutted and method pointers are never null, so all of it passed on start/ too.
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
	// Bare standalone game world — no PIE, no online subsystem. Spawned actors get authority.
	UWorld* MakeGameWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (World == nullptr)
		{
			return nullptr;
		}
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());   // registers components / physics; does NOT BeginPlay
		return World;
	}

	void TearDownGameWorld(UWorld* World)
	{
		if (World != nullptr)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld*/ false);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRicochetActivateBulletTest,
	"Ricochet.Bullets.activate_bullet_marks_bullet_active",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRicochetActivateBulletTest::RunTest(const FString&)
{
	UWorld* World = MakeGameWorld();
	if (!TestNotNull(TEXT("bare game world must be creatable"), World))
	{
		return true;
	}

	ARicochetBullet* Bullet = World->SpawnActor<ARicochetBullet>();
	if (!TestNotNull(TEXT("bullet must spawn"), Bullet))
	{
		TearDownGameWorld(World);
		return true;
	}

	// Precondition: a fresh bullet is inactive.
	TestFalse(TEXT("precondition: spawned bullet starts inactive"), Bullet->bIsActive);

	Bullet->ActivateBullet(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);

	// Solution activates the bullet (bIsActive=true). The gutted start/ instead calls
	// DeactivateBullet(), leaving it inactive.
	TestTrue(
		TEXT("ActivateBullet must mark the bullet active; start/ calls DeactivateBullet and leaves it inactive"),
		Bullet->bIsActive);

	// Activation also seeds the replicated launch velocity along the spawn direction at this
	// project's configured muzzle launch speed. SpawnRotation is ZeroRotator -> forward is +X.
	TestTrue(
		TEXT("ActivateBullet must seed InitialVelocity along the spawn direction at the project's launch speed"),
		FMath::IsNearlyEqual(Bullet->InitialVelocity.X, 75000.0, 1.0));

	TearDownGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRicochetGetAvailableBulletTest,
	"Ricochet.Bullets.pool_returns_inactive_bullet_for_reuse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRicochetGetAvailableBulletTest::RunTest(const FString&)
{
	UWorld* World = MakeGameWorld();
	if (!TestNotNull(TEXT("bare game world must be creatable"), World))
	{
		return true;
	}

	ARicochetBulletPoolManager* Pool = World->SpawnActor<ARicochetBulletPoolManager>();
	ARicochetBullet* Bullet = World->SpawnActor<ARicochetBullet>();
	if (!TestNotNull(TEXT("pool manager must spawn"), Pool) ||
		!TestNotNull(TEXT("bullet must spawn"), Bullet))
	{
		TearDownGameWorld(World);
		return true;
	}

	// One inactive bullet in the pool — the recyclable candidate GetAvailableBullet should return.
	Bullet->bIsActive = false;
	Pool->BulletPool.Add(Bullet);

	ARicochetBullet* Available = Pool->GetAvailableBullet();

	// Solution scans BulletPool and hands back the inactive bullet; the gutted start/ returns null,
	// so the pool can never recycle a round.
	TestEqual(
		TEXT("GetAvailableBullet must return the inactive pooled bullet; start/ returns nullptr"),
		Available, Bullet);

	TearDownGameWorld(World);
	return true;
}
