// Copyright (c) Yevhenii Selivanov

#include "Subsystems/BmrModularGameFeaturesLoaderSubsystem.h"

// Bomber
#include "DataAssets/BmrModularGameFeatureSettings.h"
#include "MyUtilsLibraries/ModularGameFeaturePluginUtils.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrModularGameFeaturesLoaderSubsystem)

UBmrModularGameFeaturesLoaderSubsystem& UBmrModularGameFeaturesLoaderSubsystem::Get()
{
	UBmrModularGameFeaturesLoaderSubsystem* Subsystem = GetModularGameFeaturesSubsystem();
	checkf(Subsystem, TEXT("ERROR: [%i] %hs:\n'Subsystem' is null!"), __LINE__, __FUNCTION__);
	return *Subsystem;
}

UBmrModularGameFeaturesLoaderSubsystem* UBmrModularGameFeaturesLoaderSubsystem::GetModularGameFeaturesSubsystem()
{
	return GEngine ? GEngine->GetEngineSubsystem<UBmrModularGameFeaturesLoaderSubsystem>() : nullptr;
}

bool UBmrModularGameFeaturesLoaderSubsystem::HasPendingTagDrivenActivations() const
{
	return false;
}

/*********************************************************************************************
 * Tag-Driven Features
 ********************************************************************************************* */

void UBmrModularGameFeaturesLoaderSubsystem::OnWorldASCReady_Implementation(const FGameplayEventData& Payload)
{
}

void UBmrModularGameFeaturesLoaderSubsystem::OnAscTagCountChanged_Implementation(FGameplayTag ChangedTag, int32 NewCount, UAbilitySystemComponent* SourceAsc)
{
}

void UBmrModularGameFeaturesLoaderSubsystem::QueueDeferredRecompute()
{
}

void UBmrModularGameFeaturesLoaderSubsystem::ApplyAuthoritativeFeatureSet()
{
}

bool UBmrModularGameFeaturesLoaderSubsystem::IsAuthoritativeAsc(const UAbilitySystemComponent* ASC) const
{
	return false;
}

/*********************************************************************************************
 * World Lifecycle
 ********************************************************************************************* */

void UBmrModularGameFeaturesLoaderSubsystem::OnPreWorldFinishDestroy_Implementation(UWorld* World)
{
}

#if WITH_EDITOR
void UBmrModularGameFeaturesLoaderSubsystem::OnPreBeginPIE(bool bIsSimulating)
{
}

void UBmrModularGameFeaturesLoaderSubsystem::OnEndPIE(bool bIsSimulating)
{
}

void UBmrModularGameFeaturesLoaderSubsystem::OnEditorShutdownPIE(bool bIsSimulating)
{
}
#endif

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UBmrModularGameFeaturesLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UBmrModularGameFeaturesLoaderSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
