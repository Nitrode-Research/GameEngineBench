// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions/AsyncAction_CreateWidgetAsync.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonUIExtensions.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "UObject/Stack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_CreateWidgetAsync)

class UUserWidget;

static const FName InputFilterReason_Template = FName(TEXT("CreatingWidgetAsync"));

UAsyncAction_CreateWidgetAsync::UAsyncAction_CreateWidgetAsync(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bSuspendInputUntilComplete(true)
{
}

UAsyncAction_CreateWidgetAsync* UAsyncAction_CreateWidgetAsync::CreateWidgetAsync(UObject* InWorldContextObject, TSoftClassPtr<UUserWidget> InUserWidgetSoftClass, APlayerController* InOwningPlayer, bool bSuspendInputUntilComplete)
{
	if (InUserWidgetSoftClass.IsNull())
	{
		FFrame::KismetExecutionMessage(TEXT("CreateWidgetAsync was passed a null UserWidgetSoftClass"), ELogVerbosity::Error);
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncAction_CreateWidgetAsync* Action = NewObject<UAsyncAction_CreateWidgetAsync>();
	Action->UserWidgetSoftClass = InUserWidgetSoftClass;
	Action->OwningPlayer = InOwningPlayer;
	Action->World = World;
	Action->GameInstance = World->GetGameInstance();
	Action->bSuspendInputUntilComplete = bSuspendInputUntilComplete;
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_CreateWidgetAsync::Activate()
{
	SetReadyToDestroy();
}

void UAsyncAction_CreateWidgetAsync::Cancel()
{
	Super::Cancel();
}

void UAsyncAction_CreateWidgetAsync::OnWidgetLoaded()
{
	SetReadyToDestroy();
}
