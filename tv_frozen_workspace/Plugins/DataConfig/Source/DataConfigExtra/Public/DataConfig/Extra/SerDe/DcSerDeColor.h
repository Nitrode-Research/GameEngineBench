#pragma once

#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"
#include "DataConfig/Extra/DcExtraCommon.h"
#include "DcSerDeColor.generated.h"

///	`FColor` <-> hex string like `#FFGGBBAA`

namespace DcExtra {

DATACONFIGEXTRA_API FDcResult HandlerColorDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGEXTRA_API FDcResult HandlerColorSerialize(FDcSerializeContext& Ctx);

} // namespace DcExtra

USTRUCT()
struct FDcExtraTestStructWithColor1
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcExtraTestStructWithColor1)

	UPROPERTY() FColor ColorField1;
	UPROPERTY() FColor ColorField2;
};

