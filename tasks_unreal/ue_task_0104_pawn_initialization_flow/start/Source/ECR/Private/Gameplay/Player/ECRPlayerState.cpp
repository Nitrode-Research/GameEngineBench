// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Player/ECRPlayerState.h"

#include "OnlineSubsystemTypes.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"


AECRPlayerState::AECRPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void AECRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::PreInitializeComponents()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::Reset()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::ClientInitialize(AController* C)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::SetTempNetId(const FString SomeString)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void AECRPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 AECRPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void AECRPlayerState::PostInitializeComponents()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

