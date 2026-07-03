// Copyright (c) Yevhenii Selivanov

#include "Subsystems/NMMSpotsSubsystem.h"

// NMM
#include "Components/NMMSpotComponent.h"
#include "DalRegistrySubsystem.h"
#include "NMMUtils.h"
#include "NmmGameplayTags.h"
#include "Subsystems/NMMBaseSubsystem.h"
#include "Subsystems/NMMInGameSettingsSubsystem.h"

// Bomber
#include "DataRegistries/BmrCinematicRow.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMSpotsSubsystem)

UNMMSpotsSubsystem& UNMMSpotsSubsystem::Get(const UObject* OptionalWorldContext)
{
	UNMMSpotsSubsystem* ThisSubsystem = UNMMUtils::GetSpotsSubsystem(OptionalWorldContext);
	checkf(ThisSubsystem, TEXT("%s: 'NMMSpotsSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

bool UNMMSpotsSubsystem::IsActiveMenuSpotReady() const
{
	return false;
}

void UNMMSpotsSubsystem::AddNewMainMenuSpot(UNMMSpotComponent* NewMainMenuSpotComponent)
{
}

void UNMMSpotsSubsystem::ReinitializeAllSpots()
{
}

void UNMMSpotsSubsystem::NotifySpotLoaded(UNMMSpotComponent* SpotComponent)
{
}

bool UNMMSpotsSubsystem::AreAllSpotsLoaded() const
{
	return false;
}

void UNMMSpotsSubsystem::RemoveMainMenuSpot(UNMMSpotComponent* MainMenuSpotComponent)
{
}

UNMMSpotComponent* UNMMSpotsSubsystem::GetCurrentSpot() const
{
	return nullptr;
}

void UNMMSpotsSubsystem::GetMainMenuSpots(TArray<UNMMSpotComponent*>& OutSpots) const
{
	OutSpots.Reset();
}

UNMMSpotComponent* UNMMSpotsSubsystem::GetNextSpot(int32 Incrementer) const
{
	return nullptr;
}

UNMMSpotComponent* UNMMSpotsSubsystem::MoveMainMenuSpot(int32 Incrementer)
{
	return nullptr;
}

UNMMSpotComponent* UNMMSpotsSubsystem::MoveMainMenuSpotByPredicate(int32 Incrementer, const TFunctionRef<bool(UNMMSpotComponent*)>& Predicate)
{
	return nullptr;
}

void UNMMSpotsSubsystem::TryBroadcastOnActiveMenuSpotReady()
{
}

void UNMMSpotsSubsystem::HandleUnavailableMenuSpot()
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UNMMSpotsSubsystem::OnGameFeatureInitialize_Implementation()
{
	Super::OnGameFeatureInitialize_Implementation();
}

void UNMMSpotsSubsystem::OnGameFeatureDeinitialize_Implementation()
{
	Super::OnGameFeatureDeinitialize_Implementation();
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UNMMSpotsSubsystem::OnNewMainMenuStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMSpotsSubsystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMSpotsSubsystem::OnCinematicRowsChanged_Implementation()
{
}
