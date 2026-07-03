// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonSessionSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CommonSessionSubsystem)

#if COMMONUSER_OSSV1
#include "OnlineSubsystem.h"
#include "Online.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"

#ifndef SEARCH_PRESENCE
#define SEARCH_PRESENCE FName(TEXT("PRESENCESEARCH"))
#endif

FName SETTING_ONLINESUBSYSTEM_VERSION(TEXT("OSSv1"));
#else
#include "Online/OnlineSessionNames.h"
#include "Online/OnlineServicesEngineUtils.h"

FName SETTING_ONLINESUBSYSTEM_VERSION(TEXT("OSSv2"));
using namespace UE::Online;
#endif // COMMONUSER_OSSV1


DECLARE_LOG_CATEGORY_EXTERN(LogCommonSession, Log, All);
DEFINE_LOG_CATEGORY(LogCommonSession);

#define LOCTEXT_NAMESPACE "CommonUser"

//////////////////////////////////////////////////////////////////////
//UCommonSession_SearchSessionRequest

void UCommonSession_SearchSessionRequest::NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



//////////////////////////////////////////////////////////////////////
//UCommonSession_SearchResult

#if COMMONUSER_OSSV1
FString UCommonSession_SearchResult::GetDescription() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonSession_SearchResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSession_SearchResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UCommonSession_SearchResult::GetNumOpenPrivateConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetNumOpenPublicConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetMaxPublicConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetPingInMs() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

#else
FString UCommonSession_SearchResult::GetDescription() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonSession_SearchResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSession_SearchResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UCommonSession_SearchResult::GetNumOpenPrivateConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetNumOpenPublicConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetMaxPublicConnections() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_SearchResult::GetPingInMs() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

#endif //COMMONUSER_OSSV1


class FCommonOnlineSearchSettingsBase : public FGCObject
{
public:
	FCommonOnlineSearchSettingsBase(UCommonSession_SearchSessionRequest* InSearchRequest)
	{
		SearchRequest = InSearchRequest;
	}

	virtual ~FCommonOnlineSearchSettingsBase() {}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SearchRequest);
	}

	virtual FString GetReferencerName() const override
	{
		static const FString NameString = TEXT("FCommonOnlineSearchSettings");
		return NameString;
	}

public:
	UCommonSession_SearchSessionRequest* SearchRequest = nullptr;
};

#if COMMONUSER_OSSV1
//////////////////////////////////////////////////////////////////////
// FCommonSession_OnlineSessionSettings

class FCommonSession_OnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FCommonSession_OnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4)
	{
		NumPublicConnections = MaxNumPlayers;
		if (NumPublicConnections < 0)
		{
			NumPublicConnections = 0;
		}
		NumPrivateConnections = 0;
		bIsLANMatch = bIsLAN;
		bShouldAdvertise = true;
		bAllowJoinInProgress = true;
		bAllowInvites = true;
		bUsesPresence = bIsPresence;
		bAllowJoinViaPresence = true;
		bAllowJoinViaPresenceFriendsOnly = false;
	}

	virtual ~FCommonSession_OnlineSessionSettings() {}
};

//////////////////////////////////////////////////////////////////////
// FCommonOnlineSearchSettingsOSSv1

class FCommonOnlineSearchSettingsOSSv1 : public FOnlineSessionSearch, public FCommonOnlineSearchSettingsBase
{
public:
	FCommonOnlineSearchSettingsOSSv1(UCommonSession_SearchSessionRequest* InSearchRequest)
		: FCommonOnlineSearchSettingsBase(InSearchRequest)
	{
		bIsLanQuery = (InSearchRequest->OnlineMode == ECommonSessionOnlineMode::LAN);
		MaxSearchResults = 10;
		PingBucketSize = 50;

		QuerySettings.Set(SETTING_ONLINESUBSYSTEM_VERSION, true, EOnlineComparisonOp::Equals);
		if (InSearchRequest->bUseLobbies)
		{
			QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
			QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
		}
	}

	virtual ~FCommonOnlineSearchSettingsOSSv1() {}
};
#else

