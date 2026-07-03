// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "RicochetMagazineBase.generated.h"

class ARicochetBullet;

/**
* @file RicochetMagazineBase.h
* @brief Base or Internal Magazine class, Health would be stored in Receiver
*/
UCLASS()
class URicochetMagazineBase : public UActorComponent, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	URicochetMagazineBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	bool LoadingBlocked;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	bool bHasAmmoCountInterface;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	bool bHasAmmoWindow;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Ammo")
	TArray<TSoftObjectPtr<ARicochetBullet>> Ammunition;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	int MagazineCapacity = 1;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float FailureToFeedProbability = 0.0f;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float LoadingAnimationSpeedModifier = 1.0f;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	// Ability System Component for the bullet (optional, or can be tied to instigator)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
