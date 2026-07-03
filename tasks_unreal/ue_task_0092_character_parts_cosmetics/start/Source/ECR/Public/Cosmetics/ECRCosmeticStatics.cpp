#include "ECRCosmeticStatics.h"

#include "ECRPawnComponent_CharacterParts.h"

UECRPawnComponent_CharacterParts* UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(AActor* TargetActor)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRCosmeticStatics::AddMontageToLoadQueueIfNeeded(const TSoftObjectPtr<UAnimMontage>& Montage,
                                                        TArray<FSoftObjectPath>& MontagesToLoad)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TSubclassOf<AActor> UECRCosmeticStatics::SelectBestActor(const FECRActorSelectionSet Set,
                                                         const FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSoftObjectPtr<UAnimMontage> UECRCosmeticStatics::SelectBestMontage(const FECRAnimMontageSelectionSet Set,
                                                                    FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


USkeletalMesh* UECRCosmeticStatics::SelectBestMesh(const FECRMeshSelectionSet Set, FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


TSubclassOf<UAnimInstance> UECRCosmeticStatics::SelectBestAnimInstance(const FECRAnimInstanceSelectionSet Set,
                                                           FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSubclassOf<UAnimInstance> UECRCosmeticStatics::SelectBestAnimLayer(const FECRAnimLayerSelectionSet Set,
	FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


TSoftObjectPtr<UTexture2D> UECRCosmeticStatics::SelectBestSoftTexture(const FECRSoftTexture2DSelectionSet Set,
                                                                      FGameplayTagContainer CosmeticTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

