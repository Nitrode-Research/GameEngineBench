#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#include "Engine/GameInstance.h"
#include "SpudCustomSaveInfo.h"
#include "SpudData.h"
#include "SpudState.h"

#define private public
#define protected public
#include "SpudSubsystem.h"
#undef protected
#undef private

namespace SpudSaveArchiveLifecycleTest
{
	static constexpr const TCHAR* QuickSaveSlotName = TEXT("__QuickSave__");
	static constexpr const TCHAR* AutoSaveSlotName = TEXT("__AutoSave__");
	static constexpr const TCHAR* TaskQuickSlotName = TEXT("__SPUDTask74Quick__");
	static constexpr const TCHAR* TaskAutoSlotName = TEXT("__SPUDTask74Auto__");

	static FString TestSlotPrefix()
	{
		return TEXT("SPUD_Task74_");
	}

	static FString SlotPath(const FString& SlotName)
	{
		return USpudSubsystem::GetSaveGameFilePath(SlotName);
	}

	static void DeleteSlot(const FString& SlotName)
	{
		IFileManager::Get().Delete(*SlotPath(SlotName), false, true);
	}

	static bool SlotExists(const FString& SlotName)
	{
		return IFileManager::Get().FileExists(*SlotPath(SlotName));
	}

	static FString ExpectedNumberedSlotName(const TCHAR* BaseSlotName, int32 Number)
	{
		const FString Base(BaseSlotName);
		return Base.LeftChop(2) + FString::Printf(TEXT("_%d__"), Number);
	}

	static USpudCustomSaveInfo* MakeCustomInfo(int32 Chapter, const FString& Region)
	{
		USpudCustomSaveInfo* CustomInfo = NewObject<USpudCustomSaveInfo>();
		CustomInfo->SetInt(TEXT("Chapter"), Chapter);
		CustomInfo->SetString(TEXT("Region"), Region);
		CustomInfo->SetVector(TEXT("Checkpoint"), FVector(Chapter, Chapter + 1, Chapter + 2));
		return CustomInfo;
	}

