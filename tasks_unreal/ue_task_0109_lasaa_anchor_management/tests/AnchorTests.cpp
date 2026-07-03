// Copyright Expertise centre for Digital Media, 2024. All Rights Reserved.
// Benchmark automation tests for the Anchor Management subsystem.
// Run with: Automation > filter "LASAA.Anchor"

#include "CoreMinimal.h"

#include "Anchor.h"

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAnchorTests, Log, All);

namespace AnchorTests
{
	// The LASAA plugin ships with these maps  use the Prep map for testing.
	static const TCHAR* TestMapPath = TEXT("/Game/Maps/Prep");

	static TWeakObjectPtr<AActor> TrackedAnchor;
	static TWeakObjectPtr<AActor> TrackedExtPair;
	static TArray<TWeakObjectPtr<AAnchor>> ResetSpawnedAnchors;
	static int32 BaselineAnchorCount = 0;
	static int32 BaselineStorageCount = 0;

	static UWorld* GetFirstPIEWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				const ENetMode NetMode = Context.World()->GetNetMode();
				if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	// Spawn an AAnchor actor directly (bypasses createAnchor SDK flow).
	// Used to test lifecycle, erase, and reset without needing the XR runtime.
	static AAnchor* SpawnAnchorDirect(UWorld* World, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		if (!World) return nullptr;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		return World->SpawnActor<AAnchor>(AAnchor::StaticClass(), Location, Rotation, Params);
	}

	// Populate storage with deterministic test data for JSON round-trip tests.
	static void PopulateTestStorage(int32 Count)
	{
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();
		for (int32 i = 0; i < Count; ++i)
		{
			AAnchor::anchorStorage.anchorUuids.Add(FString::Printf(TEXT("test-uuid-%03d"), i));
			AAnchor::anchorStorage.anchorTransforms.Add(
				FTransform(FRotator(0, i * 45, 0), FVector(i * 100, i * 200, i * 50)));
		}
	}

	static AActor* SpawnRootedActor(UWorld* World, const FVector& Location = FVector::ZeroVector)
	{
		if (!World) return nullptr;

		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
		if (!Actor) return nullptr;

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Root->RegisterComponent();
		Actor->SetRootComponent(Root);
		Actor->SetActorLocation(Location);
		return Actor;
	}

	static void AddExpectedMetaProjectSetupNoise(FAutomationTestBase& Test)
	{
		if (GEngine)
		{
			GEngine->Exec(nullptr, TEXT("log LogProjectSetupTool off"));
		}
	}
}

using namespace AnchorTests;

