// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RicochetMagazineBase.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
#include "RicochetMagazineDetachable.generated.h"

/**
* @file RicochetMagazineDetachable.h
* @brief Detachable Magazine class, has Health, Dirtiness and Weight Attributes and can affect 
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RICOCHET_API URicochetMagazineDetachable : public URicochetMagazineBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URicochetMagazineDetachable();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	FGameplayTagContainer CompatibleReceivers;

	float MagazineWeight = 1.0;

	virtual float GetMagazineWeight();

	virtual float GetTotalMagazineWeight();

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float Health = 1.0f;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float Dirtiness = 0.0f;
};
