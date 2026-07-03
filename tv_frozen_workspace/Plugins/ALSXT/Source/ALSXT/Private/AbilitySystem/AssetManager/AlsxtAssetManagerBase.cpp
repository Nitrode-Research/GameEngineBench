// MIT


#include "AbilitySystem/AssetManager/AlsxtAssetManagerBase.h"
#include "AbilitySystemGlobals.h"


void UAlsxtAssetManagerBase::StartInitialLoading()
{
	Super::StartInitialLoading();
	UAbilitySystemGlobals::Get().InitGlobalData();

	UE_LOG(LogTemp, Display, TEXT("Loading ALSXT Asset Manager"));
}



