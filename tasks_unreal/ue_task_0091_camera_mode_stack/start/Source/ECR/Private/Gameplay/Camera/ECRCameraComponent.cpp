// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/Camera/ECRCameraMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"


UECRCameraComponent::UECRCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::OnRegister()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::UpdateCameraModes()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::PushCameraMode(TSubclassOf<UECRCameraMode> CameraModeClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::DrawDebug(UCanvas* Canvas) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraComponent::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

