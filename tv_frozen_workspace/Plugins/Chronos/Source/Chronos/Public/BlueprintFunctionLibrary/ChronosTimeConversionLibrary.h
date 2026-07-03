// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/ChronosTypes.h"
#include "ChronosTimeConversionLibrary.generated.h"

UCLASS()
class CHRONOS_API UChronosTimeConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Time")
	static FChronosTimeofDay Convert24HrFloatToCustomTime(float TimeOfDayFloat, FChronosTimeofDayDefinition TimeOfDayDefinition);
};