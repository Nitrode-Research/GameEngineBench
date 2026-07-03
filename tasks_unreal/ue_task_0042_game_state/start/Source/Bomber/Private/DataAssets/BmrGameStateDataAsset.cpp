// Copyright (c) Yevhenii Selivanov

#include "DataAssets/BmrGameStateDataAsset.h"

// Bomber
#include "DalSubsystem.h"
#include "Structures/BmrGameStateTag.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrGameStateDataAsset)

// Returns the Game State data asset
const UBmrGameStateDataAsset& UBmrGameStateDataAsset::Get()
{
	return UDalSubsystem::GetDataAssetChecked<ThisClass>();
}

// Returns the authoritative list of allowed game-state phase transitions
const TArray<FBmrGameStateTransition>& UBmrGameStateDataAsset::GetAllowedTransitions() const
{
	if (!AllowedTransitions.IsEmpty())
	{
		return AllowedTransitions;
	}

	// Canonical Bomberrage fallback graph — used when designer hasn't authored DA_GameState's array.
	// Function-local static avoids static-init-order issues between this TU and BmrGameStateTag.cpp.
	static const TArray<FBmrGameStateTransition> CanonicalGraph = {
		{FGameplayTag(), FBmrGameStateTag::Menu},
		{FBmrGameStateTag::Menu, FBmrGameStateTag::GameStarting},
		{FBmrGameStateTag::GameStarting, FBmrGameStateTag::InGame},
		{FBmrGameStateTag::InGame, FBmrGameStateTag::EndGame},
		{FBmrGameStateTag::EndGame, FBmrGameStateTag::Menu}
	};
	return CanonicalGraph;
}
