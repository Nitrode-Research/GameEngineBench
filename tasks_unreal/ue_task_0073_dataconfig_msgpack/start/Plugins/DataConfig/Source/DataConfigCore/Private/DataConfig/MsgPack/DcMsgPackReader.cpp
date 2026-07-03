#include "DataConfig/MsgPack/DcMsgPackReader.h"
#include "DataConfig/DcEnv.h"

FDcMsgPackReader::FDcMsgPackReader()
{
	States.Add({EReadState::Root, false, 1});
}

FDcMsgPackReader::FDcMsgPackReader(FDcBlobViewData Blob)
	: View(Blob)
{
	States.Add({EReadState::Root, false, 1});
}

FDcResult FDcMsgPackReader::PeekRead(EDcDataEntry* OutPtr)
{
	if (OutPtr)
		*OutPtr = EDcDataEntry::Ended;
	return DcOk();
}

FDcResult FDcMsgPackReader::Coercion(EDcDataEntry ToEntry, bool* OutPtr)
{
	if (OutPtr)
		*OutPtr = false;
	return DcOk();
}

FDcResult FDcMsgPackReader::PeekTypeByte(uint8* OutPtr)
{
	if (OutPtr)
		*OutPtr = 0;
	return DcNoEntry();
}

FDcResult FDcMsgPackReader::ReadNone() { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadBool(bool* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadString(FString* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadName(FName* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadText(FText* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadBlob(FDcBlobViewData* OutPtr) { return DcNoEntry(); }

FDcResult FDcMsgPackReader::ReadMapRoot() { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadMapEnd() { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadArrayRoot() { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadArrayEnd() { return DcNoEntry(); }

FDcResult FDcMsgPackReader::ReadInt8(int8* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadInt16(int16* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadInt32(int32* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadInt64(int64* OutPtr) { return DcNoEntry(); }

FDcResult FDcMsgPackReader::ReadUInt8(uint8* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadUInt16(uint16* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadUInt32(uint32* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadUInt64(uint64* OutPtr) { return DcNoEntry(); }

FDcResult FDcMsgPackReader::ReadFloat(float* OutPtr) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadDouble(double* OutPtr) { return DcNoEntry(); }

FDcResult FDcMsgPackReader::ReadFixExt1(uint8* OutType, uint8* OutByte) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadFixExt2(uint8* OutType, FDcBytes2* OutBytes) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadFixExt4(uint8* OutType, FDcBytes4* OutBytes) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadFixExt8(uint8* OutType, FDcBytes8* OutBytes) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadFixExt16(uint8* OutType, FDcBytes16* OutBytes) { return DcNoEntry(); }
FDcResult FDcMsgPackReader::ReadExt(uint8* OutType, FDcBlobViewData* OutBlob) { return DcNoEntry(); }

FName FDcMsgPackReader::ClassId() { return FName(TEXT("DcMsgPackReader")); }
FName FDcMsgPackReader::GetId() { return ClassId(); }

void FDcMsgPackReader::FormatDiagnostic(FDcDiagnostic& Diag)
{
}
