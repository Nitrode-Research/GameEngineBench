// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRControllerComponent_CharacterParts.h"
#include "ECRPawnComponent_CharacterParts.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CheatManager.h"

//////////////////////////////////////////////////////////////////////

UECRControllerComponent_CharacterParts::UECRControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::BeginPlay()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRPawnComponent_CharacterParts* UECRControllerComponent_CharacterParts::GetPawnCustomizer() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRControllerComponent_CharacterParts::AddCharacterPart(const FECRCharacterPart& NewPart)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::AddCharacterPartInternal(const FECRCharacterPart& NewPart, ECharacterPartSource Source)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::RemoveCharacterPart(const FECRCharacterPart& PartToRemove)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::RemoveAllCharacterParts()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::SetAdditionalCosmeticTags(const FGameplayTagContainer NewTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::SetAdditionalActorSelectionTags(const FGameplayTagContainer NewTags)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRControllerComponent_CharacterParts::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

