#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#define private public
#define protected public
#include "SpudState.h"
#include "SpudSubsystem.h"
#undef protected
#undef private

#include "SpudPropertyUtil.h"

namespace SpudDestroyedActorPersistenceTest
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

	static AActor* SpawnNamedActor(UWorld* World, const TCHAR* Name)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(Name);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorRecordsArchiveTest,
	"SPUD.PersistentRuntimeRestore.records_stable_actor_names_and_roundtrips_archive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorRecordsArchiveTest::RunTest(const FString&)
{
	UWorld* World = SpudDestroyedActorPersistenceTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world should be available"), World))
	{
		return true;
	}

	AActor* Actor = SpudDestroyedActorPersistenceTest::SpawnNamedActor(World, TEXT("SPUD_Destroyed_Record_A"));
	if (!TestNotNull(TEXT("Actor should spawn"), Actor))
	{
		SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
		return true;
	}

	const FString LevelName = USpudState::GetLevelNameForActor(Actor);
	USpudState* State = NewObject<USpudState>();
	State->StoreLevelActorDestroyed(Actor);

	const FSpudSaveData::TLevelDataPtr LevelData = State->GetLevelData(LevelName, false);
	TestTrue(TEXT("Destroyed actor store should create level data"), LevelData.IsValid());
	if (LevelData.IsValid())
	{
		TestEqual(TEXT("Exactly one destroyed actor record should be written"),
			LevelData->DestroyedActors.Values.Num(), 1);
		if (LevelData->DestroyedActors.Values.Num() == 1)
		{
			TestEqual(TEXT("Destroyed actor record should use stable level actor identity"),
				LevelData->DestroyedActors.Values[0]->Name,
				SpudPropertyUtil::GetLevelActorName(Actor));
		}
	}

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	State->SaveToArchive(Writer);

	USpudState* Reloaded = NewObject<USpudState>();
	FMemoryReader Reader(Bytes, true);
	Reloaded->LoadFromArchive(Reader, true);
	const FSpudSaveData::TLevelDataPtr ReloadedLevelData = Reloaded->GetLevelData(LevelName, false);
	TestTrue(TEXT("Reloaded state should include destroyed actor level data"), ReloadedLevelData.IsValid());
	if (ReloadedLevelData.IsValid())
	{
		TestEqual(TEXT("Destroyed actor record should survive archive reload"),
			ReloadedLevelData->DestroyedActors.Values.Num(), 1);
		if (ReloadedLevelData->DestroyedActors.Values.Num() == 1)
		{
			TestEqual(TEXT("Reloaded destroyed actor identity should be preserved"),
				ReloadedLevelData->DestroyedActors.Values[0]->Name,
				SpudPropertyUtil::GetLevelActorName(Actor));
		}
	}

	SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorRestoreDeletesTombstonesTest,
	"SPUD.PersistentRuntimeRestore.restore_applies_destroyed_records_after_actor_restore",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorRestoreDeletesTombstonesTest::RunTest(const FString&)
{
	UWorld* World = SpudDestroyedActorPersistenceTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world should be available"), World))
	{
		return true;
	}

	AActor* Actor = SpudDestroyedActorPersistenceTest::SpawnNamedActor(World, TEXT("SPUD_Destroyed_Restore_A"));
	if (!TestNotNull(TEXT("Actor should spawn"), Actor))
	{
		SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
		return true;
	}

	USpudState* State = NewObject<USpudState>();
	State->StoreLevelActorDestroyed(Actor);
	State->RestoreLevel(World->PersistentLevel);

	TestTrue(TEXT("RestoreLevel should destroy actors listed in destroyed actor state"),
		!IsValid(Actor) || Actor->IsActorBeingDestroyed());

	SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
	return true;
}
