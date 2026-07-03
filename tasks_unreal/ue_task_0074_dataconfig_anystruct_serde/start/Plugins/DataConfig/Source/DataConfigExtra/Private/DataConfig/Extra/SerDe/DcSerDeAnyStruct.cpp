#include "DataConfig/Extra/SerDe/DcSerDeAnyStruct.h"
#include "DataConfig/DcEnv.h"

namespace DcExtra
{

FDcResult DcHandlerDeserializeAnyStruct(
	FDcDeserializeContext& Ctx,
	TFunctionRef<FDcResult(FDcDeserializeContext&, const FString&, UScriptStruct*&)> FuncLocateStruct
) {
	return DcNoEntry();
}

FDcResult DcHandlerSerializeAnyStruct(
	FDcSerializeContext& Ctx,
	TFunctionRef<FString(UScriptStruct* InStruct)> FuncWriteStructType
) {
	return DcNoEntry();
}

FDcResult HandlerDcAnyStructDeserialize(FDcDeserializeContext& Ctx)
{
	return DcNoEntry();
}

FDcResult HandlerDcAnyStructSerialize(FDcSerializeContext& Ctx)
{
	return DcNoEntry();
}

} // namespace DcExtra
