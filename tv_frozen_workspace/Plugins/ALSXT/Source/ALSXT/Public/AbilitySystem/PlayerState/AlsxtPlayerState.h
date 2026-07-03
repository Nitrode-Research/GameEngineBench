// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerState.h"
#include "../AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AlsxtCharacter.h"
#include "AlsxtPlayerState.generated.h"

class UAlsxtCharacterCustomizationComponent;
class UAlsxtCharacterMovementComponent;

/**
* @file AlsxtPlayerState.h
* @brief Base ALSXT Player State class. ASC and Gameplay Abilities/Effects are implemented here.
* AlsxtPlayerState is a template class that contains all shared Logic and Data for Player State Classes.
* Create a Blueprint class based on this class, do not use the C++ class directly in the Editor
*/
UCLASS()
class ALSXT_API AAlsxtPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAlsxtPlayerState(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "MovementComponent")
	UAlsxtCharacterMovementComponent* GetAlsxtCharacterMovementComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;


	void InitializeAbilitySystem();

	// This event is fired after Ability System Component initialization is finished.
	UFUNCTION(BlueprintNativeEvent)
	void PostInitializeAbilitySystem();

protected:
	// Data used to initialize the Ability System Component. (Can be found in "AlsxtAbilitySystemData.h")
	// UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability System", Meta = (ShowOnlyInnerProperties))
	// FAlsxtAbilitySystemInitializationData AbilitySystemInitializationData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability System")
	TSoftObjectPtr<UAlsxtAbilitySystemInitializationDataAsset> AbilitySystemInitializationData;
	
	UPROPERTY(EditAnywhere, Category = "Ability System")
	UAlsxtAbilitySystemComponent* AbilitySystemComponent;

	UAlsxtCharacterCustomizationComponent* CharacterCustomizationComponent;
	
	void MovementSpeedMultiplierChanged(const FOnAttributeChangeData& OnAttributeChangeData) const;	
	
};
