// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonUIExtensions.h"
#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "GameUIManagerSubsystem.h"
#include "GameUIPolicy.h"
#include "PrimaryGameLayout.h"
#include "CommonActivatableWidget.h"
#include "CommonLocalPlayer.h"
#include "Engine/AssetManager.h"

int32 UCommonUIExtensions::InputSuspensions = 0;

ECommonInputType UCommonUIExtensions::GetOwningPlayerInputType(const UUserWidget* WidgetContextObject)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUIExtensions::IsOwningPlayerUsingTouch(const UUserWidget* WidgetContextObject)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool UCommonUIExtensions::IsOwningPlayerUsingGamepad(const UUserWidget* WidgetContextObject)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UCommonActivatableWidget* UCommonUIExtensions::PushContentToLayer_ForPlayer(const ULocalPlayer* LocalPlayer, FGameplayTag LayerName, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(const ULocalPlayer* LocalPlayer, FGameplayTag LayerName, TSoftClassPtr<UCommonActivatableWidget> WidgetClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUIExtensions::PopContentFromLayer(UCommonActivatableWidget* ActivatableWidget)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


ULocalPlayer* UCommonUIExtensions::GetLocalPlayerFromController(APlayerController* PlayerController)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


FName UCommonUIExtensions::SuspendInputForPlayer(APlayerController* PlayerController, FName SuspendReason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FName UCommonUIExtensions::SuspendInputForPlayer(ULocalPlayer* LocalPlayer, FName SuspendReason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonUIExtensions::ResumeInputForPlayer(APlayerController* PlayerController, FName SuspendToken)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonUIExtensions::ResumeInputForPlayer(ULocalPlayer* LocalPlayer, FName SuspendToken)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

