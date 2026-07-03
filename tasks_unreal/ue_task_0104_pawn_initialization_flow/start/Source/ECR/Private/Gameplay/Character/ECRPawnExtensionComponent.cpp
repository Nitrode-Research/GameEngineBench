// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "System/ECRLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"

UECRPawnExtensionComponent::UECRPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::OnRegister()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::SetPawnData(const UECRPawnData* InPawnData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::OnRep_PawnData()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::InitializeAbilitySystem(UECRAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::UninitializeAbilitySystem()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::HandleControllerChanged()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::HandlePlayerStateReplicated()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::SetupPlayerInputComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRPawnExtensionComponent::CheckPawnReadyToInitialize()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRPawnExtensionComponent::OnPawnReadyToInitialize_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