	static USpudSubsystem* MakeSubsystem()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		return NewObject<USpudSubsystem>(GameInstance);
	}

	static TArray<uint8> MakeSaveBytes(const FString& Title, const FDateTime& Timestamp, USpudCustomSaveInfo* CustomInfo = nullptr)
	{
		USpudState* State = NewObject<USpudState>();
		State->SetTitle(FText::FromString(Title));
		State->SetTimestamp(Timestamp);
		State->SetCustomSaveInfo(CustomInfo);

		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		State->SaveToArchive(Writer);
		return Bytes;
	}

	static bool WriteSaveSlot(const FString& SlotName, const FString& Title, const FDateTime& Timestamp, USpudCustomSaveInfo* CustomInfo = nullptr)
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(SlotPath(SlotName)), true);
		const TArray<uint8> Bytes = MakeSaveBytes(Title, Timestamp, CustomInfo);
		return FFileHelper::SaveArrayToFile(Bytes, *SlotPath(SlotName));
	}

	static void CleanupTaskSlots()
	{
		DeleteSlot(TestSlotPrefix() + TEXT("ManualAlpha"));
		DeleteSlot(TestSlotPrefix() + TEXT("ManualBeta"));
		DeleteSlot(QuickSaveSlotName);
		DeleteSlot(AutoSaveSlotName);
		for (int32 Number = 1; Number <= 5; ++Number)
		{
			DeleteSlot(ExpectedNumberedSlotName(QuickSaveSlotName, Number));
			DeleteSlot(ExpectedNumberedSlotName(AutoSaveSlotName, Number));
			DeleteSlot(ExpectedNumberedSlotName(TaskQuickSlotName, Number));
			DeleteSlot(ExpectedNumberedSlotName(TaskAutoSlotName, Number));
		}
	}

	static TArray<USpudSaveGameInfo*> FilterTaskInfos(const TArray<USpudSaveGameInfo*>& Infos)
	{
		TArray<USpudSaveGameInfo*> Ret;
		for (USpudSaveGameInfo* Info : Infos)
		{
			if (Info &&
				(Info->SlotName.StartsWith(TestSlotPrefix()) ||
				 Info->SlotName == ExpectedNumberedSlotName(QuickSaveSlotName, 2) ||
				 Info->SlotName == ExpectedNumberedSlotName(AutoSaveSlotName, 2)))
			{
				Ret.Add(Info);
			}
		}
		return Ret;
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

	static void WriteUnknownChunk(FSpudChunkedDataArchive& Ar)
	{
		FSpudAdhocWrapperChunk UnknownChunk("JUNK");
		if (UnknownChunk.ChunkStart(Ar))
		{
			int32 Payload = 7401;
			Ar << Payload;
			UnknownChunk.ChunkEnd(Ar);
		}
	}

	static TArray<uint8> MakeSaveBytesWithUnknownInfoChild(const FString& Title, const FDateTime& Timestamp, USpudCustomSaveInfo* CustomInfo)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		FSpudChunkedDataArchive Ar(Writer);

		FSpudAdhocWrapperChunk SaveChunk(SPUDDATA_SAVEGAME_MAGIC);
		if (SaveChunk.ChunkStart(Ar))
		{
			FSpudAdhocWrapperChunk InfoChunk(SPUDDATA_SAVEINFO_MAGIC);
			if (InfoChunk.ChunkStart(Ar))
			{
				FSpudSaveInfo Defaults;
				uint16 SystemVersion = Defaults.SystemVersion;
				FText TitleText = FText::FromString(Title);
				FString TimestampStr = Timestamp.ToIso8601();
				Ar << SystemVersion;
				Ar << TitleText;
				Ar << TimestampStr;

				WriteUnknownChunk(Ar);
				FSpudSaveCustomInfo CustomInfoData = CustomInfo->GetData();
				CustomInfoData.WriteToArchive(Ar);

				InfoChunk.ChunkEnd(Ar);
			}
			SaveChunk.ChunkEnd(Ar);
		}

		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveBlockerNestingTest,
	"SPUD.SaveArchiveLifecycle.save_blockers_are_reason_based_and_nestable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveBlockerNestingTest::RunTest(const FString&)
{
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	USpudSubsystem* Subsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
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
	FSpudSaveArchiveHeaderMetadataTest,
	"SPUD.SaveArchiveLifecycle.header_reads_custom_info_without_full_restore",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveHeaderMetadataTest::RunTest(const FString&)
{
	USpudCustomSaveInfo* CustomInfo = SpudSaveArchiveLifecycleTest::MakeCustomInfo(7, TEXT("Harbor"));
	const FText Title = FText::FromString(TEXT("Task74 Metadata"));
	const FDateTime Timestamp(2026, 6, 24, 13, 45, 30);

	USpudState* State = NewObject<USpudState>();
	State->SetTitle(Title);
	State->SetTimestamp(Timestamp);
	State->SetCustomSaveInfo(CustomInfo);

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	State->SaveToArchive(Writer);

	FMemoryReader Reader(Bytes, true);
	USpudSaveGameInfo* LoadedInfo = NewObject<USpudSaveGameInfo>();
	TestTrue(TEXT("Save info header should load from archive"),
		USpudState::LoadSaveInfoFromArchive(Reader, *LoadedInfo));
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
		TestEqual(TEXT("Custom vector should roundtrip"), Checkpoint, FVector(7, 8, 9));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveCustomInfoOffsetRelocationTest,
	"SPUD.SaveArchiveLifecycle.custom_info_variable_updates_preserve_following_fields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveCustomInfoOffsetRelocationTest::RunTest(const FString&)
{
	USpudCustomSaveInfo* CustomInfo = NewObject<USpudCustomSaveInfo>();
	CustomInfo->SetInt(TEXT("Chapter"), 4);
	CustomInfo->SetString(TEXT("Region"), TEXT("Harbor"));
	CustomInfo->SetInt(TEXT("Score"), 99);
	CustomInfo->SetString(TEXT("Region"), TEXT("MountainPass"));

	int32 Chapter = 0;
	int32 Score = 0;
	FString Region;
	TestTrue(TEXT("Int field before rewritten variable-length value should survive relocation"),
		CustomInfo->GetInt(TEXT("Chapter"), Chapter));
	TestTrue(TEXT("Int field after rewritten variable-length value should survive relocation"),
		CustomInfo->GetInt(TEXT("Score"), Score));
	TestTrue(TEXT("Rewritten variable-length string should be readable"),
		CustomInfo->GetString(TEXT("Region"), Region));
	TestEqual(TEXT("Chapter value should remain intact"), Chapter, 4);
	TestEqual(TEXT("Score value should remain intact"), Score, 99);
	TestEqual(TEXT("Region should update to the replacement value"), Region, FString(TEXT("MountainPass")));

	const TArray<uint8> Bytes = SpudSaveArchiveLifecycleTest::MakeSaveBytes(
		TEXT("Task74 Offset"),
		FDateTime(2026, 6, 24, 15, 0, 0),
		CustomInfo);
	FMemoryReader Reader(Bytes, true);
	USpudSaveGameInfo* LoadedInfo = NewObject<USpudSaveGameInfo>();
	TestTrue(TEXT("Header should load after variable-length custom info rewrite"),
		USpudState::LoadSaveInfoFromArchive(Reader, *LoadedInfo));
	TestNotNull(TEXT("Custom info should be present after archive roundtrip"), LoadedInfo->CustomInfo.Get());
	if (LoadedInfo->CustomInfo)
	{
		Chapter = 0;
		Score = 0;
		Region.Reset();
		TestTrue(TEXT("Roundtripped chapter should be readable"), LoadedInfo->CustomInfo->GetInt(TEXT("Chapter"), Chapter));
		TestTrue(TEXT("Roundtripped score should be readable"), LoadedInfo->CustomInfo->GetInt(TEXT("Score"), Score));
		TestTrue(TEXT("Roundtripped region should be readable"), LoadedInfo->CustomInfo->GetString(TEXT("Region"), Region));
		TestEqual(TEXT("Roundtripped chapter should remain intact"), Chapter, 4);
		TestEqual(TEXT("Roundtripped score should remain intact"), Score, 99);
		TestEqual(TEXT("Roundtripped region should remain intact"), Region, FString(TEXT("MountainPass")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveHeaderOptionalChunkResetTest,
	"SPUD.SaveArchiveLifecycle.header_reader_resets_missing_optional_chunks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveHeaderOptionalChunkResetTest::RunTest(const FString&)
{
	USpudCustomSaveInfo* CustomInfo = SpudSaveArchiveLifecycleTest::MakeCustomInfo(11, TEXT("Foundry"));
	const TArray<uint8> WithCustomInfo = SpudSaveArchiveLifecycleTest::MakeSaveBytes(
		TEXT("With Custom"),
		FDateTime(2026, 6, 24, 16, 0, 0),
		CustomInfo);
	const TArray<uint8> WithoutCustomInfo = SpudSaveArchiveLifecycleTest::MakeSaveBytes(
		TEXT("Without Custom"),
		FDateTime(2026, 6, 24, 17, 0, 0),
		nullptr);

	FSpudSaveInfo StorageInfo;
	FMemoryReader ReaderWith(WithCustomInfo, true);
	FSpudChunkedDataArchive ArWith(ReaderWith);
	TestTrue(TEXT("First header read with custom info should succeed"),
		FSpudSaveData::ReadSaveInfoFromArchive(ArWith, StorageInfo));

	USpudCustomSaveInfo* LoadedCustomInfo = NewObject<USpudCustomSaveInfo>();
	LoadedCustomInfo->SetData(StorageInfo.CustomInfo);
	int32 Chapter = 0;
	TestTrue(TEXT("First read should contain custom data"),
		LoadedCustomInfo->GetInt(TEXT("Chapter"), Chapter));

	FMemoryReader ReaderWithout(WithoutCustomInfo, true);
	FSpudChunkedDataArchive ArWithout(ReaderWithout);
	TestTrue(TEXT("Second header read without custom info should succeed using same storage object"),
		FSpudSaveData::ReadSaveInfoFromArchive(ArWithout, StorageInfo));

	LoadedCustomInfo->SetData(StorageInfo.CustomInfo);
	Chapter = 0;
	TestFalse(TEXT("Missing custom info chunk should clear stale custom data from previous reads"),
		LoadedCustomInfo->GetInt(TEXT("Chapter"), Chapter));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveHeaderUnknownChunkTest,
	"SPUD.SaveArchiveLifecycle.header_reader_skips_unknown_info_children",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveHeaderUnknownChunkTest::RunTest(const FString&)
{
	USpudCustomSaveInfo* CustomInfo = SpudSaveArchiveLifecycleTest::MakeCustomInfo(13, TEXT("Vault"));
	const TArray<uint8> Bytes = SpudSaveArchiveLifecycleTest::MakeSaveBytesWithUnknownInfoChild(
		TEXT("Unknown Child"),
		FDateTime(2026, 6, 24, 18, 0, 0),
		CustomInfo);

	FMemoryReader Reader(Bytes, true);
	USpudSaveGameInfo* LoadedInfo = NewObject<USpudSaveGameInfo>();
	TestTrue(TEXT("Header-only read should skip unknown INFO child chunks"),
		USpudState::LoadSaveInfoFromArchive(Reader, *LoadedInfo));
	TestEqual(TEXT("Known title should still load after unknown child chunk"),
		LoadedInfo->Title.ToString(), FString(TEXT("Unknown Child")));
	TestNotNull(TEXT("Custom info after unknown child chunk should still load"), LoadedInfo->CustomInfo.Get());
	if (LoadedInfo->CustomInfo)
	{
		int32 Chapter = 0;
		FString Region;
		TestTrue(TEXT("Custom int after unknown child should be readable"), LoadedInfo->CustomInfo->GetInt(TEXT("Chapter"), Chapter));
		TestTrue(TEXT("Custom string after unknown child should be readable"), LoadedInfo->CustomInfo->GetString(TEXT("Region"), Region));
		TestEqual(TEXT("Custom int should survive unknown child"), Chapter, 13);
		TestEqual(TEXT("Custom string should survive unknown child"), Region, FString(TEXT("Vault")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveNumberedSlotHelpersTest,
	"SPUD.SaveArchiveLifecycle.numbered_quick_and_auto_slots_parse_and_select_latest",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveNumberedSlotHelpersTest::RunTest(const FString&)
{
	SpudSaveArchiveLifecycleTest::CleanupTaskSlots();

	int32 Number = -1;
	TestTrue(TEXT("Legacy numbered-slot base should parse as number zero"),
		USpudSubsystem::ParseNumberedSlotName(
			SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			Number));
	TestEqual(TEXT("Legacy numbered-slot base should report number zero"), Number, 0);

	const FString QuickThree = SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskQuickSlotName, 3);
	TestEqual(TEXT("Numbered formatter should use SPUD's rotation slot convention"),
		USpudSubsystem::MakeNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskQuickSlotName, 3),
		QuickThree);
	TestTrue(TEXT("Numbered slot should parse"),
		USpudSubsystem::ParseNumberedSlotName(QuickThree, SpudSaveArchiveLifecycleTest::TaskQuickSlotName, Number));
	TestEqual(TEXT("Numbered slot should preserve its suffix number"), Number, 3);
	TestFalse(TEXT("Malformed numbered slot with leading zero should not parse"),
		USpudSubsystem::ParseNumberedSlotName(
			SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskQuickSlotName, 3).Replace(TEXT("_3__"), TEXT("_03__")),
			SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			Number));
	TestFalse(TEXT("Malformed numbered slot with nonnumeric suffix should not parse"),
		USpudSubsystem::ParseNumberedSlotName(
			FString(SpudSaveArchiveLifecycleTest::TaskQuickSlotName).LeftChop(2) + TEXT("_abc__"),
			SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			Number));
	TestFalse(TEXT("Manual save names that merely contain the quick-save base should not parse"),
		USpudSubsystem::ParseNumberedSlotName(
			FString(TEXT("Manual_")) + SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			SpudSaveArchiveLifecycleTest::TaskQuickSlotName,
			Number));

	TestTrue(TEXT("Quick slot 1 should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskQuickSlotName, 1),
			TEXT("Quick One"),
			FDateTime(2026, 6, 24, 10, 0, 0)));
	TestTrue(TEXT("Quick slot 3 should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			QuickThree,
			TEXT("Quick Three"),
			FDateTime(2026, 6, 24, 12, 0, 0)));
	const FString AutoTwo = SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskAutoSlotName, 2);
	const FString AutoFour = SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::TaskAutoSlotName, 4);
	TestTrue(TEXT("Auto slot 2 should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			AutoTwo,
			TEXT("Auto Two"),
			FDateTime(2026, 6, 24, 11, 0, 0)));
	TestTrue(TEXT("Auto slot 4 should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			AutoFour,
			TEXT("Auto Four"),
			FDateTime(2026, 6, 24, 14, 0, 0)));

	USpudSubsystem* Subsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
	Subsystem->MaxQuickSaves = 5;
	Subsystem->MaxAutoSaves = 5;
	TestEqual(TEXT("Highest quick-save slot number should be discovered from files"),
		Subsystem->GetHighestSlotNumber(SpudSaveArchiveLifecycleTest::TaskQuickSlotName, 0), 3);
	TestEqual(TEXT("Highest auto-save slot number should be discovered from files"),
		Subsystem->GetHighestSlotNumber(SpudSaveArchiveLifecycleTest::TaskAutoSlotName, 0), 4);

	SpudSaveArchiveLifecycleTest::CleanupTaskSlots();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveBlockedRotationOrderingSourceTest,
	"SPUD.SaveArchiveLifecycle.blocked_rotation_saves_refuse_before_cleanup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveBlockedRotationOrderingSourceTest::RunTest(const FString&)
{
	const FString Source = SpudSaveArchiveLifecycleTest::StripComments(
		SpudSaveArchiveLifecycleTest::LoadProjectSource(
			this, TEXT("Plugins/SPUD/Source/SPUD/Private/SpudSubsystem.cpp")));
	const FString QuickBody = SpudSaveArchiveLifecycleTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::QuickSaveGame(FText Title, bool bTakeScreenshot, const USpudCustomSaveInfo* ExtraInfo, const int32 UserIndex, bool bAsync)"));
	const FString AutoBody = SpudSaveArchiveLifecycleTest::ExtractFunctionBody(
		this, Source, TEXT("void USpudSubsystem::AutoSaveGame(FText Title, bool bTakeScreenshot, const USpudCustomSaveInfo* ExtraInfo, const int32 UserIndex, bool bAsync)"));

	const int32 QuickBlockIndex = QuickBody.Find(TEXT("if (IsSaveBlocked())"));
	const int32 QuickCleanupIndex = QuickBody.Find(TEXT("CleanupOldNumberedSaves"));
	const int32 AutoBlockIndex = AutoBody.Find(TEXT("if (IsSaveBlocked())"));
	const int32 AutoCleanupIndex = AutoBody.Find(TEXT("CleanupOldNumberedSaves"));

	TestTrue(TEXT("QuickSaveGame should refuse blocked saves before numbered-slot cleanup"),
		QuickBlockIndex != INDEX_NONE &&
		QuickCleanupIndex != INDEX_NONE &&
		QuickBlockIndex < QuickCleanupIndex);
	TestTrue(TEXT("AutoSaveGame should refuse blocked saves before numbered-slot cleanup"),
		AutoBlockIndex != INDEX_NONE &&
		AutoCleanupIndex != INDEX_NONE &&
		AutoBlockIndex < AutoCleanupIndex);
	TestTrue(TEXT("Blocked quick/auto paths should report failed completion instead of continuing into SaveGame"),
		QuickBody.Contains(TEXT("SaveComplete(SPUD_QUICKSAVE_SLOTNAME, UserIndex, false)")) &&
		AutoBody.Contains(TEXT("SaveComplete(SPUD_AUTOSAVE_SLOTNAME, UserIndex, false)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveListFiltersAndSortsTest,
	"SPUD.SaveArchiveLifecycle.save_list_filters_special_slots_and_sorts_metadata",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveListFiltersAndSortsTest::RunTest(const FString&)
{
	SpudSaveArchiveLifecycleTest::CleanupTaskSlots();

	TestTrue(TEXT("Manual Alpha save should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			SpudSaveArchiveLifecycleTest::TestSlotPrefix() + TEXT("ManualAlpha"),
			TEXT("Alpha"),
			FDateTime(2026, 6, 24, 9, 0, 0)));
	TestTrue(TEXT("Manual Beta save should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			SpudSaveArchiveLifecycleTest::TestSlotPrefix() + TEXT("ManualBeta"),
			TEXT("Beta"),
			FDateTime(2026, 6, 24, 11, 0, 0)));
	TestTrue(TEXT("Quick save should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::QuickSaveSlotName, 2),
			TEXT("Quick"),
			FDateTime(2026, 6, 24, 12, 0, 0)));
	TestTrue(TEXT("Auto save should be writable"),
		SpudSaveArchiveLifecycleTest::WriteSaveSlot(
			SpudSaveArchiveLifecycleTest::ExpectedNumberedSlotName(SpudSaveArchiveLifecycleTest::AutoSaveSlotName, 2),
			TEXT("Auto"),
			FDateTime(2026, 6, 24, 13, 0, 0)));

	USpudSubsystem* Subsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
	const TArray<USpudSaveGameInfo*> ManualOnly = SpudSaveArchiveLifecycleTest::FilterTaskInfos(
		Subsystem->GetSaveGameList(false, false, ESpudSaveSorting::Title, 0));
	TestEqual(TEXT("Manual-only list should exclude numbered quick and auto slots"), ManualOnly.Num(), 2);
	if (ManualOnly.Num() == 2)
	{
		TestEqual(TEXT("Manual saves should be sorted by title"), ManualOnly[0]->Title.ToString(), FString(TEXT("Alpha")));
		TestEqual(TEXT("Manual saves should be sorted by title"), ManualOnly[1]->Title.ToString(), FString(TEXT("Beta")));
	}

	const TArray<USpudSaveGameInfo*> SlotSorted = SpudSaveArchiveLifecycleTest::FilterTaskInfos(
		Subsystem->GetSaveGameList(false, false, ESpudSaveSorting::SlotName, 0));
	TestEqual(TEXT("Manual-only slot-name sort should return the task manual saves"), SlotSorted.Num(), 2);
	if (SlotSorted.Num() == 2)
	{
		TestEqual(TEXT("Manual saves should be sorted by slot name"),
			SlotSorted[0]->SlotName, SpudSaveArchiveLifecycleTest::TestSlotPrefix() + TEXT("ManualAlpha"));
		TestEqual(TEXT("Manual saves should be sorted by slot name"),
			SlotSorted[1]->SlotName, SpudSaveArchiveLifecycleTest::TestSlotPrefix() + TEXT("ManualBeta"));
	}

	const TArray<USpudSaveGameInfo*> MostRecent = SpudSaveArchiveLifecycleTest::FilterTaskInfos(
		Subsystem->GetSaveGameList(true, true, ESpudSaveSorting::MostRecent, 0));
	TestEqual(TEXT("Including special slots should return all task saves"), MostRecent.Num(), 4);
	if (MostRecent.Num() == 4)
	{
		TestEqual(TEXT("Most recent sorting should place Auto first"),
			MostRecent[0]->Title.ToString(), FString(TEXT("Auto")));
	}

	SpudSaveArchiveLifecycleTest::CleanupTaskSlots();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudSaveArchiveCompletionCleanupTest,
	"SPUD.SaveArchiveLifecycle.save_complete_clears_deferred_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudSaveArchiveCompletionCleanupTest::RunTest(const FString&)
{
	USpudSubsystem* Subsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
	Subsystem->CurrentState = ESpudSystemState::SavingGame;
	Subsystem->SlotNameInProgress = TEXT("Task74Slot");
	Subsystem->TitleInProgress = FText::FromString(TEXT("Task74Title"));

	Subsystem->SaveComplete(TEXT("Task74Slot"), 0, true);

	TestTrue(TEXT("Save completion should return subsystem to idle"), Subsystem->IsIdle());
	TestTrue(TEXT("Slot in progress should be cleared"), Subsystem->SlotNameInProgress.IsEmpty());
	TestTrue(TEXT("Title in progress should be cleared"), Subsystem->TitleInProgress.IsEmpty());

	USpudSubsystem* EndGameSubsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
	EndGameSubsystem->CurrentState = ESpudSystemState::SavingGameAsync;
	EndGameSubsystem->bPendingEndGame = true;
	EndGameSubsystem->SaveComplete(TEXT("Task74EndGame"), 0, true);
	TestFalse(TEXT("Save completion should consume pending EndGame request"), EndGameSubsystem->bPendingEndGame);
	TestEqual(TEXT("Pending EndGame should run after async save completion"),
		EndGameSubsystem->CurrentState, ESpudSystemState::Disabled);
	EndGameSubsystem->SaveComplete(TEXT("Task74EndGame"), 0, true);
	TestEqual(TEXT("Repeated completion after pending EndGame should not re-enable persistence"),
		EndGameSubsystem->CurrentState, ESpudSystemState::Disabled);

	USpudSubsystem* NewGameSubsystem = SpudSaveArchiveLifecycleTest::MakeSubsystem();
	NewGameSubsystem->CurrentState = ESpudSystemState::SavingGameAsync;
	NewGameSubsystem->PendingNewGameArgs.Emplace(false, false);
	NewGameSubsystem->SaveComplete(TEXT("Task74NewGame"), 0, true);
	TestFalse(TEXT("Save completion should consume pending NewGame request"), NewGameSubsystem->PendingNewGameArgs.IsSet());
	TestEqual(TEXT("Pending NewGame should restart persistence after async save completion"),
		NewGameSubsystem->CurrentState, ESpudSystemState::RunningIdle);
	NewGameSubsystem->SaveComplete(TEXT("Task74NewGame"), 0, true);
	TestFalse(TEXT("Repeated completion should not resurrect consumed pending NewGame args"),
		NewGameSubsystem->PendingNewGameArgs.IsSet());
	return true;
}
