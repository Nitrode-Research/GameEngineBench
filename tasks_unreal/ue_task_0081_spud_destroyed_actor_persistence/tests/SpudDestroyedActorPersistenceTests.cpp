#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

	static FString LoadProjectSource(FAutomationTestBase* Test, const TCHAR* RelativePath)
	{
		const FString AbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		FString Source;
		if (!Test->TestTrue(
				FString::Printf(TEXT("Source file should be readable at %s"), *AbsPath),
				FFileHelper::LoadFileToString(Source, *AbsPath)))
		{
			return FString();
		}
		return Source;
	}

	static FString StripComments(const FString& Source)
	{
		FString Out;
		Out.Reserve(Source.Len());
		bool bLine = false;
		bool bBlock = false;
		bool bString = false;
		for (int32 Index = 0; Index < Source.Len(); ++Index)
		{
			const TCHAR C = Source[Index];
			const TCHAR N = Index + 1 < Source.Len() ? Source[Index + 1] : TCHAR('\0');
			if (bLine)
			{
				if (C == TCHAR('\n'))
				{
					bLine = false;
					Out.AppendChar(C);
				}
				continue;
			}
			if (bBlock)
			{
				if (C == TCHAR('*') && N == TCHAR('/'))
				{
					bBlock = false;
					++Index;
				}
				continue;
			}
			if (bString)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++Index;
				}
				else if (C == TCHAR('"'))
				{
					bString = false;
				}
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('/'))
			{
				bLine = true;
				++Index;
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('*'))
			{
				bBlock = true;
				++Index;
				continue;
			}
			if (C == TCHAR('"'))
			{
				bString = true;
			}
			Out.AppendChar(C);
		}
		return Out;
	}

	static FString ExtractFunctionBody(FAutomationTestBase* Test, const FString& Source, const TCHAR* Signature)
	{
		const int32 SignatureIndex = Source.Find(Signature);
		if (!Test->TestTrue(FString::Printf(TEXT("Should find function %s"), Signature), SignatureIndex != INDEX_NONE))
		{
			return FString();
		}

		const int32 OpenBrace = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (!Test->TestTrue(FString::Printf(TEXT("Should find body for %s"), Signature), OpenBrace != INDEX_NONE))
		{
			return FString();
		}

		int32 Depth = 0;
		for (int32 Index = OpenBrace; Index < Source.Len(); ++Index)
		{
			if (Source[Index] == TCHAR('{'))
			{
				++Depth;
			}
			else if (Source[Index] == TCHAR('}'))
			{
				--Depth;
				if (Depth == 0)
				{
					return Source.Mid(OpenBrace, Index - OpenBrace + 1);
				}
			}
		}

		Test->AddError(FString::Printf(TEXT("Unterminated function body for %s"), Signature));
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorRecordsArchiveTest,
	"SPUD.DestroyedActorPersistence.records_stable_actor_names_and_roundtrips_archive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorRecordsArchiveTest::RunTest(const FString&)
{
	UWorld* World = SpudDestroyedActorPersistenceTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world should be available"), World))
	{
		return true;
	}

	AActor* Actor = SpudDestroyedActorPersistenceTest::SpawnNamedActor(World, TEXT("SPUD_Destroyed_Record_A"));
	AActor* RuntimeActor = SpudDestroyedActorPersistenceTest::SpawnNamedActor(World, TEXT("SPUD_Destroyed_Runtime_A"));
	if (!TestNotNull(TEXT("Actor should spawn"), Actor))
	{
		SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
		return true;
	}
	TestNotNull(TEXT("Runtime actor should spawn"), RuntimeActor);
	Actor->SetFlags(RF_WasLoaded);

	const FString LevelName = USpudState::GetLevelNameForActor(Actor);
	USpudState* State = NewObject<USpudState>();
	State->StoreLevelActorDestroyed(Actor);
	State->StoreLevelActorDestroyed(Actor);
	if (RuntimeActor)
	{
		State->StoreLevelActorDestroyed(RuntimeActor);
	}

	const FSpudSaveData::TLevelDataPtr LevelData = State->GetLevelData(LevelName, false);
	TestTrue(TEXT("Destroyed actor store should create level data"), LevelData.IsValid());
	if (LevelData.IsValid())
	{
		TestEqual(TEXT("Repeated destroyed actor stores should remain idempotent"),
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
	"SPUD.DestroyedActorPersistence.restore_applies_destroyed_records_after_actor_restore",
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
	Actor->SetFlags(RF_WasLoaded);

	Actor->SetActorLocation(FVector(45.0, 12.0, 8.0));

	USpudState* State = NewObject<USpudState>();
	State->StoreActor(Actor);
	State->StoreLevelActorDestroyed(Actor);
	State->RestoreLevel(World->PersistentLevel);

	TestTrue(TEXT("RestoreLevel should apply destroyed actor records after normal actor restore"),
		!IsValid(Actor) || Actor->IsActorBeingDestroyed());

	SpudDestroyedActorPersistenceTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorStateMechanicsSourceTest,
	"SPUD.DestroyedActorPersistence.state_filters_dedupes_and_restores_tombstones",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorStateMechanicsSourceTest::RunTest(const FString&)
{
	const FString Source = SpudDestroyedActorPersistenceTest::StripComments(
		SpudDestroyedActorPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudState.cpp")));
	const FString StoreDestroyedBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudState::StoreLevelActorDestroyed(AActor* Actor, FSpudSaveData::TLevelDataPtr LevelData)"));
	const FString RestoreLevelBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudState::RestoreLevel(ULevel* Level)"));

	TestTrue(TEXT("Destroyed level actor storage should reject invalid and runtime actors"),
		StoreDestroyedBody.Contains(TEXT("!IsValid(Actor)")) &&
		StoreDestroyedBody.Contains(TEXT("SpudPropertyUtil::IsRuntimeActor(Actor)")));
	TestTrue(TEXT("Destroyed level actor storage should use stable level actor names"),
		StoreDestroyedBody.Contains(TEXT("SpudPropertyUtil::GetLevelActorName(Actor)")) &&
		StoreDestroyedBody.Contains(TEXT("DestroyedActors.Add(ActorName)")));
	TestTrue(TEXT("Destroyed level actor storage should dedupe repeated actor destruction"),
		StoreDestroyedBody.Contains(TEXT("ContainsByPredicate")) &&
		StoreDestroyedBody.Contains(TEXT("Existing->Name == ActorName")) &&
		StoreDestroyedBody.Contains(TEXT("!bAlreadyRecorded")));

	const int32 RestoreActorIndex = RestoreLevelBody.Find(TEXT("RestoreActor(Actor, LevelData"));
	const int32 DestroyActorIndex = RestoreLevelBody.Find(TEXT("DestroyActor(*DestroyedActor, Level)"));
	TestTrue(TEXT("RestoreLevel should apply destroyed tombstones after saved actor state restore"),
		RestoreActorIndex != INDEX_NONE &&
		DestroyActorIndex != INDEX_NONE &&
		DestroyActorIndex > RestoreActorIndex);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorSubscriptionFilterSourceTest,
	"SPUD.DestroyedActorPersistence.subscribes_only_level_persistent_actor_destroy_events",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorSubscriptionFilterSourceTest::RunTest(const FString&)
{
	const FString Source = SpudDestroyedActorPersistenceTest::StripComments(
		SpudDestroyedActorPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));
	const FString SubscribeAllBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::SubscribeAllLevelObjectEvents()"));
	const FString UnsubscribeAllBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::UnsubscribeAllLevelObjectEvents()"));
	const FString SubscribeBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::SubscribeLevelObjectEvents(ULevel* Level)"));
	const FString UnsubscribeBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::UnsubscribeLevelObjectEvents(ULevel* Level)"));

	TestTrue(TEXT("All-level subscription should respect excluded-level filtering"),
		SubscribeAllBody.Contains(TEXT("ShouldStoreLevel(Level)")) &&
		SubscribeAllBody.Contains(TEXT("SubscribeLevelObjectEvents(Level)")));
	TestTrue(TEXT("All-level unsubscription should use the same excluded-level filtering"),
		UnsubscribeAllBody.Contains(TEXT("ShouldStoreLevel(Level)")) &&
		UnsubscribeAllBody.Contains(TEXT("UnsubscribeLevelObjectEvents(Level)")));
	TestTrue(TEXT("Destroy delegates should attach only to persistent level-authored actors"),
		SubscribeBody.Contains(TEXT("SpudPropertyUtil::IsPersistentObject(Actor)")) &&
		SubscribeBody.Contains(TEXT("!SpudPropertyUtil::IsRuntimeActor(Actor)")) &&
		SubscribeBody.Contains(TEXT("Actor->OnDestroyed.AddUniqueDynamic(this, &USpudSubsystem::OnActorDestroyed)")));
	TestTrue(TEXT("Destroy delegates should detach with the same persistent level-authored actor filter"),
		UnsubscribeBody.Contains(TEXT("SpudPropertyUtil::IsPersistentObject(Actor)")) &&
		UnsubscribeBody.Contains(TEXT("!SpudPropertyUtil::IsRuntimeActor(Actor)")) &&
		UnsubscribeBody.Contains(TEXT("Actor->OnDestroyed.RemoveDynamic(this, &USpudSubsystem::OnActorDestroyed)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorIgnoreNonGameplaySourceTest,
	"SPUD.DestroyedActorPersistence.ignores_teardown_travel_and_unload_destruction",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorIgnoreNonGameplaySourceTest::RunTest(const FString&)
{
	const FString Source = SpudDestroyedActorPersistenceTest::StripComments(
		SpudDestroyedActorPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));
	const FString DestroyedBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::OnActorDestroyed(AActor* Actor)"));

	TestTrue(TEXT("Actor destruction should be recorded only during normal idle gameplay"),
		DestroyedBody.Contains(TEXT("CurrentState == ESpudSystemState::RunningIdle")));
	TestTrue(TEXT("Actor destruction from level removal/unload should be ignored"),
		DestroyedBody.Contains(TEXT("Level && !Level->bIsBeingRemoved")));
	TestTrue(TEXT("Valid gameplay destruction should record a destroyed level actor tombstone"),
		DestroyedBody.Contains(TEXT("GetActiveState()")) &&
		DestroyedBody.Contains(TEXT("State->StoreLevelActorDestroyed(Actor)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDestroyedActorStreamingLifecycleSourceTest,
	"SPUD.DestroyedActorPersistence.streaming_restore_store_lifecycle_keeps_delegates_in_sync",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDestroyedActorStreamingLifecycleSourceTest::RunTest(const FString&)
{
	const FString Source = SpudDestroyedActorPersistenceTest::StripComments(
		SpudDestroyedActorPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));
	const FString PostLoadBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::PostLoadStreamLevelGameThread(FName LevelName)"));
	const FString UnloadBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::HandleLevelUnloaded(ULevel* Level)"));
	const FString PostMapBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::OnPostLoadMap(UWorld* World, const int32 UserIndex)"));
	const FString SubscribeAllBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::SubscribeAllLevelObjectEvents()"));
	const FString SubscribeBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::SubscribeLevelObjectEvents(ULevel* Level)"));
	const FString UnsubscribeBody = SpudDestroyedActorPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::UnsubscribeLevelObjectEvents(ULevel* Level)"));

	TestTrue(TEXT("Streaming restore should subscribe destroyed actor delegates after level state restore"),
		PostLoadBody.Contains(TEXT("GetActiveState()->RestoreLevel(Level)")) &&
		PostLoadBody.Contains(TEXT("SubscribeLevelObjectEvents(Level)")) &&
		SubscribeBody.Contains(TEXT("Actor->OnDestroyed.AddUniqueDynamic(this, &USpudSubsystem::OnActorDestroyed)")));
	TestTrue(TEXT("Streaming unload should detach destroyed actor delegates before storing/releasing level state"),
		UnloadBody.Contains(TEXT("UnsubscribeLevelObjectEvents(Level)")) &&
		UnloadBody.Contains(TEXT("StoreLevel(Level, true, false)")) &&
		UnsubscribeBody.Contains(TEXT("Actor->OnDestroyed.RemoveDynamic(this, &USpudSubsystem::OnActorDestroyed)")));
	TestTrue(TEXT("Map restore/new-game paths should subscribe existing loaded levels for destroyed actor tracking"),
		PostMapBody.Contains(TEXT("SubscribeAllLevelObjectEvents()")) &&
		SubscribeAllBody.Contains(TEXT("SubscribeLevelObjectEvents(Level)")));

	return true;
}
