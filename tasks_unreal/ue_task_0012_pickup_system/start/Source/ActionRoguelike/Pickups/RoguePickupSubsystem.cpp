// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupSubsystem.h"

#include "EngineUtils.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/RogueDeveloperSettings.h"
#include "Core/RogueGameState.h"
#include "Player/RoguePlayerCharacter.h"
#include "ActionRoguelike.h"


void URoguePickupSubsystem::AddCoinsPickup(TArray<FVector> Locations, TArray<int32> CoinAmount)
{
}


void URoguePickupSubsystem::RemoveCoinsPickup(int32 InIndex)
{
}

FPrimitiveInstanceId URoguePickupSubsystem::AddMeshInstance(FVector InLocation)
{
	return FPrimitiveInstanceId();
}

TArray<FPrimitiveInstanceId> URoguePickupSubsystem::AddMeshInstances(const TArray<FTransform>& InAdded)
{
	return {};
}

void URoguePickupSubsystem::RemoveMeshInstances(const TArray<FPrimitiveInstanceId>& IdsToRemove)
{
}


void URoguePickupSubsystem::CreateWorldISM()
{
}


void URoguePickupSubsystem::PlayPickupSound()
{
}

void URoguePickupSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool URoguePickupSubsystem::IsTickable() const
{
	return false;
}

void URoguePickupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URoguePickupSubsystem::OnSoundAssetLoadComplete(const FSoftObjectPath& SoftObjectPath, UObject* LoadedObject)
{
}
