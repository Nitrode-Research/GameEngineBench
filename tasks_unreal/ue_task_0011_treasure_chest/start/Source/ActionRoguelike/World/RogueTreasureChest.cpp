// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueTreasureChest.h"
#include "ActionRoguelike.h"

#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/RogueCurveAnimSubsystem.h"
#include "Components/AudioComponent.h"
#include "Core/RogueDeferredTaskSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueTreasureChest)



ARogueTreasureChest::ARogueTreasureChest()
{
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(BaseMesh);

	OpenChestEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OpeningEffectComp"));
	OpenChestEffect->SetupAttachment(RootComponent);

	OpenChestSound = CreateDefaultSubobject<UAudioComponent>(TEXT("OpenChestSFX"));
	OpenChestSound->SetupAttachment(RootComponent);

	bReplicates = true;
}


void ARogueTreasureChest::Interact_Implementation(AController* InstigatorController)
{
}

void ARogueTreasureChest::UpdateTestArray(int32 StartIndex, int32 MaxCount)
{
}


void ARogueTreasureChest::OnActorLoaded_Implementation()
{
}


void ARogueTreasureChest::ConditionalOpenChest()
{
}


void ARogueTreasureChest::OnRep_LidOpened()
{
}


void ARogueTreasureChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARogueTreasureChest, bLidOpened);
}
