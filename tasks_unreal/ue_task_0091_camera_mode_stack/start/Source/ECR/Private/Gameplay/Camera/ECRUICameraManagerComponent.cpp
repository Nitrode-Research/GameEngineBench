// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRUICameraManagerComponent.h"
#include "EngineUtils.h"
#include "Algo/Transform.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/HUD.h"
#include "Engine/Engine.h"

#include "Gameplay/Camera/ECRCameraMode.h"
#include "Gameplay/Camera/ECRPlayerCameraManager.h"

UECRUICameraManagerComponent* UECRUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UECRUICameraManagerComponent::UECRUICameraManagerComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRUICameraManagerComponent::InitializeComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}
