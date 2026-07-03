// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ECRAssetManager.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/ECRGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Stats/StatsMisc.h"
#include "Engine/Engine.h"
#include "Gameplay/GAS/ECRGameplayCueManager.h"
#include "Misc/ScopedSlowTask.h"

const FName FECRBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("ECR.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(UECRAssetManager::DumpLoadedAssets)
);

//////////////////////////////////////////////////////////////////////

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FECRAssetManagerStartupJob(#JobFunc, [this](const FECRAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}
, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

//////////////////////////////////////////////////////////////////////

UECRAssetManager::UECRAssetManager()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UECRAssetManager& UECRAssetManager::Get()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	static UECRAssetManager StubValue{};
	return StubValue;
}


UObject* UECRAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


bool UECRAssetManager::ShouldLogAssetLoads()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRAssetManager::AddLoadedAsset(const UObject* Asset)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRAssetManager::DumpLoadedAssets()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRAssetManager::StartInitialLoading()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRAssetManager::InitializeAbilitySystem()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRAssetManager::InitializeGameplayCueManager()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



const UECRPawnData* UECRAssetManager::GetDefaultPawnData() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}



void UECRAssetManager::DoAllStartupJobs()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if WITH_EDITOR
void UECRAssetManager::PreBeginPIE(bool bStartSimulate)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif