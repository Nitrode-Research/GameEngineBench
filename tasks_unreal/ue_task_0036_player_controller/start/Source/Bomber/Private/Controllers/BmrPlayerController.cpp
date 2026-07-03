// Copyright (c) Yevhenii Selivanov.

#include "Controllers/BmrPlayerController.h"

// Bomber
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "Components/BmrMouseActivityComponent.h"
#include "Components/BmrMoverComponent.h"
#include "DalSubsystem.h"
#include "DataAssets/BmrInputAction.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "DataAssets/BmrPlayerInputDataAsset.h"
#include "FunctionPickerData/FunctionPickerTemplate.h"
#include "GameFramework/BmrCheatManager.h"
#include "GameFramework/BmrGameMode.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "MyUtilsLibraries/InputUtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrPawnReadySubsystem.h"
#include "Subsystems/BmrWidgetsSubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UI/SettingsWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/NavigationConfig.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPlayerController)

ABmrPlayerController::ABmrPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAutoManageActiveCameraTarget = false;
	CheatClass = UBmrCheatManager::StaticClass();
	MouseComponent = CreateDefaultSubobject<UBmrMouseActivityComponent>(TEXT("MouseActivityComponent"));
	bAttachToPawn = true;
}

void ABmrPlayerController::ServerBroadcastMessage_Implementation(const FGameplayEventData& Payload)
{
	UGlobalMessageSubsystem::BroadcastGlobalMessage(Payload);
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void ABmrPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABmrPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ABmrPlayerController::InitInputSystem()
{
	Super::InitInputSystem();
}

void ABmrPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ABmrPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
}

void ABmrPlayerController::InitPlayerState()
{
}

void ABmrPlayerController::PawnLeavingGame()
{
}

void ABmrPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);
}

void ABmrPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void ABmrPlayerController::OnPlayerInputDataAssetLoaded_Implementation(const UBmrPlayerInputDataAsset* DataAsset)
{
}

void ABmrPlayerController::OnWidgetsInitialized_Implementation()
{
}

void ABmrPlayerController::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void ABmrPlayerController::OnToggledSettings_Implementation(bool bIsVisible)
{
}

/*********************************************************************************************
 * Inputs management
 ********************************************************************************************* */

bool ABmrPlayerController::CanBindInputActions() const
{
	return false;
}

void ABmrPlayerController::SetupInputContexts(const TArray<UBmrInputMappingContext*>& InputContexts)
{
}

void ABmrPlayerController::SetupInputContexts(const TArray<const UBmrInputMappingContext*>& InputContexts)
{
}

void ABmrPlayerController::RemoveInputContexts(const TArray<UBmrInputMappingContext*>& InputContexts)
{
}

void ABmrPlayerController::SetUIInputIgnored()
{
}

void ABmrPlayerController::SetAllInputContextsEnabled(bool bEnable, FBmrGameStateTag GameStateTag)
{
}

void ABmrPlayerController::ApplyAllInputContexts()
{
}

void ABmrPlayerController::SetInputContextEnabled(bool bEnable, const UBmrInputMappingContext* InInputContext)
{
}

void ABmrPlayerController::SetAllInputContextEnabled(bool bEnable, const TArray<UBmrInputMappingContext*>& InInputContexts)
{
}

void ABmrPlayerController::BindInputActionsInContext(const UBmrInputMappingContext* InInputContext)
{
}

void ABmrPlayerController::AddNewInputContexts(const TArray<const UBmrInputMappingContext*>& InputContexts)
{
}

UBmrMouseActivityComponent& ABmrPlayerController::GetMouseActivityComponentChecked() const
{
	checkf(MouseComponent, TEXT("'MouseComponent' is null"));
	return *MouseComponent;
}

/*********************************************************************************************
 * Camera
 ********************************************************************************************* */

void ABmrPlayerController::SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
	Super::SetViewTarget(NewViewTarget, TransitionParams);
}

void ABmrPlayerController::SpawnPlayerCameraManager()
{
	Super::SpawnPlayerCameraManager();
}

void ABmrPlayerController::GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const
{
	Super::GetPlayerViewPoint(Location, Rotation);
}

#if WITH_EDITOR
void ABmrPlayerController::OnPreSwitchBeginPIEAndSIE(bool bIsPIE)
{
}
#endif // WITH_EDITOR
