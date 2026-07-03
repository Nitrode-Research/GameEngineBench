#pragma once

#include "RicochetEnums.generated.h"

UENUM(BlueprintType)
enum class ERicochetPenetrationTraceMode : uint8
{
	PT_BackTrace UMETA(DisplayName = "Back Trace"),
	PT_ByComponent UMETA(DisplayName = "By Component"),
	PT_TwoSidedGeometry UMETA(DisplayName = "Double Sided Geometry"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ERicochetPenetrationTraceMode, ERicochetPenetrationTraceMode::Count);

UENUM(BlueprintType)
enum class ERicochetFireMode : uint8
{
	FM_Auto UMETA(DisplayName = "Full Auto"),
	FM_Semiauto UMETA(DisplayName = "Semiauto"),
	FM_SingleAction UMETA(DisplayName = "SingleAction"),
	FM_DoubleAction UMETA(DisplayName = "DoubleAction"),
	FM_Manual UMETA(DisplayName = "Manual"),
	FM_Burst UMETA(DisplayName = "Burst"),
	FM_InterBurst UMETA(DisplayName = "Interruptible Burst"),
	FM_Binary UMETA(DisplayName = "Binary"),
	FM_Slamfire UMETA(DisplayName = "Slam Fire"),
	FM_Gatling UMETA(DisplayName = "Gatling"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ERicochetFireMode, ERicochetFireMode::Count);