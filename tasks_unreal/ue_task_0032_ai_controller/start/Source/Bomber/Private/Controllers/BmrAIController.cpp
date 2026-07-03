// Copyright (c) Yevhenii Selivanov.

#include "Controllers/BmrAIController.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrMoverComponent.h"
#include "DataAssets/BmrAIDataAsset.h"
#include "DataAssets/BmrGameStateDataAsset.h"
#include "GameFramework/BmrCheatManager.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrPawnReadySubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

#if WITH_EDITOR
#include "BmrUnrealEdEngine.h"
#endif

// UE
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrAIController)

ABmrAIController::ABmrAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bAttachToPawn = true;
}

void ABmrAIController::MoveToCell(const FBmrCell& DestinationCell)
{
	ABmrPawn* InOwner = GetPawn<ABmrPawn>();
	const UBmrMapComponent* MapComponent = UBmrMapComponent::GetMapComponent(InOwner);
	UBmrMoverComponent* MoverComponent = InOwner ? InOwner->GetMoverComponent() : nullptr;
	if (!MapComponent
	    || !MoverComponent)
	{
		return;
	}

	if (!MoverComponent->IsBlockedMovement())
	{
		const FBmrCell& CurrentCell = MapComponent->GetCell();
		const bool bHasArrived = CurrentCell == DestinationCell;
		LastMoveToCell = bHasArrived ? FBmrCell::InvalidCell : DestinationCell;

		const FVector Direction = bHasArrived ? FVector::ZeroVector : (DestinationCell.Location - InOwner->GetActorLocation()).GetSafeNormal2D();
		MoverComponent->RequestMoveByIntent(Direction);
	}
}

bool ABmrAIController::IsAIEnabled() const
{
	const ABmrPawn* InOwner = GetPawn<ABmrPawn>();
	const UBmrMoverComponent* MoverComponent = InOwner ? InOwner->GetMoverComponent() : nullptr;
	return MoverComponent
	       && !MoverComponent->IsBlockedMovement()
	       && UBmrCheatManager::CVarAISetEnabled.GetValueOnAnyThread();
}

/* ---------------------------------------------------
 *					Protected functions
 * --------------------------------------------------- */

void ABmrAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ABmrAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ABmrAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void ABmrAIController::Reset()
{
	Super::Reset();
}

void ABmrAIController::UpdateAI()
{
}

void ABmrAIController::SetAI(bool bShouldEnable)
{
	const ABmrPawn* InOwner = GetPawn<ABmrPawn>();
	const bool bWantsEnableDeadAI = !InOwner && bShouldEnable;
	if (bWantsEnableDeadAI
	    || !HasAuthority())
	{
		return;
	}

	Reset();

	UBmrMoverComponent* MoverComponent = InOwner ? InOwner->GetMoverComponent() : nullptr;
	if (MoverComponent)
	{
		MoverComponent->SetBlockMovement(!bShouldEnable);
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void ABmrAIController::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void ABmrAIController::OnPostRemovedFromLevel_Implementation(UBmrMapComponent* MapComponent, UObject* DestroyCauser)
{
}

void ABmrAIController::OnOwnerMovementCompleted_Implementation(const FMoverTimeStep& TimeStep)
{
}
