// Copyright (c) Yevhenii Selivanov

#include "Subsystems/NMMCameraSubsystem.h"

// NMM
#include "Components/NMMSpotComponent.h"
#include "Data/NMMDataAsset.h"
#include "Data/NmmStateTag.h"
#include "NMMUtils.h"
#include "NmmGameplayTags.h"
#include "Subsystems/NMMBaseSubsystem.h"
#include "Subsystems/NMMInGameSettingsSubsystem.h"
#include "Subsystems/NMMSpotsSubsystem.h"

// Bomber
#include "Components/BmrCameraComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "MyUtilsLibraries/CinematicUtils.h"
#include "MyUtilsLibraries/GameplayUtilsLibrary.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "Camera/CameraActor.h"
#include "CineCameraRigRail.h"
#include "Engine/World.h"
#include "LevelSequencePlayer.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMCameraSubsystem)

UNMMCameraSubsystem& UNMMCameraSubsystem::Get(const UObject* OptionalWorldContext)
{
	UNMMCameraSubsystem* ThisSubsystem = UNMMUtils::GetCameraSubsystem(OptionalWorldContext);
	checkf(ThisSubsystem, TEXT("%s: 'CameraSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

UCameraComponent* UNMMCameraSubsystem::FindCameraComponent(FNmmStateTag MenuState)
{
	return nullptr;
}

ACameraActor* UNMMCameraSubsystem::GetCurrentRailCamera()
{
	return nullptr;
}

ACineCameraRigRail* UNMMCameraSubsystem::GetCurrentRailRig()
{
	return nullptr;
}

void UNMMCameraSubsystem::PossessCamera(FNmmStateTag MenuState)
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UNMMCameraSubsystem::OnGameFeatureInitialize_Implementation()
{
	Super::OnGameFeatureInitialize_Implementation();
}

void UNMMCameraSubsystem::OnGameFeatureDeinitialize_Implementation()
{
	Super::OnGameFeatureDeinitialize_Implementation();
}

void UNMMCameraSubsystem::Tick(float DeltaTime)
{
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UNMMCameraSubsystem::OnNewMainMenuStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMCameraSubsystem::OnActiveMenuSpotReady_Implementation(UNMMSpotComponent* MainMenuSpotComponent)
{
}

/*********************************************************************************************
 * Transitioning
 ********************************************************************************************* */

bool UNMMCameraSubsystem::IsCameraForwardTransition()
{
	return true;
}

float UNMMCameraSubsystem::GetCameraStartTransitionValue()
{
	return 0.f;
}

float UNMMCameraSubsystem::GetCameraLastTransitionValue()
{
	return 0.f;
}

void UNMMCameraSubsystem::SetNewCameraRailTransitionState(ENMMCameraRailTransitionState NewCameraRailState)
{
}

void UNMMCameraSubsystem::OnBeginTransition()
{
}

void UNMMCameraSubsystem::OnEndTransition()
{
}

void UNMMCameraSubsystem::TickTransition(float DeltaTime)
{
}
