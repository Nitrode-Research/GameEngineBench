// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_WaitDelayOneFrame.h"

UGSAT_WaitDelayOneFrame::UGSAT_WaitDelayOneFrame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGSAT_WaitDelayOneFrame::Activate()
{
	EndTask();
}

UGSAT_WaitDelayOneFrame* UGSAT_WaitDelayOneFrame::WaitDelayOneFrame(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UGSAT_WaitDelayOneFrame>(OwningAbility);
}

void UGSAT_WaitDelayOneFrame::OnDelayFinish()
{
	EndTask();
}
