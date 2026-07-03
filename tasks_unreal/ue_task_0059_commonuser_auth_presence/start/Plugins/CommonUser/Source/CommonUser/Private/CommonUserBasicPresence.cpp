// Copyright Epic Games, Inc. All Rights Reserved.
#include "CommonUserBasicPresence.h"
#include "CommonSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "CommonUserTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CommonUserBasicPresence)


#if COMMONUSER_OSSV1
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlinePresenceInterface.h"
#else
#include "Online/OnlineServicesEngineUtils.h"
#include "Online/Presence.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogUserBasicPresence, Log, All);
DEFINE_LOG_CATEGORY(LogUserBasicPresence);

UCommonUserBasicPresence::UCommonUserBasicPresence()
{

}

void UCommonUserBasicPresence::Initialize(FSubsystemCollectionBase& Collection)
{
}

void UCommonUserBasicPresence::Deinitialize()
{

}

FString UCommonUserBasicPresence::SessionStateToBackendKey(ECommonSessionInformationState SessionStatus)
{
	return FString();
}

void UCommonUserBasicPresence::OnNotifySessionInformationChanged(ECommonSessionInformationState SessionStatus, const FString& GameMode, const FString& MapName)
{
}
