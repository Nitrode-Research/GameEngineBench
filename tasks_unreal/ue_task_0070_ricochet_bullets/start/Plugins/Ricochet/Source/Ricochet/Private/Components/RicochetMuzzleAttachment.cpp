// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).


#include "Components/RicochetMuzzleAttachment.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"


/**
* @file RicochetMuzzleAttachment.cpp
* @brief Core firearm component that defines Calibers, Fire Modes, Fire Rate, Base Recoil, and has Barrel and Magazine Attachments
*/
URicochetMuzzleAttachment::URicochetMuzzleAttachment()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
 
	// Set Replication Mode to Minimal for Actors spawned by Server.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}


// Called when the game starts
void URicochetMuzzleAttachment::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void URicochetMuzzleAttachment::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void URicochetMuzzleAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_SkipOwner;
	// DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MuzzleAttachmentWeight, Parameters)
}

float URicochetMuzzleAttachment::GetMuzzleAttachmentWeight()
{
	return 0.0;
}

UAbilitySystemComponent* URicochetMuzzleAttachment::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

