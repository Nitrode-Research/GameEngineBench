// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "AxosInterface.generated.h"

class UAxosGameStateComponent;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAxosInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class AXOS_API IAxosInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Axos Interface")
	TSoftObjectPtr<UAxosGameStateComponent> GetAxosGameStateComponent();
};
