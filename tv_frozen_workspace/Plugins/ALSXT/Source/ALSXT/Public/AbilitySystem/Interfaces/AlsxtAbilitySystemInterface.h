// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AbilitySystemInterface.h"
#include "AlsxtAbilitySystemInterface.generated.h"

/**
* @file AlsxtAbilitySystemInterface.h
* @brief Child Interface class of UAbilitySystemInterface for expansion and convenience.
* UAbilitySystemInterface is not a Blueprintable Interface (CannotImplementInterfaceInBlueprint) and can only be inherited in C++.
*/

class UAlsxtAbilitySystemInitializationDataAsset;
class UAlsxtAbilitySystemComponent;
struct FAlsxtAbilitySystemInitializationData;

/** Interface for actors that expose access to an ability system component */
UINTERFACE(MinimalAPI)
class UAlsxtAbilitySystemInterface : public UAbilitySystemInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtAbilitySystemInterface : public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	/** Returns the ability system component to use for this actor. It may live on another actor, such as a Pawn using the PlayerState's component */
	virtual UAlsxtAbilitySystemComponent* GetAlsxtAbilitySystemComponent() const = 0;
	virtual const TSoftObjectPtr<UAlsxtAbilitySystemInitializationDataAsset> GetAbilitySystemInitializationData() const = 0;
};