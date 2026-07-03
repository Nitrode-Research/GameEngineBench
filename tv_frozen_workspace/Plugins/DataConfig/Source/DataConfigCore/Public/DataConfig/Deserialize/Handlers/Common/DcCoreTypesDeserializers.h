#pragma once

#include "CoreMinimal.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"

namespace DcCoreTypesHandlers {

DATACONFIGCORE_API FDcResult HandlerGuidDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector2fDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector3fDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector4fDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerMatrix44fDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntPointDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntVectorDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerColorDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerDateTimeDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerTimespanDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerTextDeserialize(FDcDeserializeContext& Ctx);

#if ENGINE_MAJOR_VERSION == 5
DATACONFIGCORE_API FDcResult HandlerVector2dDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector3dDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerVector4dDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerMatrix44dDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGCORE_API FDcResult HandlerIntVector4Deserialize(FDcDeserializeContext& Ctx);
#endif // ENGINE_MAJOR_VERSION == 5

} // namespace DcCommonHandlers



