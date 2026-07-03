// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRPlayerCameraManager.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Engine/Canvas.h"
#include "Gameplay/Camera/ECRUICameraManagerComponent.h"
#include "GameFramework/PlayerController.h"

static FName UICameraComponentName(TEXT("UICamera"));

AECRPlayerCameraManager::AECRPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRUICameraManagerComponent* AECRPlayerCameraManager::GetUICameraComponent() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void AECRPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

