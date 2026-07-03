// Copyright Epic Games, Inc. All Rights Reserved.

#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "GameUIManagerSubsystem.h"
#include "GameUIPolicy.h"
#include "CommonLocalPlayer.h"
#include "Engine/LocalPlayer.h"
#include "LogCommonGame.h"
#include "Kismet/GameplayStatics.h"

/*static*/ UPrimaryGameLayout* UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(const UObject* WorldContextObject)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


/*static*/ UPrimaryGameLayout* UPrimaryGameLayout::GetPrimaryGameLayout(APlayerController* PlayerController)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


/*static*/ UPrimaryGameLayout* UPrimaryGameLayout::GetPrimaryGameLayout(ULocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UPrimaryGameLayout::UPrimaryGameLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UPrimaryGameLayout::SetIsDormant(bool InDormant)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UPrimaryGameLayout::OnIsDormantChanged()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UPrimaryGameLayout::RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UPrimaryGameLayout::OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, bool bIsTransitioning)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UPrimaryGameLayout::FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* ActivatableWidget)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UCommonActivatableWidgetContainerBase* UPrimaryGameLayout::GetLayerWidget(FGameplayTag LayerName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}
