// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/GSGameplayAbility.h"

UGSGameplayAbility::UGSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGSGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
}

FGameplayAbilityTargetDataHandle UGSGameplayAbility::MakeGameplayAbilityTargetDataHandleFromActorArray(const TArray<AActor*> TargetActors)
{
	return FGameplayAbilityTargetDataHandle();
}

FGameplayAbilityTargetDataHandle UGSGameplayAbility::MakeGameplayAbilityTargetDataHandleFromHitResults(const TArray<FHitResult> HitResults)
{
	return FGameplayAbilityTargetDataHandle();
}

FGSGameplayEffectContainerSpec UGSGameplayAbility::MakeEffectContainerSpecFromContainer(const FGSGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	return FGSGameplayEffectContainerSpec();
}

FGSGameplayEffectContainerSpec UGSGameplayAbility::MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	return FGSGameplayEffectContainerSpec();
}

TArray<FActiveGameplayEffectHandle> UGSGameplayAbility::ApplyEffectContainerSpec(const FGSGameplayEffectContainerSpec& ContainerSpec)
{
	return TArray<FActiveGameplayEffectHandle>();
}

UObject* UGSGameplayAbility::K2_GetSourceObject(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo) const
{
	return nullptr;
}

bool UGSGameplayAbility::BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle, bool EndAbilityImmediately)
{
	return false;
}

void UGSGameplayAbility::ExternalEndAbility()
{
}

FString UGSGameplayAbility::GetCurrentPredictionKeyStatus()
{
	return FString();
}

bool UGSGameplayAbility::IsPredictionKeyValidForMorePrediction() const
{
	return false;
}

bool UGSGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UGSGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

bool UGSGameplayAbility::GSCheckCost_Implementation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo) const
{
	return true;
}

void UGSGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UGSGameplayAbility::SetHUDReticle(TSubclassOf<UGSHUDReticle> ReticleClass)
{
}

void UGSGameplayAbility::ResetHUDReticle()
{
}

void UGSGameplayAbility::SendTargetDataToServer(const FGameplayAbilityTargetDataHandle& TargetData)
{
}

bool UGSGameplayAbility::IsInputPressed() const
{
	return false;
}

UAnimMontage* UGSGameplayAbility::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	return nullptr;
}

void UGSGameplayAbility::SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* InCurrentMontage)
{
}

bool UGSGameplayAbility::FindAbillityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMeshMontage)
{
	return false;
}

void UGSGameplayAbility::MontageJumpToSectionForMesh(USkeletalMeshComponent* InMesh, FName SectionName)
{
}

void UGSGameplayAbility::MontageSetNextSectionNameForMesh(USkeletalMeshComponent* InMesh, FName FromSectionName, FName ToSectionName)
{
}

void UGSGameplayAbility::MontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime)
{
}

void UGSGameplayAbility::MontageStopForAllMeshes(float OverrideBlendOutTime)
{
}
