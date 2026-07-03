// Copyright (c) Yevhenii Selivanov

#include "DalSubsystem.h"

#include "DalPrimaryDataAsset.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DalSubsystem)

UDalSubsystem& UDalSubsystem::Get()
{
	UDalSubsystem* Subsystem = GetDataAssetsLoaderSubsystem();
	checkf(Subsystem, TEXT("ASSERT: [%i] %hs:\n'Subsystem' is null"), __LINE__, __FUNCTION__);
	return *Subsystem;
}

UDalSubsystem* UDalSubsystem::GetDataAssetsLoaderSubsystem()
{
	return GEngine ? GEngine->GetEngineSubsystem<UDalSubsystem>() : nullptr;
}

const UDalPrimaryDataAsset* UDalSubsystem::GetDataAsset(TSubclassOf<UDalPrimaryDataAsset> DataAssetClass)
{
	return nullptr;
}

void UDalSubsystem::BPListenForDataAsset(TSubclassOf<UDalPrimaryDataAsset> DataAssetClass, const FOnDalDataAssetLoaded& Completed)
{
}

void UDalSubsystem::ListenForDataAssets(const UObject* Owner, const TArray<TSubclassOf<UDalPrimaryDataAsset>>& DataAssetClasses, TFunction<void()>&& Completed)
{
}

void UDalSubsystem::ListenForDataAssetInternal(const UObject* Owner, TSubclassOf<UDalPrimaryDataAsset> DataAssetClass, TFunction<void(const UDalPrimaryDataAsset&)>&& Callback)
{
}

void UDalSubsystem::RegisterDataAsset(TSubclassOf<UDalPrimaryDataAsset> DataAssetClass, const UDalPrimaryDataAsset* DataAsset)
{
}

void UDalSubsystem::UnregisterDataAsset(TSubclassOf<UDalPrimaryDataAsset> DataAssetClass)
{
}

void UDalSubsystem::TryLoadDataAssetsOnce(const UObject* OptionalWorldContext)
{
}

void UDalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UDalSubsystem::Deinitialize()
{
	DataAssetsMap.Reset();
	PendingListenersMap.Reset();
	LoadStreamableHandle.Reset();
	Super::Deinitialize();
}

void UDalSubsystem::OnGameFeatureLoading(const UGameFeatureData* GameFeatureData, const FString& PluginURL)
{
}

void UDalSubsystem::OnGameFeatureUnloading(const UGameFeatureData* GameFeatureData, const FString& PluginURL)
{
}

void UDalSubsystem::OnBeginPlay(UWorld* World, FWorldInitializationValues WorldInitializationValues)
{
}
