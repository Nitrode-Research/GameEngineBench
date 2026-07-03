// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_PlayMontageAndWaitForEvent.h"

UGSAT_PlayMontageAndWaitForEvent::UGSAT_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MontageToPlay = nullptr;
	Rate = 1.0f;
	StartSection = NAME_None;
	AnimRootMotionTranslationScale = 1.0f;
	bStopWhenAbilityEnds = true;
}

UGSAbilitySystemComponent* UGSAT_PlayMontageAndWaitForEvent::GetTargetASC()
{
	return nullptr;
}

void UGSAT_PlayMontageAndWaitForEvent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
}

void UGSAT_PlayMontageAndWaitForEvent::OnAbilityCancelled()
{
	EndTask();
}

void UGSAT_PlayMontageAndWaitForEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndTask();
}

void UGSAT_PlayMontageAndWaitForEvent::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
}

UGSAT_PlayMontageAndWaitForEvent* UGSAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(UGameplayAbility* OwningAbility, FName TaskInstanceName, UAnimMontage* InMontageToPlay, FGameplayTagContainer InEventTags, float InRate, FName InStartSection, bool bInStopWhenAbilityEnds, float InAnimRootMotionTranslationScale)
{
	return NewAbilityTask<UGSAT_PlayMontageAndWaitForEvent>(OwningAbility, TaskInstanceName);
}

void UGSAT_PlayMontageAndWaitForEvent::Activate()
{
	EndTask();
}

void UGSAT_PlayMontageAndWaitForEvent::ExternalCancel()
{
	EndTask();
}

void UGSAT_PlayMontageAndWaitForEvent::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

bool UGSAT_PlayMontageAndWaitForEvent::StopPlayingMontage()
{
	return false;
}

FString UGSAT_PlayMontageAndWaitForEvent::GetDebugString() const
{
	return FString();
}
