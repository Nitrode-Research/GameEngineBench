#pragma once

#include "PlayerControllerInterface.generated.h"

UINTERFACE(Blueprintable)
class UPlayerControllerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IPlayerControllerInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller")
	void OnLoadedInLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller")
	void OnLoadedInGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller")
	void OnDisconnected();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void OnAllPlayersLoadedInLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void OnUpdateReadyPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void OnAllPlayersReady();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void OnLobbyLocked();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void OnLobbyUnlocked();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Lobby")
	void GetTogglePlayerReadyTimeElapsed(float& TimeElaped);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void TravelFromLobbyToGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void TravelFromGameLevelToLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeTravelToGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeTravelToLobbyLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnAllPlayersLoadedInGameLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Camera")
	void CameraComponentEnableCountdownRotation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Camera")
	void CameraComponentStopRotation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Debug")
	void DisplayDebugUI(bool& Display);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnLobbyPreCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeLobbyCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnLobbyCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeLobbyPostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnLobbyPostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnLobbyCountdownComplete();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGamePreCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeGameCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGameCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeGamePostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGamePostCountdownStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGameCountdownComplete();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeGameStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGameStart();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Start Game")
	void EndGame();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Start Game")
	void EndSession();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnBeforeGameEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnGameEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Game Start")
	void OnSessionEnd();

	// SERVER LOG

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Server Log")
	void UpdateClientsServerLog();

	//Chat
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Chat|Player Settings")
	bool IsVoiceChatEnabled();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Chat|Player Settings")
	bool IsWhisperChannelEnabled();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	void SetChatChannel(UPARAM(meta = (Categories = "Game.Simple Comms Channel"))const FGameplayTag& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	UPARAM(meta = (Categories = "Game.Simple Comms Channel")) FGameplayTag GetCurrentChatChannel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Chat")
	void GetCurrentChannelChatSettings(const FGameplayTag& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	TArray<FString> GetChatWhisperChannels();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	void SetChatWhisperChannel(const FString& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	FString GetChatWhisperChannel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	void PreviousChatChannel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	void NextChatChannel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Chat")
	void OnAddChatMessage(FChatMessage ChatMessage);

	// ACTIONS

	// PLAYER ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Actions|Player")
	void AddPlayerActionClient(FPlayerAction PlayerAction);

	// FIRETEAM ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Actions|Fireteam")
	void AddFireteamActionClient(FFireteamAction FireteamAction);

	// SECTION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Actions|Section")
	void AddSectionActionClient(FSectionAction SectionAction);

	// ORGANIZATION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Controller|Actions|Organization")
	void AddOrganizationActionClient(FOrganizationAction OrganizationAction);
};