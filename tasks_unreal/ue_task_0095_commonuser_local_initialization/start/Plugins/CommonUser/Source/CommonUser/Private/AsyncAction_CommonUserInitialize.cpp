// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncAction_CommonUserInitialize.h"

#include "Delegates/Delegate.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Misc/AssertionMacros.h"
#include "TimerManager.h"
#include "UObject/WeakObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_CommonUserInitialize)

UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::InitializeForLocalPlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::LoginForOnlinePlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UAsyncAction_CommonUserInitialize::HandleFailure()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_CommonUserInitialize::HandleInitializationComplete(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_CommonUserInitialize::Activate()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


