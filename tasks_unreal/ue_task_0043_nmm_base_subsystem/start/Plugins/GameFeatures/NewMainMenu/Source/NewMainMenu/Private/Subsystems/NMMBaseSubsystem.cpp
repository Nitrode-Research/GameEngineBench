// Copyright (c) Yevhenii Selivanov

#include "Subsystems/NMMBaseSubsystem.h"

// NMM
#include "NMMUtils.h"
#include "NmmGameplayTags.h"
#include "Subsystems/NMMSpotsSubsystem.h"

// Bomber
#include "Controllers/BmrPlayerController.h"
#include "DataRegistries/BmrCinematicRow.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrModularGameFeaturesLoaderSubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMBaseSubsystem)

UNMMBaseSubsystem& UNMMBaseSubsystem::Get(const UObject* OptionalWorldContext /* = nullptr*/)
{
	UNMMBaseSubsystem* ThisSubsystem = UNMMUtils::GetBaseSubsystem(OptionalWorldContext);
	checkf(ThisSubsystem, TEXT("%s: 'SoundsSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

/*********************************************************************************************
 * New Main Menu State
 ********************************************************************************************* */

FNmmStateTag UNMMBaseSubsystem::GetPredictedMenuState() const
{
	return FNmmStateTag::None;
}

void UNMMBaseSubsystem::TryBroadcastMenuReady()
{
}

void UNMMBaseSubsystem::SetNewMainMenuState(FNmmStateTag NewState)
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UNMMBaseSubsystem::OnGameFeatureInitialize_Implementation()
{
}

void UNMMBaseSubsystem::OnGameFeatureDeinitialize_Implementation()
{
}

void UNMMBaseSubsystem::OnGameFeatureActivated(const UGameFeatureData* /*GameFeatureData*/, const FString& /*PluginURL*/)
{
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UNMMBaseSubsystem::OnFirstPawnReady_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMBaseSubsystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMBaseSubsystem::OnActiveMenuSpotReady_Implementation(UNMMSpotComponent* /*MainMenuSpotComponent*/)
{
}
