#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#define protected public
#include "SpudStreamingVolume.h"
#undef protected

namespace SpudStreamingLevelPersistenceTest
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
		for (int32 i = 0; i < Source.Len(); ++i)
		{
			const TCHAR C = Source[i];
			const TCHAR N = i + 1 < Source.Len() ? Source[i + 1] : TCHAR('\0');
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
					++i;
				}
				continue;
			}
			if (bString)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++i;
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
				++i;
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('*'))
			{
				bBlock = true;
				++i;
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
			const TCHAR C = Source[Index];
			if (C == TCHAR('{'))
			{
				++Depth;
			}
			else if (C == TCHAR('}'))
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
	FSpudStreamingVolumeFiltersPawnsTest,
	"SPUD.StreamingLevelPersistence.streaming_volume_ignores_unpossessed_pawns",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingVolumeFiltersPawnsTest::RunTest(const FString&)
{
	UWorld* World = SpudStreamingLevelPersistenceTest::CreateCleanGameWorld();
	if (!TestNotNull(TEXT("Game world should be available"), World))
	{
		return true;
	}

	ASpudStreamingVolume* Volume = World->SpawnActor<ASpudStreamingVolume>();
	APawn* UnpossessedPawn = World->SpawnActor<APawn>();
	AActor* CameraLikeActor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Streaming volume should spawn"), Volume) ||
		!TestNotNull(TEXT("Pawn should spawn"), UnpossessedPawn) ||
		!TestNotNull(TEXT("Actor should spawn"), CameraLikeActor))
	{
		SpudStreamingLevelPersistenceTest::DestroyGameWorld(World);
		return true;
	}

	TestFalse(TEXT("Unpossessed pawns must not keep streaming levels loaded"),
		Volume->IsRelevantActor(UnpossessedPawn));
	TestTrue(TEXT("Non-pawn camera overlap actors should remain relevant"),
		Volume->IsRelevantActor(CameraLikeActor));

	SpudStreamingLevelPersistenceTest::DestroyGameWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingVolumePossessionTrackingSourceTest,
	"SPUD.StreamingLevelPersistence.streaming_volume_tracks_possession_changes_for_overlapping_pawns",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingVolumePossessionTrackingSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudStreamingVolume.cpp")));

	const FString BeginPlayBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::BeginPlay()"));
	const FString EndPlayBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)"));
	const FString ControllerChangedBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::OnPawnControllerChanged(APawn* Pawn, AController* NewCtrl)"));
	const FString BeginOverlapBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::NotifyActorBeginOverlap(AActor* OtherActor)"));
	const FString EndOverlapBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::NotifyActorEndOverlap(AActor* OtherActor)"));

	TestTrue(TEXT("Streaming volumes should subscribe to pawn possession changes"),
		BeginPlayBody.Contains(TEXT("GetOnPawnControllerChanged().AddDynamic(this, &ASpudStreamingVolume::OnPawnControllerChanged)")));
	TestTrue(TEXT("Streaming volumes should unsubscribe from pawn possession changes"),
		EndPlayBody.Contains(TEXT("GetOnPawnControllerChanged().RemoveDynamic(this, &ASpudStreamingVolume::OnPawnControllerChanged)")));
	TestTrue(TEXT("All overlapping pawns should be tracked even before they become relevant"),
		BeginOverlapBody.Contains(TEXT("PawnsInVolume.Add(Pawn)")) &&
		EndOverlapBody.Contains(TEXT("PawnsInVolume.Remove(Pawn)")));
	TestTrue(TEXT("Possession changes should add or remove already-overlapping pawns from streaming requests"),
		ControllerChangedBody.Contains(TEXT("PawnsInVolume.Contains(Pawn)")) &&
		ControllerChangedBody.Contains(TEXT("AddRelevantActor(Pawn)")) &&
		ControllerChangedBody.Contains(TEXT("RemoveRelevantActor(Pawn)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingVolumeRequestGateSourceTest,
	"SPUD.StreamingLevelPersistence.streaming_volume_refcounts_relevant_actors_before_requesting_levels",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingVolumeRequestGateSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudStreamingVolume.cpp")));

	const FString AddBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::AddRelevantActor(AActor* Actor)"));
	const FString RemoveBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void ASpudStreamingVolume::RemoveRelevantActor(AActor* Actor)"));

	TestTrue(TEXT("Volume request entry should dedupe relevant actors and remember whether it was empty"),
		AddBody.Contains(TEXT("const int OldNum = RelevantActorsInVolume.Num()")) &&
		AddBody.Contains(TEXT("RelevantActorsInVolume.AddUnique(Actor)")));
	TestTrue(TEXT("Only the first relevant actor entering should request streaming loads"),
		AddBody.Contains(TEXT("if (OldNum == 0)")) &&
		AddBody.Contains(TEXT("PS->AddRequestForStreamingLevel(this, LevelName, false)")));
	TestTrue(TEXT("Volume request exit should withdraw only after a real removal empties the volume"),
		RemoveBody.Contains(TEXT("const int Removed = RelevantActorsInVolume.Remove(Actor)")) &&
		RemoveBody.Contains(TEXT("Removed > 0 && RelevantActorsInVolume.Num() == 0")) &&
		RemoveBody.Contains(TEXT("PS->WithdrawRequestForStreamingLevel(this, LevelName)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingRequestRefcountSourceTest,
	"SPUD.StreamingLevelPersistence.refcounts_requests_and_delays_unload",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingRequestRefcountSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Streaming requests should be tracked uniquely per requester"),
		Source.Contains(TEXT("Request.Requesters.AddUnique(Requester)")));
	TestTrue(TEXT("A new request should cancel a pending unload instead of loading again"),
		Source.Contains(TEXT("Request.bPendingUnload = false")) &&
		Source.Contains(TEXT("Request.LastRequestExpiredTime = 0")));
	TestTrue(TEXT("Only the first active requester should trigger LoadStreamLevel"),
		Source.Contains(TEXT("else if (PrevRequesters == 0)")) &&
		Source.Contains(TEXT("LoadStreamLevel(LevelName, BlockingLoad)")));
	TestTrue(TEXT("Withdrawing the final requester should mark pending unload and start the delay timer"),
		Source.Contains(TEXT("Request->bPendingUnload = true")) &&
		Source.Contains(TEXT("Request->LastRequestExpiredTime = UGameplayStatics::GetTimeSeconds(GetWorld())")) &&
		Source.Contains(TEXT("StartUnloadTimer()")));
	TestTrue(TEXT("Delayed unload should wait for no requesters and elapsed StreamLevelUnloadDelay"),
		Source.Contains(TEXT("Request.Requesters.Num() == 0")) &&
		Source.Contains(TEXT("Request.LastRequestExpiredTime <= UnloadBeforeTime")) &&
		Source.Contains(TEXT("UnloadStreamLevel(LevelName)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingPendingUnloadCancellationSourceTest,
	"SPUD.StreamingLevelPersistence.cancels_pending_unload_without_reloading",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingPendingUnloadCancellationSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestFalse(TEXT("Adding a second requester must not erase existing requesters"),
		Source.Contains(TEXT("Request.Requesters.Empty()")));
	TestTrue(TEXT("Request bookkeeping should remember the previous requester count before adding"),
		Source.Contains(TEXT("const int PrevRequesters = Request.Requesters.Num()")) &&
		Source.Contains(TEXT("Request.Requesters.AddUnique(Requester)")));
	TestTrue(TEXT("A new request during the unload grace period should cancel pending unload state"),
		Source.Contains(TEXT("if (Request.bPendingUnload)")) &&
		Source.Contains(TEXT("Request.bPendingUnload = false")) &&
		Source.Contains(TEXT("Request.LastRequestExpiredTime = 0")));
	TestTrue(TEXT("Canceling a pending unload should not issue a second LoadStreamLevel call"),
		Source.Contains(TEXT("else if (PrevRequesters == 0)")) &&
		Source.Contains(TEXT("LoadStreamLevel(LevelName, BlockingLoad)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingRestoreLifecycleSourceTest,
	"SPUD.StreamingLevelPersistence.restores_loaded_level_before_subscription",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingRestoreLifecycleSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Streaming load should prefetch level data before deferring game-thread restore"),
		Source.Contains(TEXT("GetActiveState()->PreLoadLevelData(LevelName.ToString())")));
	TestTrue(TEXT("Post-load should defer restore onto the game thread after the level is visible"),
		Source.Contains(TEXT("StreamLevel->SetShouldBeVisible(true)")) &&
		Source.Contains(TEXT("Self->PostLoadStreamLevelGameThread(LevelName)")));
	TestTrue(TEXT("Game-thread restore should bracket level restore with restore state and delegates"),
		Source.Contains(TEXT("IsRestoringState = true")) &&
		Source.Contains(TEXT("PreLevelRestore.Broadcast(LevelName.ToString())")) &&
		Source.Contains(TEXT("GetActiveState()->RestoreLevel(Level)")) &&
		Source.Contains(TEXT("PostLevelRestore.Broadcast(LevelName.ToString(), true)")) &&
		Source.Contains(TEXT("IsRestoringState = false")));
	TestTrue(TEXT("Destroyed-actor subscriptions should be attached after stored actor state is restored"),
		Source.Contains(TEXT("GetActiveState()->RestoreLevel(Level)")) &&
		Source.Contains(TEXT("SubscribeLevelObjectEvents(Level)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingUnloadStoreLifecycleSourceTest,
	"SPUD.StreamingLevelPersistence.stores_level_before_streaming_unload",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingUnloadStoreLifecycleSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Unload should broadcast before handing the loaded level to SPUD store logic"),
		Source.Contains(TEXT("PreUnloadStreamingLevel.Broadcast(LevelName)")) &&
		Source.Contains(TEXT("HandleLevelUnloaded(Level)")));
	TestTrue(TEXT("Streaming unload should detach destroyed-actor subscriptions before storing level state"),
		Source.Contains(TEXT("UnsubscribeLevelObjectEvents(Level)")) &&
		Source.Contains(TEXT("StoreLevel(Level, true, false)")));
	TestTrue(TEXT("Streaming unload should not store while loading a game or tearing down"),
		Source.Contains(TEXT("CurrentState != ESpudSystemState::LoadingGame && !bIsTearingDown")) &&
		Source.Contains(TEXT("StoreLevel(Level, true, false)")));
	TestTrue(TEXT("StoreLevel should fire SPUD store delegates around active-state persistence"),
		Source.Contains(TEXT("PreLevelStore.Broadcast(LevelName)")) &&
		Source.Contains(TEXT("GetActiveState()->StoreLevel(Level, bRelease, bBlocking)")) &&
		Source.Contains(TEXT("PostLevelStore.Broadcast(LevelName, true)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingTravelCleanupSourceTest,
	"SPUD.StreamingLevelPersistence.map_travel_clears_streaming_request_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingTravelCleanupSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Map travel should clear pending streaming requests"),
		Source.Contains(TEXT("PreTravelToNewMap.Broadcast(MapName)")) &&
		Source.Contains(TEXT("LevelRequests.Empty()")));
	TestTrue(TEXT("Map travel should stop delayed unload polling and forget monitored streaming wrappers"),
		Source.Contains(TEXT("StopUnloadTimer()")) &&
		Source.Contains(TEXT("MonitoredStreamingLevels.Empty()")));
	TestTrue(TEXT("The first streaming request after map travel should be forced blocking"),
		Source.Contains(TEXT("FirstStreamRequestSinceMapLoad = true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingFirstRequestBlockingSourceTest,
	"SPUD.StreamingLevelPersistence.first_stream_request_after_map_load_blocks_and_resets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingFirstRequestBlockingSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	const FString OnPreLoadMapBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::OnPreLoadMap(const FString& MapName)"));
	const FString LoadBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::LoadStreamLevel(FName LevelName, bool Blocking)"));

	TestTrue(TEXT("Map travel should arm the first-stream blocking flag"),
		OnPreLoadMapBody.Contains(TEXT("FirstStreamRequestSinceMapLoad = true")));
	TestTrue(TEXT("LoadStreamLevel should force the first post-travel streaming load to block and consume the flag"),
		LoadBody.Contains(TEXT("if (FirstStreamRequestSinceMapLoad)")) &&
		LoadBody.Contains(TEXT("Blocking = true")) &&
		LoadBody.Contains(TEXT("FirstStreamRequestSinceMapLoad = false")));
	TestTrue(TEXT("The forced blocking behavior must still use the normal latent streaming path"),
		LoadBody.Contains(TEXT("LevelsPendingLoad.Add(RequestID, LevelName)")) &&
		LoadBody.Contains(TEXT("UGameplayStatics::LoadStreamLevel(GetWorld(), LevelName, false, Blocking, Latent)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingExternalNotificationsSourceTest,
	"SPUD.StreamingLevelPersistence.external_streaming_notifications_route_through_lifecycle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingExternalNotificationsSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	const FString LoadedBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::NotifyLevelLoadedExternally(FName LevelName)"));
	const FString UnloadedBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::NotifyLevelUnloadedExternally(ULevel* Level)"));

	TestTrue(TEXT("Externally loaded levels should use the same restore lifecycle as SPUD-requested streaming loads"),
		LoadedBody.Contains(TEXT("HandleLevelLoaded(LevelName)")));
	TestTrue(TEXT("Externally unloaded levels should use the same store lifecycle as SPUD-requested streaming unloads"),
		UnloadedBody.Contains(TEXT("HandleLevelUnloaded(Level)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingWorldPartitionWrapperSourceTest,
	"SPUD.StreamingLevelPersistence.world_partition_wrappers_bind_and_forward_visibility_events",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingWorldPartitionWrapperSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	const FString TickBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::Tick(float DeltaTime)"));
	const FString DeinitializeBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::Deinitialize()"));
	const FString ShownBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudStreamingLevelWrapper::OnLevelShown()"));
	const FString HiddenBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudStreamingLevelWrapper::OnLevelHidden()"));

	TestTrue(TEXT("World Partition tracking should bind shown and hidden delegates for newly discovered streaming levels"),
		TickBody.Contains(TEXT("MonitoredStreamingLevels.Add(level, wrapper)")) &&
		TickBody.Contains(TEXT("level->OnLevelShown.AddUniqueDynamic(wrapper, &USpudStreamingLevelWrapper::OnLevelShown)")) &&
		TickBody.Contains(TEXT("level->OnLevelHidden.AddUniqueDynamic(wrapper, &USpudStreamingLevelWrapper::OnLevelHidden)")));
	TestTrue(TEXT("World Partition tracking should remove delegates when wrappers are discarded"),
		TickBody.Contains(TEXT("Level->OnLevelShown.RemoveAll(it.Value())")) &&
		TickBody.Contains(TEXT("Level->OnLevelHidden.RemoveAll(it.Value())")) &&
		DeinitializeBody.Contains(TEXT("Level->OnLevelShown.RemoveAll(Wrapper)")) &&
		DeinitializeBody.Contains(TEXT("Level->OnLevelHidden.RemoveAll(Wrapper)")));
	TestTrue(TEXT("World Partition shown events should forward to normal SPUD level restore handling"),
		ShownBody.Contains(TEXT("spud->HandleLevelLoaded(level)")));
	TestTrue(TEXT("World Partition hidden events should broadcast pre-unload and forward to normal store handling"),
		HiddenBody.Contains(TEXT("spud->PreUnloadStreamingLevel.Broadcast(FName(levelName))")) &&
		HiddenBody.Contains(TEXT("spud->HandleLevelUnloaded(level)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingUnloadCompletionThreadSourceTest,
	"SPUD.StreamingLevelPersistence.unload_completion_runs_on_game_thread_and_broadcasts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingUnloadCompletionThreadSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	const FString PostUnloadBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::PostUnloadStreamLevel(int32 LinkID)"));
	const FString PostUnloadGameThreadBody = SpudStreamingLevelPersistenceTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::PostUnloadStreamLevelGameThread(FName LevelName)"));

	TestTrue(TEXT("Unload completion should resolve and remove the latent unload request exactly once"),
		PostUnloadBody.Contains(TEXT("LevelsPendingUnload.FindAndRemoveChecked(LinkID)")));
	TestTrue(TEXT("Unload completion should hand off to the game thread before broadcasting"),
		PostUnloadBody.Contains(TEXT("AsyncTask(ENamedThreads::GameThread")) &&
		PostUnloadBody.Contains(TEXT("PostUnloadStreamLevelGameThread(LevelName)")));
	TestTrue(TEXT("Game-thread unload completion should broadcast the post-unload streaming delegate"),
		PostUnloadGameThreadBody.Contains(TEXT("PostUnloadStreamingLevel.Broadcast(LevelName)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingVisibilityOrderingSourceTest,
	"SPUD.StreamingLevelPersistence.keeps_streaming_level_hidden_until_restore_callback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingVisibilityOrderingSourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("LoadStreamLevel should initially keep the level hidden"),
		Source.Contains(TEXT("UGameplayStatics::LoadStreamLevel(GetWorld(), LevelName, false, Blocking, Latent)")));
	TestTrue(TEXT("Post-load should make the level visible only before SPUD restore is deferred"),
		Source.Contains(TEXT("StreamLevel->SetShouldBeVisible(true)")) &&
		Source.Contains(TEXT("HandleLevelLoaded(LevelName)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudStreamingLevelEligibilitySourceTest,
	"SPUD.StreamingLevelPersistence.filters_excluded_levels_and_runtime_actor_delegates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudStreamingLevelEligibilitySourceTest::RunTest(const FString&)
{
	const FString Source = SpudStreamingLevelPersistenceTest::StripComments(
		SpudStreamingLevelPersistenceTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Excluded level name patterns should prevent streaming-level store/restore"),
		Source.Contains(TEXT("for (const auto& Pattern : ExcludeLevelNamePatterns)")) &&
		Source.Contains(TEXT("LevelName.MatchesWildcard(Pattern)")) &&
		Source.Contains(TEXT("return false")));
	TestTrue(TEXT("Destroyed actor delegates should only be attached to persistent level actors"),
		Source.Contains(TEXT("SpudPropertyUtil::IsPersistentObject(Actor)")) &&
		Source.Contains(TEXT("!SpudPropertyUtil::IsRuntimeActor(Actor)")) &&
		Source.Contains(TEXT("Actor->OnDestroyed.AddUniqueDynamic")));
	TestTrue(TEXT("Destroyed actor delegates should be detached with the same level-actor filter"),
		Source.Contains(TEXT("!SpudPropertyUtil::IsRuntimeActor(Actor)")) &&
		Source.Contains(TEXT("Actor->OnDestroyed.RemoveDynamic")));

	return true;
}
