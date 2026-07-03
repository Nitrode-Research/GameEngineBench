// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AdvancedFriendsGameInstance.h"

#include "BmrGameInstance.generated.h"

class APlayerController;

/**
 * The game instance class that is primarily used to manage the online sessions (create, join, destroy etc).
 */
UCLASS()
class BOMBER_API UBmrGameInstance : public UAdvancedFriendsGameInstance
{
	GENERATED_BODY()

	UBmrGameInstance(const FObjectInitializer& ObjectInitializer);

	/*********************************************************************************************
	 * Overrides and Events
	 ********************************************************************************************* */
protected:
	/** Is overridden to listen when first local player is added. */
	virtual int32 AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;

	/** Is called on any level when first local player has had a new outer. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[Bomber]", meta = (BlueprintProtected))
	void OnPlayerControllerReady(APlayerController* PlayerController);

	/** Is called when the startup level is fully loaded and ready. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[Bomber]", meta = (BlueprintProtected))
	void OnStartupLevelReady(APlayerController* PlayerController);

	/*********************************************************************************************
	 * Create online session (server)
	 * Automatically called on game startup on empty startup level
	 * Flow: Create Session first, then open level (not ServerTravel)
	 * Startup Level: OnStartupLevelReady → TryCreateSession → OnCreateSessionComplete → OpenListenServerLevel
	 ********************************************************************************************* */
public:
	/** Attempts to create a session, is also called automatically on game startup on empty startup level. */
	UFUNCTION(BlueprintCallable, Category = "[Bomber]")
	void TryCreateSession(APlayerController* PlayerController);

protected:
	/** Called when server session is created successfully, e.g: when main level is opened. */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/*********************************************************************************************
	 * Join online session (clients)
	 * OnSessionInviteAcceptedMaster/OnSessionInviteReceivedMaster → TryJoinSession → OnDestroySessionComplete →
	 * → JoinSession → auto-ClientTravel (bAutoTravelOnAcceptedUserInviteReceived)
	 ********************************************************************************************* */
public:
	/** Attempts to join a session. */
	UFUNCTION(BlueprintCallable, Category = "[Bomber]")
	void TryJoinSession(const struct FBlueprintSessionResult& SessionToJoin);

protected:
	/** Session callback when a user accepts an invitation. */
	virtual void OnSessionInviteAcceptedMaster(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInviting, const FOnlineSessionSearchResult& SessionToJoin) override;

	/** Session callback when a user receives an invitation. */
	virtual void OnSessionInviteReceivedMaster(const FUniqueNetId& PersonInvited, const FUniqueNetId& PersonInviting, const FString& AppId, const FOnlineSessionSearchResult& SessionToJoin) override;

	/** Called when previous session is destroyed, now can join the new one. */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful, FOnlineSessionSearchResult SessionToJoin) const;
};