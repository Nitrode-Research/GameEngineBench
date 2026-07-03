// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueGameModeBase.h"
#include "ActionRoguelike.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "AI/RogueAICharacter.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Player/RoguePlayerCharacter.h"
#include "Player/RoguePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "RogueMonsterData.h"
#include "ActionRoguelike.h"
#include "RogueDeferredTaskSystem.h"
#include "RogueGameplayFunctionLibrary.h"
#include "RogueGameState.h"
#include "ActionSystem/RogueActionComponent.h"
#include "SaveSystem/RogueSaveGameSubsystem.h"
#include "Development/RogueDeveloperLocalSettings.h"
#include "Engine/AssetManager.h"
#include "Performance/RogueActorPoolingSubsystem.h"
#include "UI/RogueHUD.h"
#include "Windows/WindowsPlatformPerfCounters.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueGameModeBase)



ARogueGameModeBase::ARogueGameModeBase()
{
	PlayerStateClass = ARoguePlayerState::StaticClass();
	HUDClass = ARogueHUD::StaticClass();
	GameStateClass = ARogueGameState::StaticClass();
}


void ARogueGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}


void ARogueGameModeBase::StartPlay()
{
	Super::StartPlay();
}


void ARogueGameModeBase::RequestPrimedActors()
{
}


void ARogueGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}


void ARogueGameModeBase::StartSpawningBots()
{
}


void ARogueGameModeBase::SpawnBotTimerElapsed()
{
}


void ARogueGameModeBase::OnBotSpawnQueryCompleted(TSharedPtr<FEnvQueryResult> Result, FMonsterInfoRow* SelectedRow)
{
}


void ARogueGameModeBase::OnMonsterLoaded(FPrimaryAssetId LoadedId, FVector SpawnLocation)
{
}


void ARogueGameModeBase::OnPowerupSpawnQueryCompleted(TSharedPtr<FEnvQueryResult> Result)
{
}


void ARogueGameModeBase::RespawnPlayerElapsed(AController* Controller)
{
}


void ARogueGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
}
