// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRDamageLogDebuggerComponent.h"
#include "System/Messages/ECRVerbMessage.h"
#include "NativeGameplayTags.h"
#include "System/ECRLogChannels.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ECR_Damage_Message);

UECRDamageLogDebuggerComponent::UECRDamageLogDebuggerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageLogDebuggerComponent::BeginPlay()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageLogDebuggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageLogDebuggerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageLogDebuggerComponent::OnDamageMessage(FGameplayTag Channel, const FECRVerbMessage& Payload)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

