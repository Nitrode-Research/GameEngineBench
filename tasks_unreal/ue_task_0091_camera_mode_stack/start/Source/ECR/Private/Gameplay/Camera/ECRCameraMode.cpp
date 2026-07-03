// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRCameraMode.h"
#include "Gameplay/Camera/ECRPlayerCameraManager.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"


//////////////////////////////////////////////////////////////////////////
// FECRCameraModeView
//////////////////////////////////////////////////////////////////////////
FECRCameraModeView::FECRCameraModeView()
	: Location(ForceInit)
	  , Rotation(ForceInit)
	  , ControlRotation(ForceInit)
	  , FieldOfView(ECR_CAMERA_DEFAULT_FOV)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRCameraModeView::Blend(const FECRCameraModeView& Other, float OtherWeight)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



//////////////////////////////////////////////////////////////////////////
// UECRCameraMode
//////////////////////////////////////////////////////////////////////////
UECRCameraMode::UECRCameraMode()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRCameraComponent* UECRCameraMode::GetECRCameraComponent() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UWorld* UECRCameraMode::GetWorld() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


AActor* UECRCameraMode::GetTargetActor() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


FVector UECRCameraMode::GetPivotLocation() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FRotator UECRCameraMode::GetPivotRotation() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRCameraMode::UpdateCameraMode(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode::UpdateView(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode::SetBlendWeight(float Weight)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode::UpdateBlending(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode::DrawDebug(UCanvas* Canvas) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FRotator UECRCameraMode::NullifyRotatorRollIfNeeded(FRotator InputRot) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



//////////////////////////////////////////////////////////////////////////
// UECRCameraModeStack
//////////////////////////////////////////////////////////////////////////
UECRCameraModeStack::UECRCameraModeStack()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::ActivateStack()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::DeactivateStack()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::PushCameraMode(TSubclassOf<UECRCameraMode> CameraModeClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRCameraModeStack::EvaluateStack(float DeltaTime, FECRCameraModeView& OutCameraModeView)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UECRCameraMode* UECRCameraModeStack::GetCameraModeInstance(TSubclassOf<UECRCameraMode> CameraModeClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRCameraModeStack::UpdateStack(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::BlendStack(FECRCameraModeView& OutCameraModeView) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::DrawDebug(UCanvas* Canvas) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraModeStack::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

