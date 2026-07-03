#pragma once

#include "CoreMinimal.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"

namespace DcCoreTypesHandlers {

DATACONFIGCORE_API FDcResult HandlerGuidSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector2fSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector3fSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector4fSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerMatrix44fSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntPointSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntVectorSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerColorSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerDateTimeSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerTimespanSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerTextSerialize(FDcSerializeContext& Ctx);

#if ENGINE_MAJOR_VERSION == 5
DATACONFIGCORE_API FDcResult HandlerVector2dSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector3dSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector4dSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerMatrix44dSerialize(FDcSerializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntVector4Serialize(FDcSerializeContext& Ctx);
#endif // ENGINE_MAJOR_VERSION == 5

} // namespace DcCommonHandlers



