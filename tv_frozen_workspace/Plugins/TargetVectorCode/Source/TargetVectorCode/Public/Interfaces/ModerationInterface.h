#pragma once

#include "NativeGameplayTags.h"
#include "Utility/TVModerationStructs.h"
#include "ModerationInterface.generated.h"

UINTERFACE(Blueprintable)
class UModerationInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IModerationInterface {
	GENERATED_BODY()

public:
	// Moderation

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	TArray<FString> GetAdmins();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void AddAdmin(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void RemoveAdmin(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	bool IsPlayerAdmin(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void ResetAdmins();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	TArray<FString> GetModerators();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void AddModerator(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void RemoveModerator(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void ResetModerators();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	bool IsPlayerModerator(const FString& PlayerID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	TArray<FString> GetServerMutedPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	TArray<FString> GetKickedPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	TArray<FString> GetServerBannedPlayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	bool IsPlayerGloballyBanned();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void KickPlayer(const FString& PlayerID, FDateTime KickExpiration, const FText& Reason);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void UnKickPlayer(const FString& PlayerID, const FText& Reason);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void BanPlayer(const FString& PlayerID, const FText& Reason);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void UnBanPlayer(const FString& PlayerID, const FText& Reason);

	// Submit a request to the EOS Backend to globally ban a player
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void RequestBanPlayerGlobal(const FString& PlayerID, const FText& Reason, const FString& ReportID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void RequestUnBanPlayerGlobal(const FString& PlayerID, const FText& Reason, const FString& ReportID);

	// Profanity
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void AddPlayerProfanityRecord(FPlayerProfanityRecord NewPlayerProfanityRecord);

	// Team Killing

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void GetTeamKillSettings();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	bool IsTeamKill(const FString& InstigatorID, const FString& TargetID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Moderation")
	void AddTeamKillRecord(const FString& NewTeamKillRecord);

};