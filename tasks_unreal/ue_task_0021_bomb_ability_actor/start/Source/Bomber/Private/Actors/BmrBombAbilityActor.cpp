// Copyright (c) Yevhenii Selivanov.

#include "Actors/BmrBombAbilityActor.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrGeneratedMap.h"
#include "Bomber.h"
#include "Components/BmrMapComponent.h"
#include "DataAssets/BmrGameStateDataAsset.h"
#include "DataRegistries/BmrBombRow.h"
#include "GameFramework/BmrGameState.h"
#include "MyUtilsLibraries/MultiplayerUtilsLibrary.h"
#include "UtilityLibraries/BmrActorUtilsLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

#if WITH_EDITOR
#include "BmrUnrealEdEngine.h"
#include "MyEditorUtilsLibraries/EditorUtilsLibrary.h"
#endif

// UE
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstance.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrBombAbilityActor)

ABmrBombAbilityActor::ABmrBombAbilityActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	MapComponent = CreateDefaultSubobject<UBmrMapComponent>(TEXT("MapComponent"));
}

bool ABmrBombAbilityActor::IsBombReady() const
{
	return false;
}

/*********************************************************************************************
 * Detonation
 ********************************************************************************************* */

void ABmrBombAbilityActor::InitBomb(UAbilitySystemComponent* InASC)
{
}

int32 ABmrBombAbilityActor::GetFireRadius() const
{
	return -1;
}

void ABmrBombAbilityActor::TryDisplayExplosionCells()
{
}

void ABmrBombAbilityActor::UpdateExplosionCells()
{
}

/*********************************************************************************************
 * Cue Visuals: VFXs, SFXs, Materials
 ********************************************************************************************* */

void ABmrBombAbilityActor::ApplyMesh()
{
}

void ABmrBombAbilityActor::ApplyMaterial()
{
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

void ABmrBombAbilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ABmrBombAbilityActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABmrBombAbilityActor::OnRep_InstigatorAbilitySystemComponent()
{
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

void ABmrBombAbilityActor::OnAddedToLevel_Implementation(UBmrMapComponent* InMapComponent)
{
}

void ABmrBombAbilityActor::OnBombReady_Implementation()
{
}

void ABmrBombAbilityActor::OnPostRemovedFromLevel_Implementation(UBmrMapComponent* InMapComponent, UObject* DestroyCauser)
{
}

void ABmrBombAbilityActor::OnAnyPawnCellExit_Implementation(UBmrMapComponent* PlayerMapComponent, const FBmrCell& NewCell, const FBmrCell& PreviousCell)
{
}

/*********************************************************************************************
 * Custom Collision Response
 ********************************************************************************************* */

bool ABmrBombAbilityActor::IsServerReplicaOfClient() const
{
	return false;
}

void ABmrBombAbilityActor::InitCollisionResponseToAllPlayers()
{
}

void ABmrBombAbilityActor::GetCollisionResponseToPlayerByID(FCollisionResponseContainer& InOutCollisionResponses, int32 PlayerId, ECollisionResponse NewResponse)
{
}

bool ABmrBombAbilityActor::TryBindOnInstigatorReachedBombCell()
{
	return false;
}

void ABmrBombAbilityActor::OnInstigatorPawnCellEnter_Implementation(UBmrMapComponent* InMapComponent, const FBmrCell& NewCell, const FBmrCell& PreviousCell)
{
}