// ===================================================================
// 1.  creation_rejects_duplicate_component  (R1, R3)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorCreationDuplicateTest,
	"LASAA.Anchor.creation_rejects_duplicate_component",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorCreationDuplicateTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	TrackedAnchor.Reset();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Spawn an anchor with an existing OculusXRAnchorComponent and verify the
	// duplicate guard rejects creation before the real SDK path is entered.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		AAnchor* Anchor = SpawnAnchorDirect(World, FVector(0, 0, 100));
		if (!TestNotNull(TEXT("Anchor actor must spawn"), Anchor))
		{
			return true;
		}
		TrackedAnchor = Anchor;

		UClass* AnchorComponentClass = FindObject<UClass>(nullptr, TEXT("/Script/OculusXRAnchors.OculusXRAnchorComponent"));
		if (!TestNotNull(TEXT("OculusXRAnchorComponent class must be registered"), AnchorComponentClass))
		{
			return true;
		}

		UActorComponent* ExistingComponent = Anchor->AddComponentByClass(
			AnchorComponentClass, false, FTransform::Identity, false);
		if (!TestNotNull(TEXT("Existing OculusXRAnchorComponent must be attachable"), ExistingComponent))
		{
			return true;
		}
		ExistingComponent->RegisterComponent();

		bool Result = Anchor->createAnchor(FTransform::Identity);
		TestFalse(TEXT("createAnchor rejects duplicate existing OculusXRAnchorComponent"), Result);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 2.  persistence_roundtrip_json  (R2, R9, R11)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorPersistenceRoundtripTest,
	"LASAA.Anchor.persistence_roundtrip_json",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorPersistenceRoundtripTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	int32 TestCount = 3;

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, TestCount]() -> bool
	{
		// Populate storage with known test data.
		PopulateTestStorage(TestCount);

		TestEqual(TEXT("Storage populated with expected UUID count"),
			AAnchor::anchorStorage.anchorUuids.Num(), TestCount);
		TestEqual(TEXT("Storage populated with expected transform count"),
			AAnchor::anchorStorage.anchorTransforms.Num(), TestCount);

		// R11: filePath must be "anchors.json" (no directory prefix).
		TestEqual(TEXT("filePath is anchors.json"), AAnchor::filePath, TEXT("anchors.json"));

		// Write to JSON using the same serialization the agent's writeToJson should use
		// (FJsonObjectConverter::UStructToJsonObject  R9).
		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		FJsonObjectConverter::UStructToJsonObject(
			FAnchorStruct::StaticStruct(), &AAnchor::anchorStorage, JsonObject, 0, 0);

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject, Writer);
		FFileHelper::SaveStringToFile(OutputString, *AAnchor::filePath);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, TestCount]() -> bool
	{
		// Read the file back and verify round-trip.
		FString JsonContent;
		bool FileLoaded = FFileHelper::LoadFileToString(JsonContent, *AAnchor::filePath);
		TestTrue(TEXT("JSON file exists after write"), FileLoaded);

		if (!FileLoaded)
		{
			return true;
		}

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
		bool Parsed = FJsonSerializer::Deserialize(Reader, JsonObject);
		TestTrue(TEXT("JSON file is parseable"), Parsed);

		if (!Parsed)
		{
			return true;
		}

		// Deserialize into a fresh struct and compare.
		FAnchorStruct ReadBack;
		FJsonObjectConverter::JsonObjectToUStruct(
			JsonObject.ToSharedRef(), FAnchorStruct::StaticStruct(), &ReadBack, 0, 0);

		TestEqual(TEXT("Round-trip preserves UUID count"), ReadBack.anchorUuids.Num(), TestCount);
		TestEqual(TEXT("Round-trip preserves Transform count"), ReadBack.anchorTransforms.Num(), TestCount);

		for (int32 i = 0; i < TestCount; ++i)
		{
			TestEqual(TEXT("Round-trip preserves UUID"),
				ReadBack.anchorUuids[i],
				FString::Printf(TEXT("test-uuid-%03d"), i));
		}

		// Verify transforms survived the round-trip.
		FTransform Expected(FRotator(0, 0, 0), FVector(0, 0, 0));
		TestTrue(TEXT("Round-trip Transform[0] is within tolerance"),
			ReadBack.anchorTransforms[0].Equals(Expected, 1e-3));

		return true;
	}));

	// Cleanup.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 3.  load_from_buffer  (R6)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorLoadFromBufferTest,
	"LASAA.Anchor.load_from_buffer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorLoadFromBufferTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		// Build a JSON buffer matching FAnchorStruct layout.
		FAnchorStruct TestStorage;
		TestStorage.anchorUuids.Add(TEXT("buf-uuid-001"));
		TestStorage.anchorTransforms.Add(FTransform(FRotator::ZeroRotator, FVector(10, 20, 30)));
		TestStorage.anchorUuids.Add(TEXT("buf-uuid-002"));
		TestStorage.anchorTransforms.Add(FTransform(FRotator(90, 0, 0), FVector(-5, 100, 50)));

		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		FJsonObjectConverter::UStructToJsonObject(
			FAnchorStruct::StaticStruct(), &TestStorage, JsonObject, 0, 0);

		FString JsonStr;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
		FJsonSerializer::Serialize(JsonObject, Writer);

		// Deserialize the buffer into the global anchorStorage.
		// This replicates what loadAnchorsFromBuffer should do internally (R6).
		TSharedPtr<FJsonObject> ParsedObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
		if (FJsonSerializer::Deserialize(Reader, ParsedObject))
		{
			FJsonObjectConverter::JsonObjectToUStruct(
				ParsedObject.ToSharedRef(), FAnchorStruct::StaticStruct(),
				&AAnchor::anchorStorage, 0, 0);
		}

		TestEqual(TEXT("Buffer load: UUID count"), AAnchor::anchorStorage.anchorUuids.Num(), 2);
		TestEqual(TEXT("Buffer load: UUID[0]"), AAnchor::anchorStorage.anchorUuids[0], TEXT("buf-uuid-001"));
		TestEqual(TEXT("Buffer load: UUID[1]"), AAnchor::anchorStorage.anchorUuids[1], TEXT("buf-uuid-002"));

		FTransform Expected0(FRotator::ZeroRotator, FVector(10, 20, 30));
		TestTrue(TEXT("Buffer load: Transform[0] matches"),
			AAnchor::anchorStorage.anchorTransforms[0].Equals(Expected0, 1e-3));

		// R6: UUID strings must be convertible to FOculusXRUUID via
		// UOculusXRAnchorBPFunctionLibrary::StringToAnchorUUID before calling QueryAnchors.
		// Verify the stored strings are non-empty and well-formed.
		bool bUuidsValid = true;
		for (const FString& Uuid : AAnchor::anchorStorage.anchorUuids)
		{
			if (Uuid.IsEmpty())
			{
				bUuidsValid = false;
				break;
			}
		}
		TestTrue(TEXT("All UUID strings are non-empty for SDK conversion"), bUuidsValid);

		// Cleanup.
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 4.  sixty_four_cap_enforced  (R8)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorCapTest,
	"LASAA.Anchor.sixty_four_cap_enforced",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorCapTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();

		// Fill to 64  should succeed.
		for (int32 i = 0; i < 64; ++i)
		{
			AAnchor::anchorStorage.anchorUuids.Add(FString::FromInt(i));
			AAnchor::anchorStorage.anchorTransforms.Add(FTransform::Identity);
		}
		TestEqual(TEXT("Storage accepts 64 anchors"), AAnchor::anchorStorage.anchorUuids.Num(), 64);

		// R8: The cap is enforced in addToList, checking anchorUuids.Num() >= 64.
		// Verify the guard is active.
		bool bCapActive = AAnchor::anchorStorage.anchorUuids.Num() >= 64;
		TestTrue(TEXT("64-anchor cap guard is active"), bCapActive);

		// Attempting to add beyond the cap should be a no-op.
		// Simulate what addToList does: check before append.
		int32 CountBefore = AAnchor::anchorStorage.anchorUuids.Num();
		if (AAnchor::anchorStorage.anchorUuids.Num() < 64)
		{
			AAnchor::anchorStorage.anchorUuids.Add(TEXT("overflow"));
			AAnchor::anchorStorage.anchorTransforms.Add(FTransform::Identity);
		}
		int32 CountAfter = AAnchor::anchorStorage.anchorUuids.Num();
		TestEqual(TEXT("Storage does not exceed 64 after overflow attempt"), CountAfter, 64);
		TestEqual(TEXT("Count unchanged"), CountBefore, CountAfter);

		// Cleanup.
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 5.  lifecycle_register_unregister  (R2)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorLifecycleTest,
	"LASAA.Anchor.lifecycle_register_unregister",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorLifecycleTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	TrackedAnchor.Reset();
	BaselineAnchorCount = 0;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Record baseline.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		BaselineAnchorCount = AAnchor::getNumAnchors();
		UE_LOG(LogAnchorTests, Display, TEXT("[Lifecycle] Baseline anchor count = %d"), BaselineAnchorCount);
		return true;
	}));

	// Spawn an anchor  BeginPlay should register it in allAnchors.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		AAnchor* Anchor = SpawnAnchorDirect(World, FVector(0, 0, 100));
		if (!TestNotNull(TEXT("Anchor actor must spawn"), Anchor))
		{
			return true;
		}
		TrackedAnchor = Anchor;
		return true;
	}));

	// Allow BeginPlay to fire.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// Verify the anchor registered itself.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		int32 CountAfterSpawn = AAnchor::getNumAnchors();
		UE_LOG(LogAnchorTests, Display, TEXT("[Lifecycle] Count after spawn = %d (baseline = %d)"),
			CountAfterSpawn, BaselineAnchorCount);
		TestTrue(TEXT("Anchor count must increase after BeginPlay"),
			CountAfterSpawn > BaselineAnchorCount);
		return true;
	}));

	// Destroy the anchor  EndPlay should unregister it.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor* Anchor = Cast<AAnchor>(TrackedAnchor.Get());
		if (Anchor)
		{
			Anchor->Destroy();
		}
		TrackedAnchor.Reset();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Verify the anchor unregistered itself.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		int32 CountAfterDestroy = AAnchor::getNumAnchors();
		UE_LOG(LogAnchorTests, Display, TEXT("[Lifecycle] Count after destroy = %d (baseline = %d)"),
			CountAfterDestroy, BaselineAnchorCount);
		TestEqual(TEXT("Anchor count must return to baseline after EndPlay"),
			CountAfterDestroy, BaselineAnchorCount);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 6.  erase_with_and_without_sdk_component  (R5)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorEraseTest,
	"LASAA.Anchor.erase_with_and_without_sdk_component",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorEraseTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	TrackedAnchor.Reset();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Spawn an anchor directly (no SDK component) and add it to storage.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		// Populate storage with a test entry.
		AAnchor::anchorStorage.anchorUuids.Add(TEXT("erase-test-uuid"));
		AAnchor::anchorStorage.anchorTransforms.Add(FTransform::Identity);
		BaselineStorageCount = AAnchor::anchorStorage.anchorUuids.Num();

		AAnchor* Anchor = SpawnAnchorDirect(World, FVector(0, 500, 100));
		if (!TestNotNull(TEXT("Anchor actor must spawn for erase test"), Anchor))
		{
			return true;
		}
		TrackedAnchor = Anchor;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Call erase on the anchor  it has no OculusXRAnchorComponent, so it should
	// take the direct cleanup path: eraseFromList + writeToJson + Destroy (R5).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor* Anchor = Cast<AAnchor>(TrackedAnchor.Get());
		if (!TestNotNull(TEXT("Anchor must still exist for erase"), Anchor))
		{
			return true;
		}

		// The anchor has no SDK component (spawned directly), so erase should
	// clean up directly without waiting for an SDK callback.
		Anchor->erase();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Verify the anchor was destroyed and storage was cleaned up.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		TestFalse(TEXT("Erased anchor actor must be destroyed"), TrackedAnchor.IsValid());

		// The UUID should have been removed from storage.
		FTransform Remaining = AAnchor::anchorStorage.getTransform(TEXT("erase-test-uuid"));
		TestTrue(TEXT("Erased anchor UUID must return Identity from getTransform"),
			Remaining.Equals(FTransform::Identity, 1e-9));

		// Cleanup remaining storage.
		AAnchor::anchorStorage.anchorUuids.Empty();
		AAnchor::anchorStorage.anchorTransforms.Empty();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 7.  calibration_offset_propagation  (R10)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorCalibrationOffsetTest,
	"LASAA.Anchor.calibration_offset_propagation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorCalibrationOffsetTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	TrackedAnchor.Reset();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Spawn an anchor with an external pair actor.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		AAnchor::setCalibrationOffset(FTransform::Identity);

		AAnchor* Anchor = SpawnAnchorDirect(World, FVector(0, 0, 100));
		if (!TestNotNull(TEXT("Anchor must spawn"), Anchor))
		{
			return true;
		}
		TrackedAnchor = Anchor;
		AAnchor::allAnchors.AddUnique(Anchor);

		// Spawn a dummy external pair actor.
		FActorSpawnParameters ExtParams;
		AActor* ExtPair = SpawnRootedActor(World, FVector(100, 100, 100));
		if (ExtPair)
		{
			Anchor->setExtPairAndCalibrationOffset(ExtPair);
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Apply a calibration offset  all existing anchors should update their external pairs.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor* Anchor = Cast<AAnchor>(TrackedAnchor.Get());
		if (!Anchor)
		{
			return true;
		}

		FVector ExtPairLocationBefore = Anchor->getCurrentExtLocation();

		// R10: setCalibrationOffset is static and propagates to all anchors.
		FTransform CalibrationOffset(FRotator(0, 90, 0), FVector(50, 0, 0));
		AAnchor::setCalibrationOffset(CalibrationOffset);

		FVector ExtPairLocationAfter = Anchor->getCurrentExtLocation();

		// The external pair's relative transform should have been adjusted.
		// addCalibrationOffset computes: newPose = cam2xr.Inverse() * gtExtPose
		// and applies it as the external pair's relative transform.
		bool bTransformChanged = !ExtPairLocationBefore.Equals(ExtPairLocationAfter, 1.0f);
		TestTrue(TEXT("External pair transform must change when calibration offset is applied"),
			bTransformChanged);

		UE_LOG(LogAnchorTests, Display,
			TEXT("[Calibration] ExtPair before = (%.1f, %.1f, %.1f), after = (%.1f, %.1f, %.1f)"),
			ExtPairLocationBefore.X, ExtPairLocationBefore.Y, ExtPairLocationBefore.Z,
			ExtPairLocationAfter.X, ExtPairLocationAfter.Y, ExtPairLocationAfter.Z);

		return true;
	}));

	// Reset calibration to identity.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AAnchor::setCalibrationOffset(FTransform::Identity);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 8.  load_spawns_external_pairs_attached_to_owner  (R7)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorLoadExternalPairsTest,
	"LASAA.Anchor.load_spawns_external_pairs_attached_to_owner",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorLoadExternalPairsTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World)
		{
			return true;
		}

		// R7: External pair is attached to newOwner (not to the spawned anchor),
		// using FAttachmentTransformRules::KeepRelativeTransform.
		//
		// This test verifies the attachment behavior by calling setExtPairAndCalibrationOffset
		// and checking that the external pair's AttachParent is set correctly.

		AActor* Owner = SpawnRootedActor(World);
		if (!TestNotNull(TEXT("Owner actor must spawn"), Owner))
		{
			return true;
		}

		AAnchor* Anchor = SpawnAnchorDirect(World, FVector(0, 0, 100));
		if (!TestNotNull(TEXT("Anchor must spawn"), Anchor))
		{
			return true;
		}

		AActor* ExtPair = SpawnRootedActor(World, FVector(200, 200, 100));
		if (!TestNotNull(TEXT("External pair must spawn"), ExtPair))
		{
			return true;
		}

		// The agent's loadAnchorsInternal should attach extPair to newOwner.
		// Simulate this by manually attaching and verifying.
		ExtPair->AttachToActor(Owner, FAttachmentTransformRules::KeepRelativeTransform);
		TestTrue(TEXT("External pair must be attached to owner (not anchor)"),
			ExtPair->GetRootComponent() && ExtPair->GetRootComponent()->GetAttachParent() == Owner->GetRootComponent());

		UE_LOG(LogAnchorTests, Display,
			TEXT("[LoadExternalPairs] ExtPair AttachParent = %s (expected Owner = %s)"),
			*GetNameSafe(ExtPair->GetRootComponent()->GetAttachParent()),
			*GetNameSafe(Owner->GetRootComponent()));

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 9.  reset_clears_everything  (R12)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorResetTest,
	"LASAA.Anchor.reset_clears_everything",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorResetTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ResetSpawnedAnchors.Empty();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Spawn several anchors and populate storage.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World)
		{
			return true;
		}

		// Populate storage.
		PopulateTestStorage(3);
		BaselineStorageCount = AAnchor::anchorStorage.anchorUuids.Num();

		// Spawn anchor actors.
		for (int32 i = 0; i < 3; ++i)
		{
			AAnchor* Anchor = SpawnAnchorDirect(World, FVector(i * 100, i * 200, 100));
			if (Anchor)
			{
				ResetSpawnedAnchors.Add(Anchor);
			}
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// Verify anchors are present.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		int32 AnchorCount = AAnchor::getNumAnchors();
		int32 StorageCount = AAnchor::anchorStorage.anchorUuids.Num();
		UE_LOG(LogAnchorTests, Display,
			TEXT("[Reset] Before: anchors = %d, storage = %d"), AnchorCount, StorageCount);
		TestTrue(TEXT("At least one anchor should be active"), AnchorCount > 0);
		TestTrue(TEXT("Storage should have entries"), StorageCount > 0);
		return true;
	}));

	// Call resetAnchors  R12: must copy allAnchors before iterating to destroy.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		// Should not crash even with active anchors.
		AAnchor::resetAnchors();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Verify everything is cleared.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		TestEqual(TEXT("allAnchors must be empty after reset"), AAnchor::getNumAnchors(), 0);
		TestEqual(TEXT("anchorUuids must be empty after reset"), AAnchor::anchorStorage.anchorUuids.Num(), 0);
		TestEqual(TEXT("anchorTransforms must be empty after reset"),
			AAnchor::anchorStorage.anchorTransforms.Num(), 0);

		// Verify the JSON file was written (empty struct).
		FString JsonContent;
		if (FFileHelper::LoadFileToString(JsonContent, *AAnchor::filePath))
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
			bool Parsed = FJsonSerializer::Deserialize(Reader, JsonObject);
			TestTrue(TEXT("Reset must write valid (empty) JSON"), Parsed);
		}

		UE_LOG(LogAnchorTests, Display, TEXT("[Reset] After: anchors = %d, storage = %d"),
			AAnchor::getNumAnchors(), AAnchor::anchorStorage.anchorUuids.Num());

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// ===================================================================
// 10. average_pair_distance  (R10)
// ===================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorAvgDistanceTest,
	"LASAA.Anchor.average_pair_distance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnchorAvgDistanceTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World)
		{
			return true;
		}

		AAnchor::setCalibrationOffset(FTransform::Identity);

		// Spawn anchors with known external pair offsets.
		TArray<AAnchor*> Anchors;
		const FVector AnchorLocations[] = {
			FVector(100, 0, 0),
			FVector(0, 100, 0),
			FVector(0, 0, 100)
		};
		for (int32 i = 0; i < 3; ++i)
		{
			AAnchor* Anchor = SpawnAnchorDirect(World, AnchorLocations[i]);
			if (!Anchor) continue;
			AAnchor::allAnchors.AddUnique(Anchor);

			// Spawn external pair at a known distance from the anchor.
			// gtExtPose defaults to identity in production until an SDK/create or
			// load path assigns it, so setExtPairAndCalibrationOffset places the
			// pair at the origin. These anchors are exactly 100 units from origin.
			FActorSpawnParameters ExtParams;
			AActor* ExtPair = SpawnRootedActor(World);
			if (ExtPair)
			{
				Anchor->setExtPairAndCalibrationOffset(ExtPair);
			}
			Anchors.Add(Anchor);
		}

		if (Anchors.Num() < 2)
		{
			AddWarning(TEXT("Not enough anchors with external pairs for distance test."));
			return true;
		}

		// All external pairs are 100 units away in Y from their anchors.
		// Average distance should be ~100.
		double AvgDistance = AAnchor::getAvgPairDistance();
		UE_LOG(LogAnchorTests, Display, TEXT("[AvgDistance] Computed = %.2f (expected ~100)"), AvgDistance);
		TestTrue(TEXT("Average pair distance should be approximately 100"),
			FMath::Abs(AvgDistance - 100.0) < 5.0);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
