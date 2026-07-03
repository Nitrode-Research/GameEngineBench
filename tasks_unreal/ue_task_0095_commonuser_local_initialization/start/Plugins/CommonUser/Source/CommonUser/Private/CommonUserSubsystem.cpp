// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonUserSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"
#include "UObject/UObjectHash.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CommonUserSubsystem)

#if COMMONUSER_OSSV1
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#else
#include "Online/Auth.h"
#include "Online/ExternalUI.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"
#include "Online/Privileges.h"

using namespace UE::Online;
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogCommonUser, Log, All);
DEFINE_LOG_CATEGORY(LogCommonUser);

UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Error, "SystemMessage.Error");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Warning, "SystemMessage.Warning");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Display, "SystemMessage.Display");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Error_InitializeLocalPlayerFailed, "SystemMessage.Error.InitializeLocalPlayerFailed");

UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::Platform_Trait_RequiresStrictControllerMapping, "Platform.Trait.RequiresStrictControllerMapping");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::Platform_Trait_SingleOnlineUser, "Platform.Trait.SingleOnlineUser");


//////////////////////////////////////////////////////////////////////
// UCommonUserInfo

UCommonUserInfo::FCachedData* UCommonUserInfo::GetCachedData(ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo::FCachedData* UCommonUserInfo::GetCachedData(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UCommonUserInfo::UpdateCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserInfo::UpdateCachedNetId(const FUniqueNetIdRepl& NewId, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


class UCommonUserSubsystem* UCommonUserInfo::GetSubsystem() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


ECommonUserPrivilegeResult UCommonUserInfo::GetCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


ECommonUserAvailability UCommonUserInfo::GetPrivilegeAvailability(ECommonUserPrivilege Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FUniqueNetIdRepl UCommonUserInfo::GetNetId(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UCommonUserInfo::GetNickname() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UCommonUserInfo::GetDebugString() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FPlatformUserId UCommonUserInfo::GetPlatformUserId() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonUserInfo::GetPlatformUserIndex() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



//////////////////////////////////////////////////////////////////////
// UCommonUserSubsystem

void UCommonUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::CreateOnlineContexts()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::Deinitialize()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::DestroyOnlineContexts()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UCommonUserInfo* UCommonUserSubsystem::CreateLocalUserInfo(int32 LocalPlayerIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


bool UCommonUserSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::BindOnlineDelegates()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::LogOutLocalUser(FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::TransferPlatformAuth(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::AutoLogin(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::ShowLoginUI(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::QueryUserPrivilege(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



#if COMMONUSER_OSSV1
IOnlineSubsystem* UCommonUserSubsystem::GetOnlineSubsystem(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


IOnlineIdentity* UCommonUserSubsystem::GetOnlineIdentity(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


FName UCommonUserSubsystem::GetOnlineSubsystemName(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


EOnlineServerConnectionStatus::Type UCommonUserSubsystem::GetConnectionStatus(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::BindOnlineDelegatesOSSv1()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::AutoLoginOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::ShowLoginUIOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::QueryUserPrivilegeOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#else

UE::Online::EOnlineServices UCommonUserSubsystem::GetOnlineServicesProvider(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UE::Online::IAuthPtr UCommonUserSubsystem::GetOnlineAuth(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UE::Online::EOnlineServicesConnectionStatus UCommonUserSubsystem::GetConnectionStatus(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::BindOnlineDelegatesOSSv2()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::CacheConnectionStatus(ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::TransferPlatformAuthOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::AutoLoginOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::ShowLoginUIOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::QueryUserPrivilegeOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSharedPtr<FAccountInfo> UCommonUserSubsystem::GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#endif

bool UCommonUserSubsystem::HasOnlineConnection(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


ELoginStatusType UCommonUserSubsystem::GetLocalUserLoginStatus(FPlatformUserId PlatformUser, ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FUniqueNetIdRepl UCommonUserSubsystem::GetLocalUserNetId(FPlatformUserId PlatformUser, ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::SetMaxLocalPlayers(int32 InMaxLocalPlayers)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UCommonUserSubsystem::GetMaxLocalPlayers() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonUserSubsystem::GetNumLocalPlayers() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


ECommonUserInitializationState UCommonUserSubsystem::GetLocalPlayerInitializationState(int32 LocalPlayerIndex) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::TryToInitializeForLocalPlay(int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::TryToLoginForOnlinePlay(int32 LocalPlayerIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::TryToInitializeUser(FCommonUserInitializeParams Params)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::ListenForLoginKeyInput(TArray<FKey> AnyUserKeys, TArray<FKey> NewUserKeys, FCommonUserInitializeParams Params)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::CancelUserInitialization(int32 LocalPlayerIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::ResetUserState()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::OverrideInputKeyForLogin(FInputKeyEventArgs& EventArgs)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


static inline FText GetErrorText(const FOnlineErrorType& InOnlineError)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::HandleLoginForUserInitialize(const UCommonUserInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, const TOptional<FOnlineErrorType>& InError, ECommonUserOnlineContext Context, FCommonUserInitializeParams Params)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleUserInitializeFailed(FCommonUserInitializeParams Params, FText Error)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleUserInitializeSucceeded(FCommonUserInitializeParams Params)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::LoginLocalUser(const UCommonUserInfo* UserInfo, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext Context, FOnLocalUserLoginCompleteDelegate OnComplete)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
void UCommonUserSubsystem::HandleUserLoginCompleted(int32 PlatformUserIndex, bool bWasSuccessful, const FUniqueNetId& NetId, const FString& ErrorString, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleOnLoginUIClosed(TSharedPtr<const FUniqueNetId> LoggedInNetId, const int PlatformUserIndex, const FOnlineError& Error, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleCheckPrivilegesComplete(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults, ECommonUserPrivilege UserPrivilege, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#else

void UCommonUserSubsystem::HandleUserLoginCompletedV2(const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result, FPlatformUserId PlatformUser, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleOnLoginUIClosedV2(const UE::Online::TOnlineResult<UE::Online::FExternalUIShowLoginUI>& Result, FPlatformUserId PlatformUser, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleCheckPrivilegesComplete(const UE::Online::TOnlineResult<UE::Online::FQueryUserPrivilege>& Result, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, EUserPrivileges DesiredPrivilege, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1

void UCommonUserSubsystem::RefreshLocalUserInfo(UCommonUserInfo* UserInfo)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleChangedAvailability(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::UpdateUserPrivilegeResult(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#if COMMONUSER_OSSV1
ECommonUserPrivilege UCommonUserSubsystem::ConvertOSSPrivilege(EUserPrivileges::Type Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


EUserPrivileges::Type UCommonUserSubsystem::ConvertOSSPrivilege(ECommonUserPrivilege Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


ECommonUserPrivilegeResult UCommonUserSubsystem::ConvertOSSPrivilegeResult(EUserPrivileges::Type Privilege, uint32 Results) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

#else
ECommonUserPrivilege UCommonUserSubsystem::ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


EUserPrivileges UCommonUserSubsystem::ConvertOnlineServicesPrivilege(ECommonUserPrivilege Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


ECommonUserPrivilegeResult UCommonUserSubsystem::ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

#endif // COMMONUSER_OSSV1

FString UCommonUserSubsystem::PlatformUserIdToString(FPlatformUserId UserId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString UCommonUserSubsystem::ECommonUserOnlineContextToString(ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FText UCommonUserSubsystem::GetPrivilegeDescription(ECommonUserPrivilege Privilege) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FText UCommonUserSubsystem::GetPrivilegeResultDescription(ECommonUserPrivilegeResult Result) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


const UCommonUserSubsystem::FOnlineContextCache* UCommonUserSubsystem::GetContextCache(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonUserSubsystem::FOnlineContextCache* UCommonUserSubsystem::GetContextCache(ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


ECommonUserOnlineContext UCommonUserSubsystem::ResolveOnlineContext(ECommonUserOnlineContext Context) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::HasSeparatePlatformContext() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const UCommonUserInfo* UserInfo)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForPlatformUserIndex(int32 PlatformUserIndex) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForControllerId(int32 ControllerId) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForInputDevice(FInputDeviceId InputDevice) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


bool UCommonUserSubsystem::IsRealPlatformUserIndex(int32 PlatformUserIndex) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUserSubsystem::IsRealPlatformUser(FPlatformUserId PlatformUser) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FPlatformUserId UCommonUserSubsystem::GetPlatformUserIdForIndex(int32 PlatformUserIndex) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 UCommonUserSubsystem::GetPlatformUserIndexForId(FPlatformUserId PlatformUser) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FPlatformUserId UCommonUserSubsystem::GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FInputDeviceId UCommonUserSubsystem::GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUserSubsystem::SetTraitTags(const FGameplayTagContainer& InTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonUserSubsystem::ShouldWaitForStartInput() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


#if COMMONUSER_OSSV1
void UCommonUserSubsystem::HandleIdentityLoginStatusChanged(int32 PlatformUserIndex, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleControllerPairingChanged(int32 PlatformUserIndex, FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleNetworkConnectionStatusChanged(const FString& ServiceName, EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#else
void UCommonUserSubsystem::HandleAuthLoginStatusChanged(const UE::Online::FAuthLoginStatusChanged& EventParameters, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUserSubsystem::HandleNetworkConnectionStatusChanged(const UE::Online::FConnectionStatusChanged& EventParameters, ECommonUserOnlineContext Context)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

#endif // COMMONUSER_OSSV1

void UCommonUserSubsystem::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

