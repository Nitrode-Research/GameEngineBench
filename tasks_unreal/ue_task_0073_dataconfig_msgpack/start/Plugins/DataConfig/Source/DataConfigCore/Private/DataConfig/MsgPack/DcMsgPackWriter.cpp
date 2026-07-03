#include "DataConfig/MsgPack/DcMsgPackWriter.h"
#include "DataConfig/DcEnv.h"

FDcMsgPackWriter::BufferType& FDcMsgPackWriter::GetMainBuffer()
{
	return States[0].Buffer;
}

FDcMsgPackWriter::FDcMsgPackWriter()
{
	States.Add({EWriteState::Root, false, 0, 0});
}

FDcResult FDcMsgPackWriter::PeekWrite(EDcDataEntry Next, bool* bOutOk)
{
	if (bOutOk)
		*bOutOk = false;
	return DcOk();
}

FDcResult FDcMsgPackWriter::WriteNone() { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteBool(bool Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteString(const FString& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteName(const FName& Name) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteText(const FText& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteBlob(const FDcBlobViewData& Value) { return DcNoEntry(); }

FDcResult FDcMsgPackWriter::WriteMapRoot() { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteMapEnd() { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteArrayRoot() { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteArrayEnd() { return DcNoEntry(); }

FDcResult FDcMsgPackWriter::WriteInt8(const int8& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteInt16(const int16& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteInt32(const int32& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteInt64(const int64& Value) { return DcNoEntry(); }

FDcResult FDcMsgPackWriter::WriteUInt8(const uint8& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteUInt16(const uint16& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteUInt32(const uint32& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteUInt64(const uint64& Value) { return DcNoEntry(); }

FDcResult FDcMsgPackWriter::WriteFloat(const float& Value) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteDouble(const double& Value) { return DcNoEntry(); }

FDcResult FDcMsgPackWriter::WriteFixExt1(uint8 Type, uint8 Byte) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteFixExt2(uint8 Type, FDcBytes2 Bytes) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteFixExt4(uint8 Type, FDcBytes4 Bytes) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteFixExt8(uint8 Type, FDcBytes8 Bytes) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteFixExt16(uint8 Type, FDcBytes16 Bytes) { return DcNoEntry(); }
FDcResult FDcMsgPackWriter::WriteExt(uint8 Type, FDcBlobViewData Blob) { return DcNoEntry(); }

void FDcMsgPackWriter::FormatDiagnostic(FDcDiagnostic& Diag)
{
}

FName FDcMsgPackWriter::ClassId() { return FName(TEXT("DcMsgPackWriter")); }
FName FDcMsgPackWriter::GetId() { return ClassId(); }
