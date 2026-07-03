#pragma once

#include "CoreMinimal.h"
#include "DataConfig/Misc/DcTypeUtils.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"

struct FDcJsonWriterShared;

namespace DcSerializeUtils
{

DATACONFIGCORE_API FDcResult RecursiveSerialize(FDcSerializeContext& Ctx);

DATACONFIGCORE_API EDcSerializePredicateResult PredicateIsRootProperty(FDcSerializeContext& Ctx);

struct DATACONFIGCORE_API FDcScopedTryUseJSONOverrideConfig
{
	FDcJsonWriterShared* JsonWriterShared;
	bool bPrevUseOverrideConfig;

	FDcScopedTryUseJSONOverrideConfig(FDcWriter* Writer);
	~FDcScopedTryUseJSONOverrideConfig();
};

} // namespace DcSerializeUtils

