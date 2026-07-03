// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRPawnComponent_CharacterParts.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagAssetInterface.h"
#include "Gameplay/GAS/ECRPlayerOwnedTaggedActor.h"
#include "Gameplay/Player/ECRPlayerState.h"

//////////////////////////////////////////////////////////////////////

FString FECRAppliedCharacterPartEntry::GetDebugString() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////

void FECRCharacterPartList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRCharacterPartList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRCharacterPartList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FECRCharacterPartHandle FECRCharacterPartList::AddEntry(FECRCharacterPart NewPart)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void FECRCharacterPartList::RemoveEntry(FECRCharacterPartHandle Handle)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void FECRCharacterPartList::ClearAllEntries(bool bBroadcastChangeDelegate)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FGameplayTagContainer FECRCharacterPartList::CollectCombinedTags() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void FECRCharacterPartList::UpdatePlayerStateOnEntries()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool FECRCharacterPartList::SpawnActorForEntry(FECRAppliedCharacterPartEntry& Entry)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


bool FECRCharacterPartList::DestroyActorForEntry(FECRAppliedCharacterPartEntry& Entry)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////

UECRPawnComponent_CharacterParts::UECRPawnComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , CharacterPartList(this)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FECRCharacterPartHandle UECRPawnComponent_CharacterParts::AddCharacterPart(const FECRCharacterPart& NewPart)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRPawnComponent_CharacterParts::RemoveCharacterPart(FECRCharacterPartHandle Handle)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::RemoveAllCharacterParts()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::BeginPlay()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


TArray<AActor*> UECRPawnComponent_CharacterParts::GetCharacterPartActors() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


USkeletalMeshComponent* UECRPawnComponent_CharacterParts::GetParentMeshComponent() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


USceneComponent* UECRPawnComponent_CharacterParts::GetSceneComponentToAttachTo() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


FGameplayTagContainer UECRPawnComponent_CharacterParts::GetCombinedTags(FGameplayTag RequiredPrefix) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRPawnComponent_CharacterParts::UpdatePlayerStateOnEntries()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::BroadcastChanged()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::SetAdditionalCosmeticTags(const FGameplayTagContainer NewTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::SetAdditionalActorSelectionTags(const FGameplayTagContainer NewTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRPawnComponent_CharacterParts::OnRep_AdditionalCosmeticTags()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

