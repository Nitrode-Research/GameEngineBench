// Copyright (c) Yevhenii Selivanov

#include "Components/BmrMoverComponent.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrGeneratedMap.h"
#include "Actors/BmrPawn.h"
#include "Components/BmrMapComponent.h"
#include "DataAssets/BmrGeneratedMapDataAsset.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Structures/BmrMoverSyncState.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrMoverComponent)

void UBmrMoverComponent::RequestMoveByIntent(const FVector& Direction)
{
}

void UBmrMoverComponent::TeleportToLocation(const FVector& InLocation)
{
}

void UBmrMoverComponent::SetBlockMovement(bool bShouldBlock)
{
}

bool UBmrMoverComponent::IsBlockedMovement() const
{
	return false;
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void UBmrMoverComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBmrMoverComponent::ProduceInput(const int32 DeltaTimeMS, FMoverInputCmdContext* Cmd)
{
	Super::ProduceInput(DeltaTimeMS, Cmd);
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void UBmrMoverComponent::OnPawnReady_Implementation(const FGameplayEventData& Payload)
{
}

void UBmrMoverComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UBmrMoverComponent::OnPreRemovedFromLevel_Implementation(UBmrMapComponent* MapComponent, UObject* DestroyCauser)
{
}

void UBmrMoverComponent::OnControllerChanged_Implementation(APawn* Pawn, AController* OldController, AController* NewController)
{
}

void UBmrMoverComponent::OnPostMove_Implementation(const FMoverTimeStep& TimeStep, FMoverSyncState& SyncState, FMoverAuxStateContext& AuxState)
{
}

void UBmrMoverComponent::OnMoveInputTriggered_Implementation(const FInputActionValue& ActionValue)
{
}

void UBmrMoverComponent::OnMoveInputCompleted_Implementation(const FInputActionValue& ActionValue)
{
}

void UBmrMoverComponent::OnSkateAttributeChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
}
