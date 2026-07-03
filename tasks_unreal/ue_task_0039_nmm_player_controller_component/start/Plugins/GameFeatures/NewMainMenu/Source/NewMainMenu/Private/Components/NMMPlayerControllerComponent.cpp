// Copyright (c) Yevhenii Selivanov

#include "Components/NMMPlayerControllerComponent.h"

// NMM
#include "Components/NMMSpotComponent.h"
#include "Data/NMMDataAsset.h"
#include "Data/NMMSaveGameData.h"
#include "Data/NmmStateTag.h"
#include "NMMUtils.h"
#include "NmmGameplayTags.h"

// Bomber
#include "Actors/BmrPawn.h"
#include "Components/BmrCameraComponent.h"
#include "Components/BmrMouseActivityComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "DalSubsystem.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "GameFramework/BmrGameState.h"
#include "MyUtilsLibraries/InputUtilsLibrary.h"
#include "MyUtilsLibraries/SaveUtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrSoundsSubsystem.h"
#include "Subsystems/BmrWidgetsSubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMPlayerControllerComponent)

UNMMPlayerControllerComponent::UNMMPlayerControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

ABmrPlayerController* UNMMPlayerControllerComponent::GetPlayerController() const
{
	return nullptr;
}

ABmrPlayerController& UNMMPlayerControllerComponent::GetPlayerControllerChecked() const
{
	ABmrPlayerController* PC = GetPlayerController();
	checkf(PC, TEXT("'PC' is null"));
	return *PC;
}

/*********************************************************************************************
 * Main methods
 ********************************************************************************************* */

void UNMMPlayerControllerComponent::SetSaveGameData(USaveGame* NewSaveGameData)
{
}

void UNMMPlayerControllerComponent::SetCinematicInputContextEnabled(bool bEnable)
{
}

void UNMMPlayerControllerComponent::SetCinematicMouseVisibilityEnabled(bool bEnabled)
{
}

void UNMMPlayerControllerComponent::SetManagedInputContextsEnabled(FNmmStateTag NewMenuState)
{
}

/*********************************************************************************************
 * Sounds
 ********************************************************************************************* */

void UNMMPlayerControllerComponent::PlayMainMenuMusic()
{
}

void UNMMPlayerControllerComponent::StopMainMenuMusic()
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UNMMPlayerControllerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNMMPlayerControllerComponent::OnUnregister()
{
	Super::OnUnregister();
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UNMMPlayerControllerComponent::OnDataAssetLoaded_Implementation(const UNMMDataAsset* DataAsset)
{
}

void UNMMPlayerControllerComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMPlayerControllerComponent::OnNewMainMenuStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMPlayerControllerComponent::OnWidgetsInitialized_Implementation()
{
}

void UNMMPlayerControllerComponent::OnAsyncLoadGameFromSlotCompleted_Implementation(USaveGame* SaveGame)
{
}
