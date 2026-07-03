// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_WaitTargetDataUsingActor.h"

UGSAT_WaitTargetDataUsingActor::UGSAT_WaitTargetDataUsingActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TargetActor = nullptr;
	bCreateKeyIfNotValidForMorePredicting = false;
	ConfirmationType = EGameplayTargetingConfirmation::Instant;
}

UGSAT_WaitTargetDataUsingActor* UGSAT_WaitTargetDataUsingActor::WaitTargetDataWithReusableActor(UGameplayAbility* OwningAbility, FName TaskInstanceName, TEnumAsByte<EGameplayTargetingConfirmation::Type> InConfirmationType, AGameplayAbilityTargetActor* InTargetActor, bool bInCreateKeyIfNotValidForMorePredicting)
{
	return NewAbilityTask<UGSAT_WaitTargetDataUsingActor>(OwningAbility, TaskInstanceName);
}

void UGSAT_WaitTargetDataUsingActor::Activate()
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::OnTargetDataReplicatedCancelledCallback()
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& Data)
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& Data)
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::ExternalConfirm(bool bEndTask)
{
	if (bEndTask)
	{
		EndTask();
	}
}

void UGSAT_WaitTargetDataUsingActor::ExternalCancel()
{
	EndTask();
}

void UGSAT_WaitTargetDataUsingActor::InitializeTargetActor() const
{
}

void UGSAT_WaitTargetDataUsingActor::FinalizeTargetActor() const
{
}

void UGSAT_WaitTargetDataUsingActor::RegisterTargetDataCallbacks()
{
}

void UGSAT_WaitTargetDataUsingActor::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

bool UGSAT_WaitTargetDataUsingActor::ShouldReplicateDataToServer() const
{
	return false;
}
