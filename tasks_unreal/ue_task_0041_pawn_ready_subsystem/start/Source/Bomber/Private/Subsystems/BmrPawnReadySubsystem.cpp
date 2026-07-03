// Copyright (c) Yevhenii Selivanov

#include "Subsystems/BmrPawnReadySubsystem.h"

// Bomber
#include "Actors/BmrPawn.h"
#include "GameFramework/BmrPlayerState.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPawnReadySubsystem)

UBmrPawnReadySubsystem& UBmrPawnReadySubsystem::Get(const UObject* OptionalWorldContext)
{
	UBmrPawnReadySubsystem* Subsystem = GetPawnReadySubsystem(OptionalWorldContext);
	checkf(Subsystem, TEXT("ERROR: [%i] %hs:\n'Subsystem' is null!"), __LINE__, __FUNCTION__);
	return *Subsystem;
}

UBmrPawnReadySubsystem* UBmrPawnReadySubsystem::GetPawnReadySubsystem(const UObject* OptionalWorldContext)
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	return World ? World->GetSubsystem<UBmrPawnReadySubsystem>() : nullptr;
}

void UBmrPawnReadySubsystem::Broadcast_OnPawnPossessed(ABmrPawn& Pawn)
{
}

void UBmrPawnReadySubsystem::Broadcast_OnPlayerStateInit(const ABmrPlayerState& PlayerState)
{
}

void UBmrPawnReadySubsystem::Broadcast_OnPawnAdded(ABmrPawn& Pawn)
{
}

bool UBmrPawnReadySubsystem::IsReady(const ABmrPawn* Pawn) const
{
	return false;
}

bool UBmrPawnReadySubsystem::IsReady(const ABmrPlayerState* PlayerState) const
{
	return false;
}

void UBmrPawnReadySubsystem::Reset()
{
}

void UBmrPawnReadySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UBmrPawnReadySubsystem::FOnReadyData& UBmrPawnReadySubsystem::FindOrAdd(ABmrPawn& Pawn)
{
	FOnReadyData* Found = OnReadyHandles.FindByPredicate([&Pawn](const FOnReadyData& It) { return It.Pawn == &Pawn; });
	if (Found)
	{
		return *Found;
	}
	FOnReadyData& Added = OnReadyHandles.Emplace_GetRef();
	Added.Pawn = &Pawn;
	return Added;
}

bool UBmrPawnReadySubsystem::IsPawnPossessed(const FOnReadyData& FoundHandle)
{
	return false;
}

void UBmrPawnReadySubsystem::TryBroadcastOnReady_Internal(ABmrPawn& Pawn)
{
}
