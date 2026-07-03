// Copyright (c) Yevhenii Selivanov

#include "Components/NMMSpotComponent.h"

// NMM
#include "Data/NMMDataAsset.h"
#include "Data/NMMSaveGameData.h"
#include "NMMUtils.h"
#include "NmmGameplayTags.h"
#include "Subsystems/NMMCameraSubsystem.h"
#include "Subsystems/NMMSpotsSubsystem.h"

// Bomber
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "Controllers/BmrPlayerController.h"
#include "DalSubsystem.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "DataRegistries/BmrPlayerSkinRow.h"
#include "GameFramework/BmrPlayerState.h"
#include "MyUtilsLibraries/CinematicUtils.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "LevelSequencePlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMSpotComponent)

UNMMSpotComponent::UNMMSpotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetActiveFlag(true);
}

bool UNMMSpotComponent::IsCurrentSpot() const
{
	return UNMMSpotsSubsystem::Get().GetCurrentSpot() == this;
}

bool UNMMSpotComponent::IsSpotAvailable() const
{
	const UBmrSkeletalMeshComponent* MeshComponent = GetMeshComponent();
	return IsActive()
	       && MasterPlayer
	       && MeshComponent
	       && MeshComponent->IsActive()
	       && MeshComponent->IsVisible();
}

bool UNMMSpotComponent::IsSpotSkinAvailable() const
{
	const UBmrSkeletalMeshComponent* MeshComponent = GetMeshComponent();
	return MeshComponent && MeshComponent->IsSkinAvailable(MeshComponent->GetAppliedSkinIndex());
}

UBmrSkeletalMeshComponent* UNMMSpotComponent::GetMeshComponent() const
{
	return GetOwner()->FindComponentByClass<UBmrSkeletalMeshComponent>();
}

UBmrSkeletalMeshComponent& UNMMSpotComponent::GetMeshChecked() const
{
	UBmrSkeletalMeshComponent* Mesh = GetMeshComponent();
	checkf(Mesh, TEXT("'Mesh' is nullptr, can not get mesh for '%s' spot."), *GetNameSafe(this));
	return *Mesh;
}

ABmrSkeletalMeshActor& UNMMSpotComponent::GetOwnerChecked() const
{
	return *CastChecked<ABmrSkeletalMeshActor>(GetOwner());
}

void UNMMSpotComponent::ApplyMeshOnPlayer()
{
}

/*********************************************************************************************
 * Cinematics
 ********************************************************************************************* */

ULevelSequence* UNMMSpotComponent::GetMasterSequence() const
{
	return MasterPlayer ? Cast<ULevelSequence>(MasterPlayer->GetSequence()) : nullptr;
}

void UNMMSpotComponent::ReinitializeCinematicData()
{
}

void UNMMSpotComponent::StopMasterSequence()
{
}

bool UNMMSpotComponent::CanChangeCinematicState(FNmmStateTag NewMenuState) const
{
	return false;
}

void UNMMSpotComponent::SetCinematicByState(FNmmStateTag MenuState)
{
}

void UNMMSpotComponent::InitMasterSequencePlayer()
{
}

/*********************************************************************************************
 * Protected functions
 ********************************************************************************************* */

void UNMMSpotComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNMMSpotComponent::OnUnregister()
{
	Super::OnUnregister();
}

void UNMMSpotComponent::UpdateCinematicData()
{
}

void UNMMSpotComponent::MarkCinematicAsSeen()
{
}

void UNMMSpotComponent::ApplyCinematicState()
{
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UNMMSpotComponent::OnDataAssetLoaded_Implementation(const UNMMDataAsset* DataAsset)
{
}

void UNMMSpotComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMSpotComponent::OnNewMainMenuStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UNMMSpotComponent::OnMasterSequencePaused_Implementation()
{
}

void UNMMSpotComponent::OnCameraRailTransitionStateChanged_Implementation(ENMMCameraRailTransitionState CameraRailTransitionStateChanged)
{
}
