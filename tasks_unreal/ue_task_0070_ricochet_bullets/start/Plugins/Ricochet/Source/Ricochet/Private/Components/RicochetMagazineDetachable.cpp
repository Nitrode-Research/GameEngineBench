// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Components/RicochetMagazineDetachable.h"
#include "Actors/RicochetBullet.h"

/**
* @file RicochetMagazineDetachable.cpp
* @brief Detachable Magazine class, has Health, Dirtiness and Weight Attributes and can affect 
*/
URicochetMagazineDetachable::URicochetMagazineDetachable()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void URicochetMagazineDetachable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


// Called when the game starts
void URicochetMagazineDetachable::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void URicochetMagazineDetachable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

float URicochetMagazineDetachable::GetMagazineWeight()
{
	return MagazineWeight;
}

float URicochetMagazineDetachable::GetTotalMagazineWeight()
{
	if (Ammunition.IsEmpty())
	{
		return GetMagazineWeight();
	}
	else
	{
		float RoundsWeight = 0;
		for (const TSoftObjectPtr<ARicochetBullet> Round : Ammunition)
		{
			RoundsWeight = RoundsWeight + Round.Get()->GetRoundWeight();
		}
		return RoundsWeight + GetMagazineWeight();
	}
}

