// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions/AsyncAction_PushContentToLayerForPlayer.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "CommonInputSubsystem.h"
#include "CommonUIExtensions.h"
#include "GameUIManagerSubsystem.h"
#include "PrimaryGameLayout.h"
#include "GameUIPolicy.h"
#include "CommonLocalPlayer.h"
#include "CommonActivatableWidget.h"

UAsyncAction_PushContentToLayerForPlayer::UAsyncAction_PushContentToLayerForPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UAsyncAction_PushContentToLayerForPlayer* UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(APlayerController* InOwningPlayer, TSoftClassPtr<UCommonActivatableWidget> InWidgetClass, FGameplayTag InLayerName, bool bSuspendInputUntilComplete)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UAsyncAction_PushContentToLayerForPlayer::Cancel()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_PushContentToLayerForPlayer::Activate()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

