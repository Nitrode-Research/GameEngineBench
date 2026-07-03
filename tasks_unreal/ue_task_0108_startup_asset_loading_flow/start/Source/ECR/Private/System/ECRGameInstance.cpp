// Copyleft: All rights reversed


#include "System/ECRGameInstance.h"
#include "System/ECRLogChannels.h"
#include "System/MatchSettings.h"
#include "GUI/ECRGUIPlayerController.h"
#include "ECRUtilsLibrary.h"
#include "OnlineSubsystem.h"
#include "Algo/Accumulate.h"
#include "GameFramework/PlayerState.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineTitleFileInterface.h"
#include "Kismet/KismetSystemLibrary.h"


UECRGameInstance::UECRGameInstance()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::LogOut()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::LoginViaEpic(const FString PlayerName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::LoginPersistent(const FString PlayerName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::LoginViaDevTool(const FString PlayerName, const FString Address, const FString CredName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::Login(FString PlayerName, FString LoginType, FString Id, FString Token)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::OnLoginComplete(int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId,
                                       const FString& Error)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



FString UECRGameInstance::GetMatchFactionString(
	const TArray<FFactionAlliance>& FactionAlliances, const TMap<FName, FText>& FactionNamesToShortTexts)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UECRGameInstance::CreateMatch(FECRMatchSettings MatchSettings)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::TravelToNewMatch(FECRMatchSettings MatchSettings, FString NewLevel)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::FindMatches(const FString GameVersion, const FString MatchType, const FString MatchMode,
                                   const FString MapName, const FString
                                   RegionName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::FindMatchByUniqueInGameId(const FString GameVersion, const FString MatchId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::JoinMatch(const FBlueprintSessionResult Session)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRGameInstance::TogglePlayerRegistrationInMatch(FUniqueNetIdRepl Player, bool bRegister)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UECRGameInstance::UpdateSessionSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::UpdateSessionCurrentPlayerAmount(const int32 NewPlayerAmount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::UpdateSessionMatchStartedTimestamp(const double NewTimestamp)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::UpdateSessionDayTime(const FName NewDayTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::DestroySession()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::OnCreateMatchComplete(FName SessionName, const bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::OnFindMatchesComplete(const bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnFindMatchByUniqueIdComplete(bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::OnJoinSessionComplete(const FName SessionName,
                                             const EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
                                                 const FString& ErrorStr)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyCreationComplete(FName SessionName, bool bWasSuccessful)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyInviteAcceptedByMe(const bool bWasSuccessful, const int32 ControllerId,
                                                 FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnJoinPartyComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyLeaveComplete(FName SessionName, bool bSuccess)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyMemberJoined(FName SessionName, const FUniqueNetId& UniqueId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyMemberDataChanged(FName SessionName, const FUniqueNetId& TargetUniqueNetId,
                                                const FOnlineSessionSettings& SessionSettings)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyMemberLeft(FName SessionName, const FUniqueNetId& Player, EOnSessionParticipantLeftReason Reason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnPartyDataReceived(FName SessionName, const FOnlineSessionSettings& NewSettings)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::OnSessionFailure(const FUniqueNetId& PlayerId, ESessionFailure::Type Reason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FOnlineSessionSettings UECRGameInstance::GetSessionSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FOnlineSessionSettings UECRGameInstance::GetPartySessionSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UECRGameInstance::GetConnectionStringWithParams(FString ConnectString, bool bListen)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



FString UECRGameInstance::GetPlayerNickname()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::GetIsLoggedIn()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UECRGameInstance::GetUserAccountID()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UECRGameInstance::GetUserAuthToken()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRGameInstance::QueueGettingFriendsList()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::CreateParty(TMap<FString, FString> SessionData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::DestroyParty()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UECRGameInstance::GetIsInHostPartySession()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::GetIsInClientPartySession()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



FString UECRGameInstance::GetPartyMemberName(FUniqueNetIdRepl MemberId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRGameInstance::KickPartyMember(FUniqueNetIdRepl MemberId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::LeaveParty()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::InviteToParty(FUniqueNetIdRepl PlayerId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



TArray<FUniqueNetIdRepl> UECRGameInstance::GetPartyMembersList(bool bForClient)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::SetPartyData(FString Key, FString Value)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::TogglePartyPresence(bool bWantPresence)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::SetPartyMemberData(FString Key, FString Value, bool bForClient)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UECRGameInstance::SetPartyMemberDataInBatch(TMap<FString, FString> Data, bool bForClient)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UECRGameInstance::GetPartyData(bool bForClient)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRGameInstance::StartListeningForPartyEvents()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::QueueReadTitleStorageFile(const FString& FileName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRGameInstance::HandleReadTitleStorageFileCompleted(bool bWasSuccessful, const FString& FileName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::Init()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRGameInstance::Shutdown()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

