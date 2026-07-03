// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameUIPolicy.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameUIManagerSubsystem.h"
#include "CommonLocalPlayer.h"
#include "PrimaryGameLayout.h"
#include "Engine/Engine.h"
#include "LogCommonGame.h"

// Static
UGameUIPolicy* UGameUIPolicy::GetGameUIPolicy(const UObject* WorldContextObject)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UGameUIManagerSubsystem* UGameUIPolicy::GetOwningUIManager() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UWorld* UGameUIPolicy::GetWorld() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UPrimaryGameLayout* UGameUIPolicy::GetRootLayout(const UCommonLocalPlayer* LocalPlayer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UGameUIPolicy::NotifyPlayerAdded(UCommonLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::NotifyPlayerRemoved(UCommonLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::NotifyPlayerDestroyed(UCommonLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::AddLayoutToViewport(UCommonLocalPlayer* LocalPlayer, UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::RemoveLayoutFromViewport(UCommonLocalPlayer* LocalPlayer, UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::OnRootLayoutAddedToViewport(UCommonLocalPlayer* LocalPlayer, UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::OnRootLayoutRemovedFromViewport(UCommonLocalPlayer* LocalPlayer, UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::OnRootLayoutReleased(UCommonLocalPlayer* LocalPlayer, UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::RequestPrimaryControl(UPrimaryGameLayout* Layout)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UGameUIPolicy::CreateLayoutWidget(UCommonLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TSubclassOf<UPrimaryGameLayout> UGameUIPolicy::GetLayoutWidgetClass(UCommonLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}
