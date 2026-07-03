// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_SpawnActors.h"
#include "LogModularFeatures_ExtraActions.h"
#include "Components/GameFrameworkComponentManager.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_SpawnActors)
#endif

void UGameFeatureAction_SpawnActors::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	ResetExtension();
}

void UGameFeatureAction_SpawnActors::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
	ResetExtension();
}

void UGameFeatureAction_SpawnActors::ResetExtension()
{
	DestroyActors();
}

void UGameFeatureAction_SpawnActors::OnWorldInitialized(UWorld* World, [[maybe_unused]] const UWorld::InitializationValues)
{
	AddToWorld(World);
}

void UGameFeatureAction_SpawnActors::AddToWorld(UWorld* World)
{
}

void UGameFeatureAction_SpawnActors::SpawnActors(UWorld* WorldReference)
{
}

void UGameFeatureAction_SpawnActors::DestroyActors()
{
	SpawnedActors.Empty();
}
