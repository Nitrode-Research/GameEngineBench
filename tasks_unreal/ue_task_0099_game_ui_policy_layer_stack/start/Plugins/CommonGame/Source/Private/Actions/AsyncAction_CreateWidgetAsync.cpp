// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions/AsyncAction_CreateWidgetAsync.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "CommonInputSubsystem.h"
#include "CommonUIExtensions.h"

static const FName InputFilterReason_Template = FName(TEXT("CreatingWidgetAsync"));

UAsyncAction_CreateWidgetAsync::UAsyncAction_CreateWidgetAsync(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bSuspendInputUntilComplete(true)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UAsyncAction_CreateWidgetAsync* UAsyncAction_CreateWidgetAsync::CreateWidgetAsync(UObject* InWorldContextObject, TSoftClassPtr<UUserWidget> InUserWidgetSoftClass, APlayerController* InOwningPlayer, bool bSuspendInputUntilComplete)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UAsyncAction_CreateWidgetAsync::Activate()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_CreateWidgetAsync::Cancel()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_CreateWidgetAsync::OnWidgetLoaded()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

