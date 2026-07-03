#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "SpudData.h"
#include "SpudState.h"
#include "SpudRuntimeActorIdentityTestTypes.h"

namespace SpudRuntimeActorIdentityTest
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

	static FSpudSaveData::TLevelDataPtr FindLevelData(USpudState* State, const FString& LevelName)
	{
		if (!State)
		{
			return nullptr;
		}

		if (FSpudSaveData::TLevelDataPtr* Found = State->SaveData.LevelDataMap.Find(LevelName))
		{
			return *Found;
		}

		return nullptr;
	}

	static TArray<ASpudRuntimeIdentityTestActor*> FindIdentityActors(UWorld* World)
	{
		TArray<ASpudRuntimeIdentityTestActor*> Actors;
		if (!World)
		{
			return Actors;
		}

		for (TActorIterator<ASpudRuntimeIdentityTestActor> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Actors.Add(*It);
			}
		}

		return Actors;
	}

	static ASpudRuntimeIdentityTestActor* FindActorByGuid(const TArray<ASpudRuntimeIdentityTestActor*>& Actors, const FGuid& Guid)
	{
		for (ASpudRuntimeIdentityTestActor* Actor : Actors)
		{
			if (IsValid(Actor) && Actor->SpudGuid == Guid)
			{
				return Actor;
			}
		}

		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorStoresByGuidTest,
	"SPUD.PersistentRuntimeRestore.stores_runtime_actor_by_stable_identity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorStoresByGuidTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	ASpudRuntimeIdentityTestActor* OriginalActor = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	ASpudRuntimeIdentityTestActor* LaterActorWithSameIdentity = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	if (!TestNotNull(TEXT("Original runtime actor should spawn"), OriginalActor) ||
		!TestNotNull(TEXT("Second runtime actor should spawn"), LaterActorWithSameIdentity))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	const FGuid StableGuid(0x10203040, 0x50607080, 0x90A0B0C0, 0xD0E0F001);
	OriginalActor->SpudGuid = StableGuid;
	OriginalActor->SavedValue = 73;
	LaterActorWithSameIdentity->SpudGuid = StableGuid;
	LaterActorWithSameIdentity->SavedValue = 74;

	const FString LevelName = USpudState::GetLevelNameForActor(OriginalActor);
	State->StoreActor(OriginalActor);
	State->StoreActor(LaterActorWithSameIdentity);
	const FSpudSaveData::TLevelDataPtr LevelData = SpudRuntimeActorIdentityTest::FindLevelData(State, LevelName);
	if (!TestNotNull(TEXT("StoreActor should create level data"), LevelData.Get()))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	TestEqual(TEXT("Saving another runtime actor with the same persistent identity should update one spawned record"),
		LevelData->SpawnedActors.Contents.Num(), 1);
	for (const TPair<FString, FSpudSpawnedActorData>& SpawnedActor : LevelData->SpawnedActors.Contents)
	{
		TestEqual(TEXT("Spawned actor record should retain the persistent identity"), SpawnedActor.Value.Guid, StableGuid);
	}

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorRespawnsAndFixesReferencesTest,
	"SPUD.PersistentRuntimeRestore.respawns_runtime_actors_and_fixes_runtime_references",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorRespawnsAndFixesReferencesTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	ASpudRuntimeIdentityTestActor* Source = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	ASpudRuntimeIdentityTestActor* Target = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	if (!TestNotNull(TEXT("Source actor should spawn"), Source) ||
		!TestNotNull(TEXT("Target actor should spawn"), Target))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	const FGuid SourceGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444);
	const FGuid TargetGuid(0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD);
	Source->SpudGuid = SourceGuid;
	Source->SavedValue = 10;
	Source->LinkedActor = Target;
	Target->SpudGuid = TargetGuid;
	Target->SavedValue = 20;

	const FString LevelName = USpudState::GetLevelNameForActor(Source);
	State->StoreActor(Source);
	State->StoreActor(Target);

	Source->Destroy();
	Target->Destroy();
	World->CleanupActors();

	State->RestoreLevel(World->PersistentLevel);

	TArray<ASpudRuntimeIdentityTestActor*> RestoredActors = SpudRuntimeActorIdentityTest::FindIdentityActors(World);
	TestEqual(TEXT("Both runtime actors should be respawned"), RestoredActors.Num(), 2);

	ASpudRuntimeIdentityTestActor* RestoredSource = nullptr;
	ASpudRuntimeIdentityTestActor* RestoredTarget = nullptr;
	for (ASpudRuntimeIdentityTestActor* Actor : RestoredActors)
	{
		if (Actor->SpudGuid == SourceGuid)
		{
			RestoredSource = Actor;
		}
		else if (Actor->SpudGuid == TargetGuid)
		{
			RestoredTarget = Actor;
		}
	}

	TestNotNull(TEXT("Source should be restored with its saved guid"), RestoredSource);
	TestNotNull(TEXT("Target should be restored with its saved guid"), RestoredTarget);
	if (RestoredSource && RestoredTarget)
	{
		TestEqual(TEXT("Source SaveGame value should restore"), RestoredSource->SavedValue, 10);
		TestEqual(TEXT("Target SaveGame value should restore"), RestoredTarget->SavedValue, 20);
		TestEqual(TEXT("Runtime-to-runtime reference should fix up after respawn"), RestoredSource->LinkedActor.Get(), RestoredTarget);
	}

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorArchiveRoundTripTest,
	"SPUD.PersistentRuntimeRestore.archive_roundtrip_preserves_runtime_identity_and_references",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorArchiveRoundTripTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* SavedState = NewObject<USpudState>();
	ASpudRuntimeIdentityTestActor* Source = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	ASpudRuntimeIdentityTestActor* Target = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	if (!TestNotNull(TEXT("Source actor should spawn"), Source) ||
		!TestNotNull(TEXT("Target actor should spawn"), Target))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	const FGuid SourceGuid(0x73000001, 0x73000002, 0x73000003, 0x73000004);
	const FGuid TargetGuid(0x73000005, 0x73000006, 0x73000007, 0x73000008);
	Source->SpudGuid = SourceGuid;
	Source->SavedValue = 731;
	Source->LinkedActor = Target;
	Source->WeakLinkedActor = Target;
	Target->SpudGuid = TargetGuid;
	Target->SavedValue = 732;

	SavedState->StoreActor(Source);
	SavedState->StoreActor(Target);

	TArray<uint8> SaveBytes;
	FMemoryWriter Writer(SaveBytes, true);
	SavedState->SaveToArchive(Writer);
	TestTrue(TEXT("Serialized SPUD state should contain runtime actor records"), SaveBytes.Num() > 0);

	USpudState* LoadedState = NewObject<USpudState>();
	FMemoryReader Reader(SaveBytes, true);
	LoadedState->LoadFromArchive(Reader, true);

	Source->Destroy();
	Target->Destroy();
	World->CleanupActors();

	LoadedState->RestoreLevel(World->PersistentLevel);

	TArray<ASpudRuntimeIdentityTestActor*> RestoredActors = SpudRuntimeActorIdentityTest::FindIdentityActors(World);
	TestEqual(TEXT("Both runtime actors should respawn after archive load"), RestoredActors.Num(), 2);

	ASpudRuntimeIdentityTestActor* RestoredSource = SpudRuntimeActorIdentityTest::FindActorByGuid(RestoredActors, SourceGuid);
	ASpudRuntimeIdentityTestActor* RestoredTarget = SpudRuntimeActorIdentityTest::FindActorByGuid(RestoredActors, TargetGuid);
	TestNotNull(TEXT("Source should retain stable guid through archive load"), RestoredSource);
	TestNotNull(TEXT("Target should retain stable guid through archive load"), RestoredTarget);
	if (RestoredSource && RestoredTarget)
	{
		TestEqual(TEXT("Source SaveGame value should survive archive load"), RestoredSource->SavedValue, 731);
		TestEqual(TEXT("Target SaveGame value should survive archive load"), RestoredTarget->SavedValue, 732);
		TestEqual(TEXT("Strong runtime actor reference should fix up after archive load"), RestoredSource->LinkedActor.Get(), RestoredTarget);
		TestEqual(TEXT("Weak runtime actor reference should fix up after archive load"), RestoredSource->WeakLinkedActor.Get(), RestoredTarget);
	}

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorRoamingRestoreTest,
	"SPUD.PersistentRuntimeRestore.restores_roaming_runtime_actors_and_references",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorRoamingRestoreTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	ASpudRuntimeIdentityRoamingActor* Source = World->SpawnActor<ASpudRuntimeIdentityRoamingActor>();
	ASpudRuntimeIdentityRoamingActor* Target = World->SpawnActor<ASpudRuntimeIdentityRoamingActor>();
	if (!TestNotNull(TEXT("Roaming source actor should spawn"), Source) ||
		!TestNotNull(TEXT("Roaming target actor should spawn"), Target))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	const FGuid SourceGuid(0x73100001, 0x73100002, 0x73100003, 0x73100004);
	const FGuid TargetGuid(0x73100005, 0x73100006, 0x73100007, 0x73100008);
	Source->SpudGuid = SourceGuid;
	Source->SavedValue = 901;
	Source->LinkedActor = Target;
	Target->SpudGuid = TargetGuid;
	Target->SavedValue = 902;

	State->StoreActor(Source);
	State->StoreActor(Target);

	Source->Destroy();
	Target->Destroy();
	World->CleanupActors();

	State->RestoreLevel(World->PersistentLevel);

	TArray<ASpudRuntimeIdentityTestActor*> RestoredActors = SpudRuntimeActorIdentityTest::FindIdentityActors(World);
	TestEqual(TEXT("Both roaming runtime actors should be respawned"), RestoredActors.Num(), 2);

	ASpudRuntimeIdentityTestActor* RestoredSource = SpudRuntimeActorIdentityTest::FindActorByGuid(RestoredActors, SourceGuid);
	ASpudRuntimeIdentityTestActor* RestoredTarget = SpudRuntimeActorIdentityTest::FindActorByGuid(RestoredActors, TargetGuid);
	TestNotNull(TEXT("Roaming source should restore with stable guid"), RestoredSource);
	TestNotNull(TEXT("Roaming target should restore with stable guid"), RestoredTarget);
	if (RestoredSource && RestoredTarget)
	{
		TestTrue(TEXT("Source should restore as a roaming actor"), RestoredSource->IsA<ASpudRuntimeIdentityRoamingActor>());
		TestTrue(TEXT("Target should restore as a roaming actor"), RestoredTarget->IsA<ASpudRuntimeIdentityRoamingActor>());
		TestEqual(TEXT("Roaming source SaveGame value should restore"), RestoredSource->SavedValue, 901);
		TestEqual(TEXT("Roaming target SaveGame value should restore"), RestoredTarget->SavedValue, 902);
		TestEqual(TEXT("Roaming runtime reference should fix up after respawn"), RestoredSource->LinkedActor.Get(), RestoredTarget);
	}

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorSkipsDuplicateExistingInstanceTest,
	"SPUD.PersistentRuntimeRestore.does_not_duplicate_existing_runtime_actor_with_same_guid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorSkipsDuplicateExistingInstanceTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	ASpudRuntimeIdentityTestActor* Actor = World->SpawnActor<ASpudRuntimeIdentityTestActor>();
	if (!TestNotNull(TEXT("Runtime actor should spawn"), Actor))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	const FGuid StableGuid(0xABCDEF01, 0x23456789, 0xABCDEF02, 0x3456789A);
	Actor->SpudGuid = StableGuid;
	Actor->SavedValue = 101;
	State->StoreActor(Actor);

	Actor->SavedValue = -1;
	State->RestoreLevel(World->PersistentLevel);

	TArray<ASpudRuntimeIdentityTestActor*> Actors = SpudRuntimeActorIdentityTest::FindIdentityActors(World);
	TestEqual(TEXT("Existing runtime actor with same guid should be restored, not duplicated"), Actors.Num(), 1);
	if (Actors.Num() == 1)
	{
		TestEqual(TEXT("Existing actor should receive saved state during restore"), Actors[0]->SavedValue, 101);
		TestEqual(TEXT("Existing actor should keep stable guid"), Actors[0]->SpudGuid, StableGuid);
	}

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudRuntimeActorRespectsNeverRespawnStableNameTest,
	"SPUD.PersistentRuntimeRestore.respects_never_respawn_and_override_name_for_auto_created_actors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudRuntimeActorRespectsNeverRespawnStableNameTest::RunTest(const FString&)
{
	UWorld* World = SpudRuntimeActorIdentityTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world must be available"), World))
	{
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	ASpudRuntimeIdentityNamedActor* Actor = World->SpawnActor<ASpudRuntimeIdentityNamedActor>();
	if (!TestNotNull(TEXT("Auto-created style actor should spawn"), Actor))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	Actor->SpudGuid = FGuid(0x11110000, 0x22220000, 0x33330000, 0x44440000);
	Actor->SavedValue = 500;
	const FString LevelName = USpudState::GetLevelNameForActor(Actor);
	State->StoreActor(Actor);

	const FSpudSaveData::TLevelDataPtr LevelData = SpudRuntimeActorIdentityTest::FindLevelData(State, LevelName);
	if (!TestNotNull(TEXT("StoreActor should create level data"), LevelData.Get()))
	{
		SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
		return true;
	}

	TestTrue(TEXT("NeverRespawn runtime actor should be stored as a stable named level actor"),
		LevelData->LevelActors.Contents.Contains(TEXT("StableAutoCreatedRuntimeActor")));
	TestEqual(TEXT("NeverRespawn runtime actor should not create a spawned actor record"),
		LevelData->SpawnedActors.Contents.Num(), 0);

	SpudRuntimeActorIdentityTest::DestroyGameWorld(World);
	return true;
}
