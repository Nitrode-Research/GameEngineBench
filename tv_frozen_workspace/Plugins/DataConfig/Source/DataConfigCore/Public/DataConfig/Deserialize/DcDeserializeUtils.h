#pragma once

#include "CoreMinimal.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"

namespace DcDeserializeUtils
{

DATACONFIGCORE_API FDcResult RecursiveDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGCORE_API EDcDeserializePredicateResult PredicateIsRootProperty(FDcDeserializeContext& Ctx);

} // namespace DcDeserializeUtils


