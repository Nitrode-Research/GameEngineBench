#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"

#define private public
#define protected public
#include "SpudRoamingActorSubsystem.h"
#undef protected
#undef private

namespace SpudRoamingActorTest
{
	static UWorld* CreateCleanGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		if (!World)
		{
			return nullptr;
		}

		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	static void DestroyGameWorld(UWorld* World)
	{
		if (!World || !GEngine)
		{
			return;
		}

		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	static void AddCachedCell(
		USpudRoamingActorSubsystem* Tracker,
		const FString& LevelName,
		const FBox& Bounds,
		EWorldPartitionRuntimeCellState State)
	{
		USpudRoamingActorSubsystem::FCachedCellData& Data = Tracker->CellCache.AddDefaulted_GetRef();
		Data.Cell = nullptr;
		Data.Bounds = Bounds;
		Data.LevelName = LevelName;
		Data.State = State;
		Data.bPendingUnload = false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRoamingActorFindsMostSpecificCellTest,
	"SPUD.RoamingActors.finds_most_specific_cell_and_reports_activation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRoamingActorFindsMostSpecificCellTest::RunTest(const FString&)
{
	USpudRoamingActorSubsystem* Tracker = NewObject<USpudRoamingActorSubsystem>();
	TestNotNull(TEXT("Tracker should be constructible"), Tracker);

	SpudRoamingActorTest::AddCachedCell(
		Tracker,
		TEXT("LandscapeCell"),
		FBox(FVector(-1000.0, -1000.0, -100.0), FVector(1000.0, 1000.0, 100.0)),
		EWorldPartitionRuntimeCellState::Activated);
	SpudRoamingActorTest::AddCachedCell(
		Tracker,
		TEXT("InteriorCell"),
		FBox(FVector(-100.0, -100.0, -100.0), FVector(100.0, 100.0, 100.0)),
		EWorldPartitionRuntimeCellState::Loaded);

	FString CellName;
	bool bIsActivated = true;
	const bool bFound = Tracker->FindCellForLocation(FVector(10.0, 10.0, 0.0), CellName, bIsActivated);

	TestTrue(TEXT("Location inside cached cells should resolve to a cell"), bFound);
	TestEqual(TEXT("Smallest containing cell should win"), CellName, FString(TEXT("InteriorCell")));
	TestFalse(TEXT("Loaded but inactive cells should be reported as inactive"), bIsActivated);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRoamingActorTracksPendingUnloadTest,
	"SPUD.RoamingActors.tracks_pending_unload_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRoamingActorTracksPendingUnloadTest::RunTest(const FString&)
{
	USpudRoamingActorSubsystem* Tracker = NewObject<USpudRoamingActorSubsystem>();
	TestNotNull(TEXT("Tracker should be constructible"), Tracker);

	SpudRoamingActorTest::AddCachedCell(
		Tracker,
		TEXT("BoundaryCell"),
		FBox(FVector::ZeroVector, FVector(100.0, 100.0, 100.0)),
		EWorldPartitionRuntimeCellState::Activated);

	Tracker->OnPreUnloadStreamingLevel(FName(TEXT("BoundaryCell")));
	TestTrue(TEXT("Pre-unload should mark the cached cell as pending unload"), Tracker->CellCache[0].bPendingUnload);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRoamingActorRegistersActorsTest,
	"SPUD.RoamingActors.registers_and_unregisters_unique_actors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRoamingActorRegistersActorsTest::RunTest(const FString&)
{
	UWorld* World = SpudRoamingActorTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudRoamingActorSubsystem* Tracker = NewObject<USpudRoamingActorSubsystem>(World);
	TestNotNull(TEXT("Tracker should be constructible"), Tracker);

	AActor* Actor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Actor should spawn"), Actor))
	{
		SpudRoamingActorTest::DestroyGameWorld(World);
		return true;
	}

	Tracker->RegisterActor(Actor);
	Tracker->RegisterActor(Actor);
	TestEqual(TEXT("Duplicate registration should not add duplicate tracked actor entries"), Tracker->TrackedActors.Num(), 1);
	if (Tracker->TrackedActors.Num() == 1)
	{
		TestEqual(TEXT("Tracked entry should point at the actor"), Tracker->TrackedActors[0].Actor.Get(), Actor);
	}

	Tracker->UnregisterActor(Actor);
	TestEqual(TEXT("Unregister should remove the tracked actor"), Tracker->TrackedActors.Num(), 0);

	SpudRoamingActorTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRoamingActorClampsBoundaryRestoreTest,
	"SPUD.RoamingActors.clamps_actor_inside_target_cell",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRoamingActorClampsBoundaryRestoreTest::RunTest(const FString&)
{
	UWorld* World = SpudRoamingActorTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudRoamingActorSubsystem* Tracker = NewObject<USpudRoamingActorSubsystem>(World);
	TestNotNull(TEXT("Tracker should be constructible"), Tracker);
	SpudRoamingActorTest::AddCachedCell(
		Tracker,
		TEXT("TargetCell"),
		FBox(FVector::ZeroVector, FVector(100.0, 100.0, 100.0)),
		EWorldPartitionRuntimeCellState::Activated);

	AActor* Actor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Actor should spawn"), Actor))
	{
		SpudRoamingActorTest::DestroyGameWorld(World);
		return true;
	}

	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("SPUDRoamingTestRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();

	Actor->SetActorLocation(FVector(150.0, -25.0, 200.0));
	Tracker->ClampActorToCell(Actor, TEXT("TargetCell"));

	TestTrue(TEXT("Actor should be clamped inside the target cell with an inset"),
		Actor->GetActorLocation().Equals(FVector(90.0, 10.0, 90.0)));

	SpudRoamingActorTest::DestroyGameWorld(World);
	return true;
}
