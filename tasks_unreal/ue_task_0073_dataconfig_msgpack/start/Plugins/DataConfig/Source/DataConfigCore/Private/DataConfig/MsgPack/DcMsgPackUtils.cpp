#include "DataConfig/MsgPack/DcMsgPackUtils.h"
#include "DataConfig/DcEnv.h"

namespace DcMsgPackUtils
{

const FName DC_META_MSGPACK_BLOB = FName(TEXT("DcMsgPackBlob"));

FDcResult MsgPackExtensionHandler(FDcReader* RawReader, FDcWriter* RawWriter)
{
	return DcNoEntry();
}

FDcResult ReadExtBytes(FDcMsgPackReader* Reader, uint8& OutType, TArray<uint8>& OutBytes)
{
	OutType = 0;
	OutBytes.Reset();
	return DcNoEntry();
}

} // namespace DcMsgPackUtils
