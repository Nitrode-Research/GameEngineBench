// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_PlayMontageForMeshAndWaitForEvent.h"

UGSAT_PlayMontageForMeshAndWaitForEvent::UGSAT_PlayMontageForMeshAndWaitForEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mesh = nullptr;
	MontageToPlay = nullptr;
	Rate = 1.0f;
	StartSection = NAME_None;
	AnimRootMotionTranslationScale = 1.0f;
	bStopWhenAbilityEnds = true;
	bReplicateMontage = true;
	OverrideBlendOutTimeForCancelAbility = -1.0f;
	OverrideBlendOutTimeForStopWhenEndAbility = -1.0f;
}

UGSAbilitySystemComponent* UGSAT_PlayMontageForMeshAndWaitForEvent::GetTargetASC()
{
	return nullptr;
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::OnAbilityCancelled()
{
	EndTask();
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndTask();
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
}

UGSAT_PlayMontageForMeshAndWaitForEvent* UGSAT_PlayMontageForMeshAndWaitForEvent::PlayMontageForMeshAndWaitForEvent(UGameplayAbility* OwningAbility, FName TaskInstanceName, USkeletalMeshComponent* InMesh, UAnimMontage* InMontageToPlay, FGameplayTagContainer InEventTags, float InRate, FName InStartSection, bool bInStopWhenAbilityEnds, float InAnimRootMotionTranslationScale, bool bInReplicateMontage, float InOverrideBlendOutTimeForCancelAbility, float InOverrideBlendOutTimeForStopWhenEndAbility)
{
	return NewAbilityTask<UGSAT_PlayMontageForMeshAndWaitForEvent>(OwningAbility, TaskInstanceName);
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::Activate()
{
	EndTask();
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::ExternalCancel()
{
	EndTask();
}

void UGSAT_PlayMontageForMeshAndWaitForEvent::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}

bool UGSAT_PlayMontageForMeshAndWaitForEvent::StopPlayingMontage(float OverrideBlendOutTime)
{
	return false;
}

FString UGSAT_PlayMontageForMeshAndWaitForEvent::GetDebugString() const
{
	return FString();
}
