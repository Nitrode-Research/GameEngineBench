#pragma once

#include "NativeGameplayTags.h"
#include "Utility/ALSXTGameplayTags.h"
#include "Utility/TVStructs.h"
#include "PlayerStateInterface.generated.h"

UINTERFACE(Blueprintable)
class UPlayerStateInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IPlayerStateInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void SetOnlinePlayerID(const FString& ID);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	FString GetOnlinePlayerID();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	FPlayerInfo GetPlayerInfo();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	bool GetPlayerFireTeam(FString& FireTeam);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void SetPlayerFireTeam(FString& FireTeam);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLoadedInLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLoadedInGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnDisconnected();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void StartGame();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void EndGame();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void EndSession();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeGameEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGameEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnSessionEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeGameStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGameStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnUpdateReadyPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnAllPlayersReady();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyLocked();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyUnlocked();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Server State")
	void SetPlayerServerState(FGameplayTag NewPlayerServerState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Server State")
	void GetPlayerServerState(FGameplayTag& PlayerServerState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Player Session State")
	void SetPlayerSessionState(FGameplayTag NewPlayerSessionState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Player Session State")
	void GetPlayerSessionState(FGameplayTag& PlayerSessionState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Server Role")
	void SetPlayerServerRole(FGameplayTag NewPlayerServerRole);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Server Role")
	void GetPlayerServerRole(FGameplayTag& PlayerServerRole);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void GetVitals(float& Health, float& Stamina, float& Armor, float& Thirst, float& Hunger);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void AddReadyPlayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void RemoveReadyPlayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnAllPlayersLoadedInLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnAllPlayersLoadedInGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeTravelToGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeTravelToLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyPreCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeLobbyCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeLobbyPostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyPostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnLobbyCountdownComplete();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGamePreCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeGameCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGameCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnBeforeGamePostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGamePostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State")
	void OnGameCountdownComplete();

	// SERVER LOG

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Server Log")
	void UpdateClientsServerLog();

	//Chat
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	void OnAddChatMessage(FChatMessage ChatMessage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	TArray<FString> GetMutedPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	void MutePlayer(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	void UnMutePlayer(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	bool IsPlayerMuted(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	TArray<FString> GetBlockedPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	void BlockPlayer(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	void UnBlockPlayer(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Chat")
	bool IsPlayerBlocked(const FString& PlayerID);

	// ACTIONS

	// PLAYER ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Actions|Player")
	void AddPlayerActionClient(FPlayerAction PlayerAction);

	// FIRETEAM ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Actions|Fireteam")
	void AddFireteamActionClient(FFireteamAction FireteamAction);

	// SECTION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Actions|Section")
	void AddSectionActionClient(FSectionAction SectionAction);

	// ORGANIZATION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Actions|Organization")
	void AddOrganizationActionClient(FOrganizationAction OrganizationAction);

	//Respawn

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Respawn")
	void StartRespawnCooldown();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Respawn")
	void StopRespawnCooldown();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Respawn")
	float GetRespawnCooldownLength();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Respawn")
	float GetRespawnCooldownValue();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Respawn")
	void OnRespawnCooldownComplete();
};