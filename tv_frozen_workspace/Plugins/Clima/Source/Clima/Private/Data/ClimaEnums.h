// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "ClimaEnums.generated.h"

/**
* @file ClimaEnums.h
* @brief Clima Enums
*/

UENUM(BlueprintType)
enum class EClimaAtmosphereType : uint8
{
	AT_Constant UMETA(DisplayName = "Constant"),
	AT_Curve UMETA(DisplayName = "Density Curve"),
	AT_Earth UMETA(DisplayName = "Earth/IGL")
};