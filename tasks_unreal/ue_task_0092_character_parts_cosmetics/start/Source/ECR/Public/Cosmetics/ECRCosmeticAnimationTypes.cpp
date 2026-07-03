// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRCosmeticAnimationTypes.h"

TSoftObjectPtr<UTexture2D> FECRSoftTexture2DSelectionSet::SelectBestTexture(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSubclassOf<UAnimInstance> FECRAnimLayerSelectionSet::SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



TSoftObjectPtr<UAnimMontage> FECRAnimMontageSelectionSet::SelectBestMontage(
	const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TArray<TSoftObjectPtr<UAnimMontage>> FECRAnimMontageSelectionSet::GetAllMontages()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSubclassOf<AActor> FECRActorSelectionSet::SelectBestActor(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


USkeletalMesh* FECRMeshSelectionSet::SelectBestMesh(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


TSubclassOf<UAnimInstance> FECRAnimInstanceSelectionSet::SelectBestAnimInstance(const FGameplayTagContainer& CosmeticTags) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

