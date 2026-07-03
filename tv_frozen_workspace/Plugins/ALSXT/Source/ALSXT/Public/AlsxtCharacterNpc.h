// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AbilitySystem/AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "UObject/ObjectPtr.h"
#include "AbilitySystemComponent.h"
#include "AlsxtCharacter.h"
#include "AlsxtCharacterNpc.generated.h"

class UAlsxtCharacterCustomizationComponent;

/**
* @file AlsxtCharacterNpc.cpp
* @brief Base AI ALSXT Character class. AI do not have Player States so we implement everything directly on the Avatar.
* AlsxtCharacterNPC is a template class that contains all shared Logic and Data for NPC Classes.
* Create a Blueprint class based on this class, do not use the C++ class directly in the Editor
*/
UCLASS()
class ALSXT_API AAlsxtCharacterNpc : public AAlsxtCharacter
{
	GENERATED_BODY()

public:
	AAlsxtCharacterNpc(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{ return AbilitySystemComponent; }
	
protected:	
	UAlsxtAbilitySystemComponent* AbilitySystemComponent;
	UAlsxtCharacterCustomizationComponent* CharacterCustomizationComponent;

	void InitializeAbilitySystem();

	// This event is fired after Ability System Component initialization is finished.
	UFUNCTION(BlueprintNativeEvent)
	void PostInitializeAbilitySystem();

};