class FCommonOnlineSearchSettingsOSSv2 : public FCommonOnlineSearchSettingsBase
{
public:
	FCommonOnlineSearchSettingsOSSv2(UCommonSession_SearchSessionRequest* InSearchRequest)
		: FCommonOnlineSearchSettingsBase(InSearchRequest)
	{
		FindLobbyParams.MaxResults = 10;

		FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{ SETTING_ONLINESUBSYSTEM_VERSION, ESchemaAttributeComparisonOp::Equals, true });

		if (InSearchRequest->bUseLobbies)
		{
			FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{ SEARCH_PRESENCE, ESchemaAttributeComparisonOp::Equals, true });
		}
	}
public:
	FFindLobbies::Params FindLobbyParams;
};

#endif // COMMONUSER_OSSV1

//////////////////////////////////////////////////////////////////////
// UCommonSession_HostSessionRequest

FString UCommonSession_HostSessionRequest::GetMapName() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UCommonSession_HostSessionRequest::ConstructTravelURL() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonSession_HostSessionRequest::ValidateAndLogErrors() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonSession_HostSessionRequest::GetMaxPlayers() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// UCommonSessionSubsystem

void UCommonSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::BindOnlineDelegates()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::BindOnlineDelegatesOSSv1()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#else

void UCommonSessionSubsystem::BindOnlineDelegatesOSSv2()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif

void UCommonSessionSubsystem::Deinitialize()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonSessionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UCommonSession_HostSessionRequest* UCommonSessionSubsystem::CreateOnlineHostSessionRequest()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonSession_SearchSessionRequest* UCommonSessionSubsystem::CreateOnlineSearchSessionRequest()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UCommonSessionSubsystem::HostSession(APlayerController* HostingPlayer, UCommonSession_HostSessionRequest* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::CreateOnlineSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::CreateOnlineSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#else

void UCommonSessionSubsystem::CreateOnlineSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#endif

void UCommonSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnRegisterLocalPlayerComplete_CreateSession(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::FinishSessionCreation(bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::FindSessions(APlayerController* SearchingPlayer, UCommonSession_SearchSessionRequest* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::FindSessionsInternal(APlayerController* SearchingPlayer, const TSharedRef<FCommonOnlineSearchSettings>& InSearchSettings)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::FindSessionsInternalOSSv1(ULocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#else

void UCommonSessionSubsystem::FindSessionsInternalOSSv2(ULocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::QuickPlaySession(APlayerController* JoiningOrHostingPlayer, UCommonSession_HostSessionRequest* HostRequest)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettings(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#if COMMONUSER_OSSV1
TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettingsOSSv1(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#else

TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettingsOSSv2(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UCommonSession_HostSessionRequest> HostRequest)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::CleanUpSessions()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::CleanUpSessionsOSSv1()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#else
void UCommonSessionSubsystem::CleanUpSessionsOSSv2()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#endif // COMMONUSER_OSSV1

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1


void UCommonSessionSubsystem::JoinSession(APlayerController* JoiningPlayer, UCommonSession_SearchResult* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::JoinSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::JoinSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#else

void UCommonSessionSubsystem::JoinSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::OnSessionJoinRequested(const UE::Online::FUILobbyJoinRequested& EventParams)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UE::Online::FAccountId UCommonSessionSubsystem::GetAccountId(APlayerController* PlayerController) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UE::Online::FLobbyId UCommonSessionSubsystem::GetLobbyId(const FName SessionName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::InternalTravelToSession(const FName SessionName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::NotifyUserRequestedSession(const FPlatformUserId& PlatformUserId, UCommonSession_SearchResult* RequestedSession, const FOnlineResultInformation& RequestedSessionResult)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::NotifyJoinSessionComplete(const FOnlineResultInformation& Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::NotifyCreateSessionComplete(const FOnlineResultInformation& Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 LocalUserIndex, FUniqueNetIdPtr AcceptingUserId, const FOnlineSessionSearchResult& SearchResult)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonSessionSubsystem::HandlePostLoadMap(UWorld* World)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#undef LOCTEXT_NAMESPACE
