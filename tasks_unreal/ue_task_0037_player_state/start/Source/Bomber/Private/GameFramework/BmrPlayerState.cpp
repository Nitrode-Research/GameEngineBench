// Copyright (c) Yevhenii Selivanov

#include "GameFramework/BmrPlayerState.h"

// Bomber
#include "AbilitySystem/Attributes/BmrHealthAttributeSet.h"
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrGeneratedMap.h"
#include "Actors/BmrPawn.h"
#include "AdvancedIdentityLibrary.h"
#include "AdvancedSteamFriendsLibrary.h"
#include "Components/BmrMapComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "GameFramework/BmrGameMode.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrGameUserSettings.h"
#include "MyUtilsLibraries/MultiplayerUtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrPawnReadySubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrActorUtilsLibrary.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"
#include "GameplayAbilitiesModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPlayerState)

ABmrPlayerState::ABmrPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	PowerupsSet = CreateDefaultSubobject<UBmrPowerupsAttributeSet>(TEXT("PowerupsAttributeSet"));
	HealthSet = CreateDefaultSubobject<UBmrHealthAttributeSet>(TEXT("HealthAttributeSet"));

	SetPlayerId(INDEX_NONE);
}

bool ABmrPlayerState::IsPlayerStateLocallyControlled() const
{
	const APlayerController* PC = GetPlayerController();
	return PC && PC->IsLocalPlayerController();
}

ABmrPawn& ABmrPlayerState::GetPawnChecked() const
{
	ABmrPawn* Pawn = GetPawn<ABmrPawn>();
	checkf(Pawn, TEXT("ERROR: [%i] %hs:\n'Pawn' is null!"), __LINE__, __FUNCTION__);
	return *Pawn;
}

/*********************************************************************************************
 * Gameplay Ability System (GAS)
 ********************************************************************************************* */

UAbilitySystemComponent& ABmrPlayerState::GetAbilitySystemComponentChecked() const
{
	checkf(AbilitySystemComponent, TEXT("ERROR: [%i] %hs:\n'AbilitySystemComponent' is null!"), __LINE__, __FUNCTION__);
	return *AbilitySystemComponent;
}

void ABmrPlayerState::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
}

void ABmrPlayerState::ApplyDefaultAttributes()
{
}

/*********************************************************************************************
 * Chosen Mesh Data
 ********************************************************************************************* */

const FBmrPlayerTag& ABmrPlayerState::GetPlayerTag() const
{
	return FBmrPlayerTag::None;
}

void ABmrPlayerState::SetChosenMeshData(const FBmrMeshData& InMeshData)
{
}

void ABmrPlayerState::ServerSetChosenMeshData_Implementation(const FBmrMeshData& InMeshData)
{
}

void ABmrPlayerState::OnRep_ChosenMeshData()
{
}

void ABmrPlayerState::ApplyChosenMeshData()
{
}

/*********************************************************************************************
 * End Game State
 ********************************************************************************************* */

void ABmrPlayerState::UpdateEndGameState()
{
}

void ABmrPlayerState::SetEndGameState(EBmrEndGameState NewEndGameState)
{
}

void ABmrPlayerState::OnRep_EndGameState()
{
}

void ABmrPlayerState::ApplyEndGameState()
{
}

/*********************************************************************************************
 * Nickname
 ********************************************************************************************* */

FName ABmrPlayerState::GetPendingPlayerName() const
{
	return FName(*GetOldPlayerName());
}

void ABmrPlayerState::SetSavedPlayerName(FName NewName)
{
}

void ABmrPlayerState::ServerSetPlayerName_Implementation(FName NewName)
{
}

void ABmrPlayerState::SetDefaultPlayerName()
{
}

void ABmrPlayerState::SetPlayerName(const FString& NewPlayerName)
{
	Super::SetPlayerName(NewPlayerName);
}

void ABmrPlayerState::ApplyPlayerName()
{
}

void ABmrPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
}

/*********************************************************************************************
 * Is Player Dead
 ********************************************************************************************* */

void ABmrPlayerState::SetPlayerDead(bool bIsDead)
{
}

void ABmrPlayerState::OnRep_IsPlayerDead()
{
}

void ABmrPlayerState::ApplyIsPlayerDead()
{
}

void ABmrPlayerState::OnPostPlayerDead_Implementation()
{
}

/*********************************************************************************************
 * Killed Opponents Num
 ********************************************************************************************* */

void ABmrPlayerState::SetOpponentKilled(const ABmrPawn* DefeatedPlayer)
{
}

void ABmrPlayerState::SetOpponentKilledNum(int32 NewOpponentsKilledNum)
{
}

void ABmrPlayerState::OnRep_OpponentsKilledNum()
{
}

void ABmrPlayerState::ApplyOpponentsKilledNum()
{
}

/*********************************************************************************************
 * Is Human / Bot
 ********************************************************************************************* */

void ABmrPlayerState::SetIsABot()
{
}

void ABmrPlayerState::SetIsHuman()
{
}

void ABmrPlayerState::OnRep_IsABot()
{
}

void ABmrPlayerState::ApplyIsABot()
{
}

/*********************************************************************************************
 * Player ID
 ********************************************************************************************* */

void ABmrPlayerState::SetHumanId(APlayerController* PlayerController)
{
}

void ABmrPlayerState::SetDefaultBotId()
{
}

void ABmrPlayerState::OnRep_PlayerId()
{
	Super::OnRep_PlayerId();
}

void ABmrPlayerState::ApplyPlayerId()
{
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void ABmrPlayerState::OnPlayerStateInit_Implementation()
{
}

void ABmrPlayerState::OnPawnChanged_Implementation(APawn* NewPawn)
{
}

void ABmrPlayerState::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void ABmrPlayerState::OnSaveSettings_Implementation()
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void ABmrPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABmrPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABmrPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ABmrPlayerState::OnDeactivated()
{
	Super::OnDeactivated();
}

void ABmrPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	Super::RegisterPlayerWithSession(bWasFromInvite);
}

void ABmrPlayerState::UnregisterPlayerWithSession()
{
	Super::UnregisterPlayerWithSession();
}

void ABmrPlayerState::PostRepNotifies()
{
	Super::PostRepNotifies();
}
