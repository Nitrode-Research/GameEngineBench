#pragma once

#include "AlsxtVertexEnums.generated.h"

UENUM(BlueprintType)
enum class EProminentRGBAChannel : uint8
{
	Red UMETA(DisplayName = "Red"),
	Green UMETA(DisplayName = "Green"),
	Blue UMETA(DisplayName = "Blue"),
	Alpha UMETA(DisplayName = "Alpha"),
	White UMETA(DisplayName = "White"),
	None  UMETA(DisplayName = "None"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EProminentRGBAChannel, EProminentRGBAChannel::Count);

UENUM(BlueprintType)
enum class EVertexColorChannel : uint8
{
	Red UMETA(DisplayName = "Red"),
	Green UMETA(DisplayName = "Green"),
	Blue UMETA(DisplayName = "Blue"),
	Alpha UMETA(DisplayName = "Alpha"),
	White UMETA(DisplayName = "White"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EVertexColorChannel, EVertexColorChannel::Count);