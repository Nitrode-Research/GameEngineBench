// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_ServerWaitForClientTargetData.h"

UGSAT_ServerWaitForClientTargetData::UGSAT_ServerWaitForClientTargetData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGSAT_ServerWaitForClientTargetData* UGSAT_ServerWaitForClientTargetData::ServerWaitForClientTargetData(UGameplayAbility* OwningAbility, FName TaskInstanceName, bool TriggerOnce)
{
	return NewAbilityTask<UGSAT_ServerWaitForClientTargetData>(OwningAbility, TaskInstanceName);
}

void UGSAT_ServerWaitForClientTargetData::Activate()
{
	EndTask();
}

void UGSAT_ServerWaitForClientTargetData::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	EndTask();
}

void UGSAT_ServerWaitForClientTargetData::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}
