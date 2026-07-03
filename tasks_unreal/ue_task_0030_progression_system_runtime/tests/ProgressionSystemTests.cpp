// Copyright 2026 GameDevBench. Bomber progression system source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace BomberProgressionTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(Out, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProgressionSystem_SourceContract,
	"Bomber.ProgressionSystem.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProgressionSystem_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates progression save loading, global-message lifecycle, star pooling, animation, transforms, and dynamic materials.
	FString Subsystem;
	FString StarActor;
	if (!TestTrue(TEXT("PSWorldSubsystem source readable"), BomberProgressionTests::LoadSource(TEXT("Plugins/GameFeatures/ProgressionSystem/Source/ProgressionSystemRuntime/Private/Data/PSWorldSubsystem.cpp"), Subsystem))
		|| !TestTrue(TEXT("PSStarActor source readable"), BomberProgressionTests::LoadSource(TEXT("Plugins/GameFeatures/ProgressionSystem/Source/ProgressionSystemRuntime/Private/LevelActors/PSStarActor.cpp"), StarActor)))
	{
		return true;
	}

	TestTrue(TEXT("Subsystem initialization must create dynamic star materials and listen for global messages"),
		Subsystem.Contains(TEXT("GetBombDynamicProgressionOverlayMaterial")) && Subsystem.Contains(TEXT("UMaterialInstanceDynamic::Create"))
		&& Subsystem.Contains(TEXT("StarDynamicProgressMaterial")) && Subsystem.Contains(TEXT("StarLockedProgressMaterial"))
		&& Subsystem.Contains(TEXT("CallOrStartListeningForGlobalMessage")) && Subsystem.Contains(TEXT("Player_LocalPawnReady"))
		&& Subsystem.Contains(TEXT("GameState_Changed")));
	TestTrue(TEXT("Subsystem must async-load save data from configured progression save slot"),
		Subsystem.Contains(TEXT("FAsyncLoadGameFromSlot")) && Subsystem.Contains(TEXT("BindUObject(this, &ThisClass::OnAsyncLoadGameFromSlotCompleted)"))
		&& Subsystem.Contains(TEXT("AsyncLoadGameFromSlot")) && Subsystem.Contains(TEXT("GetSaveSlotName")) && Subsystem.Contains(TEXT("GetSaveSlotIndex")));
	TestTrue(TEXT("Loaded save callback must load data table rows, create missing save data, initialize rows, and broadcast readiness"),
		Subsystem.Contains(TEXT("GetProgressionDataTable")) && Subsystem.Contains(TEXT("UMyDataTable::GetRows"))
		&& Subsystem.Contains(TEXT("CreateSaveGameObject")) && Subsystem.Contains(TEXT("SetProgressionMap"))
		&& Subsystem.Contains(TEXT("SetFirstElementAsCurrent")) && Subsystem.Contains(TEXT("OnInitialized()"))
		&& Subsystem.Contains(TEXT("ProgressionSystemInitialized")) && Subsystem.Contains(TEXT("BroadcastGlobalMessage")));
	TestTrue(TEXT("Spot registration must map row names to spots and refresh stars for current row"),
		Subsystem.Contains(TEXT("RegisterSpotComponent")) && Subsystem.Contains(TEXT("GetMeshChecked().GetPlayerTag"))
		&& Subsystem.Contains(TEXT("SpotComponentsMapInternal.Add")) && Subsystem.Contains(TEXT("UpdateProgressionStarActors")));
	TestTrue(TEXT("Star update must return old pooled stars and take current-row stars from pool"),
		Subsystem.Contains(TEXT("ReturnToPoolArray")) && Subsystem.Contains(TEXT("PoolActorHandlersInternal.Empty"))
		&& Subsystem.Contains(TEXT("FOnSpawnAllCallback")) && Subsystem.Contains(TEXT("TakeFromPoolArray"))
		&& Subsystem.Contains(TEXT("GetStarActorClass")) && Subsystem.Contains(TEXT("PointsToUnlock")));
	TestTrue(TEXT("Pooled star completion must assign locked/unlocked progress and initialize transforms"),
		Subsystem.Contains(TEXT("OnTakeActorsFromPoolCompleted")) && Subsystem.Contains(TEXT("CurrentLevelProgression"))
		&& Subsystem.Contains(TEXT("FMath::Clamp")) && Subsystem.Contains(TEXT("UpdateStarActorProgressMeshMaterial"))
		&& Subsystem.Contains(TEXT("EPSStarActorState::Locked")) && Subsystem.Contains(TEXT("OnInitialized(PreviousActorLocation)")));
	TestTrue(TEXT("Star actor must play hide/menu curve animations and return to pool when hide finishes"),
		StarActor.Contains(TEXT("TryPlayHideStarAnimation")) && StarActor.Contains(TEXT("HideStarsAnimation"))
		&& StarActor.Contains(TEXT("ReturnToPool(this)")) && StarActor.Contains(TEXT("TryPlayMenuStarAnimation"))
		&& StarActor.Contains(TEXT("MenuStarsAnimation")) && StarActor.Contains(TEXT("ApplyTransformFromCurveTable")));
	TestTrue(TEXT("Star actor must initialize row transforms and offset subsequent stars"),
		StarActor.Contains(TEXT("StarActorTransform")) && StarActor.Contains(TEXT("OffsetBetweenStarActors"))
		&& StarActor.Contains(TEXT("InitialTransformInternal")) && StarActor.Contains(TEXT("SetActorTransform")));
	TestTrue(TEXT("Star actor must select dynamic materials and apply progress scalar/overlay material"),
		StarActor.Contains(TEXT("EPSStarActorState::Locked")) && StarActor.Contains(TEXT("EPSStarActorState::Partial"))
		&& StarActor.Contains(TEXT("EPSStarActorState::Unlocked")) && StarActor.Contains(TEXT("GetStarProgressionDynamicMaterial"))
		&& StarActor.Contains(TEXT("SetScalarParameterValue")) && StarActor.Contains(TEXT("SetOverlayMaterial")));

	return true;
}
