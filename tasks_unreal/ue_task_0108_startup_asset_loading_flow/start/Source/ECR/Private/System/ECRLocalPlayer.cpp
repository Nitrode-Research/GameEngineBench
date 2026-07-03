// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ECRLocalPlayer.h"
#include "Settings/ECRSettingsLocal.h"
#include "Settings/ECRSettingsShared.h"
#include "AudioMixerBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

UECRLocalPlayer::UECRLocalPlayer()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRLocalPlayer::PostInitProperties()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



UECRSettingsLocal* UECRLocalPlayer::GetLocalSettings() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECRSettingsShared* UECRLocalPlayer::GetSharedSettings() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRLocalPlayer::ResetSharedSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRLocalPlayer::OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRLocalPlayer::OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

