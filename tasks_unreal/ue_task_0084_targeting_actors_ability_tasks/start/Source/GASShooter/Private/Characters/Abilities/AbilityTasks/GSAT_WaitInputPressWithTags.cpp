// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_WaitInputPressWithTags.h"

UGSAT_WaitInputPressWithTags::UGSAT_WaitInputPressWithTags(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTestInitialState = false;
}

UGSAT_WaitInputPressWithTags* UGSAT_WaitInputPressWithTags::WaitInputPressWithTags(UGameplayAbility* OwningAbility, FGameplayTagContainer RequiredTags, FGameplayTagContainer IgnoredTags, bool bTestAlreadyPressed)
{
	return NewAbilityTask<UGSAT_WaitInputPressWithTags>(OwningAbility);
}

void UGSAT_WaitInputPressWithTags::Activate()
{
	EndTask();
}

void UGSAT_WaitInputPressWithTags::OnPressCallback()
{
	EndTask();
}

void UGSAT_WaitInputPressWithTags::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

void UGSAT_WaitInputPressWithTags::Reset()
{
}
