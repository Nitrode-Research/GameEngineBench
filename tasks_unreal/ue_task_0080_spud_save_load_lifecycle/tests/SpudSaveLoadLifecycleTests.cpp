#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#include "SpudSubsystem.h"

#include "SpudCustomSaveInfo.h"
#include "SpudState.h"

namespace SpudSaveLoadLifecycleTest
{
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveBlockerNestingTest,
	"SPUD.SaveLoadLifecycle.save_blockers_are_reason_based_and_nestable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveBlockerNestingTest::RunTest(const FString&)
{
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	USpudSubsystem* Subsystem = NewObject<USpudSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("SPUD subsystem should be constructible"), Subsystem))
	{
		return true;
	}

	Subsystem->AddSaveBlocker(TEXT("Combat"));
	Subsystem->AddSaveBlocker(TEXT("Combat"));
	Subsystem->AddSaveBlocker(TEXT("Cinematic"));
	TestTrue(TEXT("Any active reason should block saves"), Subsystem->IsSaveBlocked());
	TestEqual(TEXT("Nested duplicate blockers should expose unique reason names"),
		Subsystem->GetSaveBlockers().Num(), 2);

	Subsystem->RemoveSaveBlocker(TEXT("Combat"));
	TestTrue(TEXT("Removing one nested blocker should keep the reason active"),
		Subsystem->IsSaveBlocked() && Subsystem->GetSaveBlockers().Contains(FName(TEXT("Combat"))));

	Subsystem->RemoveSaveBlocker(TEXT("Combat"));
	TestTrue(TEXT("Other active reasons should continue blocking after one reason clears"),
		Subsystem->IsSaveBlocked() && !Subsystem->GetSaveBlockers().Contains(FName(TEXT("Combat"))));

	Subsystem->EndGame();
	TestFalse(TEXT("EndGame should clear stale save blockers"), Subsystem->IsSaveBlocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveHeaderMetadataRoundTripTest,
	"SPUD.SaveLoadLifecycle.save_header_roundtrips_title_timestamp_and_custom_info",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveHeaderMetadataRoundTripTest::RunTest(const FString&)
{
	USpudCustomSaveInfo* CustomInfo = NewObject<USpudCustomSaveInfo>();
	CustomInfo->SetInt(TEXT("Chapter"), 7);
	CustomInfo->SetString(TEXT("Region"), TEXT("Harbor"));
	CustomInfo->SetVector(TEXT("Checkpoint"), FVector(10.0, 20.0, 30.0));

	USpudState* State = NewObject<USpudState>();
	const FText Title = FText::FromString(TEXT("Manual Save 76"));
	const FDateTime Timestamp(2026, 6, 24, 13, 45, 30);
	State->SetTitle(Title);
	State->SetTimestamp(Timestamp);
	State->SetCustomSaveInfo(CustomInfo);

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	State->SaveToArchive(Writer);

	FMemoryReader Reader(Bytes, true);
	USpudSaveGameInfo* LoadedInfo = NewObject<USpudSaveGameInfo>();
	const bool bLoaded = USpudState::LoadSaveInfoFromArchive(Reader, *LoadedInfo);
	TestTrue(TEXT("Save info header should load from archive"), bLoaded);
	TestEqual(TEXT("Title should be read from save header"), LoadedInfo->Title.ToString(), Title.ToString());
	TestEqual(TEXT("Timestamp should be read from save header"), LoadedInfo->Timestamp, Timestamp);
	TestNotNull(TEXT("Custom info object should be recreated from save header"), LoadedInfo->CustomInfo.Get());
	if (LoadedInfo->CustomInfo)
	{
		int32 Chapter = 0;
		FString Region;
		FVector Checkpoint = FVector::ZeroVector;
		TestTrue(TEXT("Custom int should be available from header"), LoadedInfo->CustomInfo->GetInt(TEXT("Chapter"), Chapter));
		TestTrue(TEXT("Custom string should be available from header"), LoadedInfo->CustomInfo->GetString(TEXT("Region"), Region));
		TestTrue(TEXT("Custom vector should be available from header"), LoadedInfo->CustomInfo->GetVector(TEXT("Checkpoint"), Checkpoint));
		TestEqual(TEXT("Custom int should roundtrip"), Chapter, 7);
		TestEqual(TEXT("Custom string should roundtrip"), Region, FString(TEXT("Harbor")));
		TestEqual(TEXT("Custom vector should roundtrip"), Checkpoint, FVector(10.0, 20.0, 30.0));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudBlockedSaveRefusalSourceTest,
	"SPUD.SaveLoadLifecycle.blocked_saves_fail_before_slot_cleanup_or_capture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudBlockedSaveRefusalSourceTest::RunTest(const FString&)
{
	const FString HeaderSource = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Public/SpudSubsystem.h")));
	const FString Source = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("IsSaveBlocked should reflect the blocker map"),
		HeaderSource.Contains(TEXT("return !SaveBlockers.IsEmpty()")));
	TestTrue(TEXT("Manual SaveGame should refuse blocked saves with a failed completion broadcast"),
		Source.Contains(TEXT("if (IsSaveBlocked())")) &&
		Source.Contains(TEXT("SaveComplete(SlotName, UserIndex, false)")) &&
		Source.Contains(TEXT("Save to slot %s refused")));
	const bool bQuickAutoRefuseBeforeCleanup =
		Source.Contains(TEXT("QuickSaveGame refused before slot cleanup")) &&
		Source.Contains(TEXT("AutoSaveGame refused before slot cleanup")) &&
		Source.Contains(TEXT("SaveComplete(SPUD_QUICKSAVE_SLOTNAME, UserIndex, false)")) &&
		Source.Contains(TEXT("SaveComplete(SPUD_AUTOSAVE_SLOTNAME, UserIndex, false)"));
	const bool bCleanupOnlyAfterSuccessfulSave =
		Source.Contains(TEXT("CleanupSuccessfulNumberedSave")) &&
		Source.Contains(TEXT("if (bSuccess)")) &&
		Source.Contains(TEXT("CleanupSuccessfulNumberedSave(SlotName, UserIndex)"));
	TestTrue(TEXT("Blocked quick/auto saves must not delete old numbered saves"),
		bQuickAutoRefuseBeforeCleanup || bCleanupOnlyAfterSuccessfulSave);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudAsyncSaveLifecycleSourceTest,
	"SPUD.SaveLoadLifecycle.async_save_serializes_plain_state_off_thread",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudAsyncSaveLifecycleSourceTest::RunTest(const FString&)
{
	const FString Source = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("Async save should capture UObject state before leaving the game thread"),
		Source.Contains(TEXT("State->StoreWorldGlobals(World)")) &&
		Source.Contains(TEXT("StoreWorld(World, false, true)")) &&
		Source.Contains(TEXT("State->SetCustomSaveInfo(ExtraInfo)")));
	TestTrue(TEXT("Async save should serialize only the captured state on a background thread"),
		Source.Contains(TEXT("CurrentState == ESpudSystemState::SavingGameAsync")) &&
		Source.Contains(TEXT("TWeakObjectPtr WeakState(State)")) &&
		Source.Contains(TEXT("ENamedThreads::AnyBackgroundThreadNormalTask")) &&
		Source.Contains(TEXT("RawState->SaveToArchive(Archive)")));
	TestTrue(TEXT("Platform save calls and completion should return to the game thread"),
		Source.Contains(TEXT("ENamedThreads::GameThread")) &&
		Source.Contains(TEXT("SaveSystem->SaveGameAsync")) &&
		Source.Contains(TEXT("S->SaveComplete(SlotName, UserIndex, bSuccess)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudDeferredSaveCompletionSourceTest,
	"SPUD.SaveLoadLifecycle.save_completion_resumes_deferred_end_or_new_game",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudDeferredSaveCompletionSourceTest::RunTest(const FString&)
{
	const FString Source = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("NewGame during async save should be recorded as a deferred request"),
		Source.Contains(TEXT("IsSavingGame()")) &&
		Source.Contains(TEXT("PendingNewGameArgs.Emplace(bCheckServerOnly, bAfterLevelLoad)")));
	TestTrue(TEXT("EndGame during async save should be recorded without destroying active state"),
		Source.Contains(TEXT("CurrentState == ESpudSystemState::SavingGameAsync")) &&
		Source.Contains(TEXT("bPendingEndGame = true")));
	TestTrue(TEXT("SaveComplete should clear transient save fields and resume exactly one deferred request"),
		Source.Contains(TEXT("SlotNameInProgress = \"\"")) &&
		Source.Contains(TEXT("TitleInProgress = FText()")) &&
		Source.Contains(TEXT("ExtraInfoInProgress = nullptr")) &&
		Source.Contains(TEXT("PendingNewGameArgs.Reset()")) &&
		Source.Contains(TEXT("NewGame(bCheckServerOnly, bAfterLevelLoad)")) &&
		Source.Contains(TEXT("bPendingEndGame = false")) &&
		Source.Contains(TEXT("EndGame()")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudLoadTravelLifecycleSourceTest,
	"SPUD.SaveLoadLifecycle.load_restores_globals_and_pins_world_before_travel",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudLoadTravelLifecycleSourceTest::RunTest(const FString&)
{
	const FString Source = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("LoadGame should load the save archive before travelling"),
		Source.Contains(TEXT("State->LoadFromArchive(*Archive, false)")) ||
		Source.Contains(TEXT("State->LoadFromArchive(Archive, true)")));
	TestTrue(TEXT("Destination world package should be pinned before OpenLevel"),
		Source.Contains(TEXT("LoadPackage(nullptr, *LevelName, LOAD_None)")) &&
		Source.Contains(TEXT("WorldToLoad = UWorld::FindWorldInPackage(WorldPackage)")));
	TestTrue(TEXT("Persistent globals should be restored before map travel"),
		Source.Contains(TEXT("State->RestoreGlobalObject(Ptr.Get())")) &&
		Source.Contains(TEXT("State->RestoreGlobalObject(Pair.Value.Get(), Pair.Key)")));
	TestTrue(TEXT("LoadGame should defer final world restore through OpenLevel and SlotNameInProgress"),
		Source.Contains(TEXT("SlotNameInProgress = SlotName")) &&
		Source.Contains(TEXT("UGameplayStatics::OpenLevel")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudPostLoadFinalizeSourceTest,
	"SPUD.SaveLoadLifecycle.post_load_map_finalizes_restore_and_clears_world_pin",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudPostLoadFinalizeSourceTest::RunTest(const FString&)
{
	const FString Source = SpudSaveLoadLifecycleTest::StripComments(
		SpudSaveLoadLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));

	TestTrue(TEXT("OnPostLoadMap should restore the loaded world with restore delegates"),
		Source.Contains(TEXT("PreLevelRestore.Broadcast(LevelName)")) &&
		Source.Contains(TEXT("State->RestoreLoadedWorld(World)")) &&
		Source.Contains(TEXT("PostLevelRestore.Broadcast(LevelName, true)")));
	TestTrue(TEXT("Post-load restore should subscribe persistent level actors and complete the load"),
		Source.Contains(TEXT("SubscribeAllLevelObjectEvents()")) &&
		Source.Contains(TEXT("LoadComplete(SlotNameInProgress, UserIndex, true)")));
	TestTrue(TEXT("LoadComplete should clear restoring state and release the pinned world"),
		Source.Contains(TEXT("IsRestoringState = false")) &&
		Source.Contains(TEXT("PostLoadGame.Broadcast(SlotName, bSuccess)")) &&
		Source.Contains(TEXT("WorldToLoad = nullptr")));

	return true;
}
