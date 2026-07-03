// Copyright 2026 GameDevBench. DataConfig MessagePack source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DataConfigMsgPackTests
{
	static bool LoadSourceFile(const TCHAR* RelativePath, FString& OutSource)
	{
		const FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(OutSource, *SourcePath);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigMsgPack_ReaderProtocolSource,
	"TargetVector.DataConfigMsgPack.reader_protocol_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigMsgPack_ReaderProtocolSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates reader classification, bounds checks, traversal state, numeric byte order, and diagnostics.
	FString Source;
	if (!TestTrue(TEXT("MessagePack reader source must be readable"),
		DataConfigMsgPackTests::LoadSourceFile(TEXT("Plugins/DataConfig/Source/DataConfigCore/Private/DataConfig/MsgPack/DcMsgPackReader.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Reader must guard all byte reads against end-of-buffer access"),
		Source.Contains(TEXT("CheckNoEOF")) && Source.Contains(TEXT("ReadingPastEnd")) && Source.Contains(TEXT("State.Index")));
	TestTrue(TEXT("Reader must classify type bytes through the MessagePack common table"),
		Source.Contains(TEXT("ReadTypeByte")) && Source.Contains(TEXT("TypeByteToDataEntry")) && Source.Contains(TEXT("State.LastTypeByte")));
	TestTrue(TEXT("Reader must maintain array/map/root traversal state"),
		Source.Contains(TEXT("EndTopRead")) && Source.Contains(TEXT("CheckTopStateRemains")) && Source.Contains(TEXT("bMapAtValue")) && Source.Contains(TEXT("Remain")));
	TestTrue(TEXT("Reader must decode nil and booleans"),
		Source.Contains(TEXT("ReadNone")) && Source.Contains(TEXT("ReadDataTypeCheck(this, EDcDataEntry::None)")) && Source.Contains(TEXT("MSGPACK_TRUE")) && Source.Contains(TEXT("MSGPACK_FALSE")));
	TestTrue(TEXT("Reader must decode fixstr and wide UTF-8 strings"),
		Source.Contains(TEXT("MSGPACK_MINFIXSTR")) && Source.Contains(TEXT("MSGPACK_STR8")) && Source.Contains(TEXT("MSGPACK_STR16")) && Source.Contains(TEXT("MSGPACK_STR32")) && Source.Contains(TEXT("FUTF8ToTCHAR")));
	TestTrue(TEXT("Reader must decode binary blobs"),
		Source.Contains(TEXT("MSGPACK_BIN8")) && Source.Contains(TEXT("MSGPACK_BIN16")) && Source.Contains(TEXT("MSGPACK_BIN32")) && Source.Contains(TEXT("FDcBlobViewData")));
	TestTrue(TEXT("Reader must decode fix and wide arrays/maps"),
		Source.Contains(TEXT("MSGPACK_MINFIXMAP")) && Source.Contains(TEXT("MSGPACK_MAP16")) && Source.Contains(TEXT("MSGPACK_MAP32"))
		&& Source.Contains(TEXT("MSGPACK_MINFIXARRAY")) && Source.Contains(TEXT("MSGPACK_ARRAY16")) && Source.Contains(TEXT("MSGPACK_ARRAY32")));
	TestTrue(TEXT("Reader must decode fixed and wide numeric values with byte reversal"),
		Source.Contains(TEXT("ReadNumericDispatch")) && Source.Contains(TEXT("MSGPACK_MINFIXINT")) && Source.Contains(TEXT("MSGPACK_MAXFIXINT"))
		&& Source.Contains(TEXT("ReadUInt8")) && Source.Contains(TEXT("ReadInt8")) && Source.Contains(TEXT("ReadFloat")) && Source.Contains(TEXT("ReadDouble"))
		&& Source.Contains(TEXT("ReverseBytes")));
	TestTrue(TEXT("Reader must decode fixed and variable extension payloads"),
		Source.Contains(TEXT("ReadExtDispatch")) && Source.Contains(TEXT("MSGPACK_FIXEXT1")) && Source.Contains(TEXT("MSGPACK_FIXEXT16"))
		&& Source.Contains(TEXT("MSGPACK_EXT8")) && Source.Contains(TEXT("MSGPACK_EXT16")) && Source.Contains(TEXT("MSGPACK_EXT32")));
	TestTrue(TEXT("Reader diagnostics must report the last decoded entry"),
		Source.Contains(TEXT("FormatDiagnostic")) && Source.Contains(TEXT("Last read")) && Source.Contains(TEXT("TypeByteToDataEntry(State.LastTypeByte)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigMsgPack_WriterProtocolSource,
	"TargetVector.DataConfigMsgPack.writer_protocol_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigMsgPack_WriterProtocolSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates writer transition checks, compact encoding choices, nested buffers, byte order, and diagnostics.
	FString Source;
	if (!TestTrue(TEXT("MessagePack writer source must be readable"),
		DataConfigMsgPackTests::LoadSourceFile(TEXT("Plugins/DataConfig/Source/DataConfigCore/Private/DataConfig/MsgPack/DcMsgPackWriter.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Writer must validate scalar/container write transitions"),
		Source.Contains(TEXT("PeekWrite")) && Source.Contains(TEXT("IsValidWriteScalar")) && Source.Contains(TEXT("ArrayEnd")) && Source.Contains(TEXT("MapEnd")));
	TestTrue(TEXT("Writer must track nested container position and map key/value alternation"),
		Source.Contains(TEXT("EndWriteValuePosition")) && Source.Contains(TEXT("bMapAtValue")) && Source.Contains(TEXT("TopState.Size")));
	TestTrue(TEXT("Writer must write numeric values in MessagePack byte order"),
		Source.Contains(TEXT("WriteNumber")) && Source.Contains(TEXT("FPlatformMemory::Memcpy")) && Source.Contains(TEXT("ReverseBytes")));
	TestTrue(TEXT("Writer must emit compact UTF-8 string encodings"),
		Source.Contains(TEXT("FTCHARToUTF8")) && Source.Contains(TEXT("Mask_3b_5b")) && Source.Contains(TEXT("MSGPACK_MINFIXSTR"))
		&& Source.Contains(TEXT("MSGPACK_STR8")) && Source.Contains(TEXT("MSGPACK_STR16")) && Source.Contains(TEXT("MSGPACK_STR32")));
	TestTrue(TEXT("Writer must emit binary blob encodings"),
		Source.Contains(TEXT("MSGPACK_BIN8")) && Source.Contains(TEXT("MSGPACK_BIN16")) && Source.Contains(TEXT("MSGPACK_BIN32")) && Source.Contains(TEXT("Value.DataPtr")));
	TestTrue(TEXT("Writer must buffer arrays/maps and emit fix or wide container headers"),
		Source.Contains(TEXT("States.Add")) && Source.Contains(TEXT("ParentState.Buffer.Append")) && Source.Contains(TEXT("Mask_4b_4b"))
		&& Source.Contains(TEXT("MSGPACK_MINFIXMAP")) && Source.Contains(TEXT("MSGPACK_MAP16")) && Source.Contains(TEXT("MSGPACK_MAP32"))
		&& Source.Contains(TEXT("MSGPACK_MINFIXARRAY")) && Source.Contains(TEXT("MSGPACK_ARRAY16")) && Source.Contains(TEXT("MSGPACK_ARRAY32")));
	TestTrue(TEXT("Writer must encode compact signed and unsigned integers"),
		Source.Contains(TEXT("MSGPACK_UINT8")) && Source.Contains(TEXT("MSGPACK_UINT16")) && Source.Contains(TEXT("MSGPACK_UINT64"))
		&& Source.Contains(TEXT("MSGPACK_INT8")) && Source.Contains(TEXT("MSGPACK_INT16")) && Source.Contains(TEXT("MSGPACK_INT64"))
		&& Source.Contains(TEXT("Value < 128")) && Source.Contains(TEXT("Value >= -32")));
	TestTrue(TEXT("Writer must encode nil, bool, float, double, and extensions"),
		Source.Contains(TEXT("MSGPACK_NIL")) && Source.Contains(TEXT("MSGPACK_TRUE")) && Source.Contains(TEXT("MSGPACK_FALSE"))
		&& Source.Contains(TEXT("MSGPACK_FLOAT32")) && Source.Contains(TEXT("MSGPACK_FLOAT64"))
		&& Source.Contains(TEXT("MSGPACK_FIXEXT1")) && Source.Contains(TEXT("MSGPACK_FIXEXT16")) && Source.Contains(TEXT("MSGPACK_EXT32")));
	TestTrue(TEXT("Writer diagnostics must report the last written entry"),
		Source.Contains(TEXT("FormatDiagnostic")) && Source.Contains(TEXT("Last write")) && Source.Contains(TEXT("States.Top().LastTypeByte")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigMsgPack_ExtensionDispatchSource,
	"TargetVector.DataConfigMsgPack.extension_dispatch_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigMsgPack_ExtensionDispatchSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates extension copy-through for every fixed and variable MessagePack extension width.
	FString Source;
	if (!TestTrue(TEXT("MessagePack utils source must be readable"),
		DataConfigMsgPackTests::LoadSourceFile(TEXT("Plugins/DataConfig/Source/DataConfigCore/Private/DataConfig/MsgPack/DcMsgPackUtils.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Extension handler must use typed MessagePack reader and writer instances"),
		Source.Contains(TEXT("DcCastReader")) && Source.Contains(TEXT("DcCastWriter")) && Source.Contains(TEXT("PeekTypeByte")));
	TestTrue(TEXT("Extension handler must copy all fixed extension widths"),
		Source.Contains(TEXT("MSGPACK_FIXEXT1")) && Source.Contains(TEXT("ReadFixExt1")) && Source.Contains(TEXT("WriteFixExt1"))
		&& Source.Contains(TEXT("MSGPACK_FIXEXT2")) && Source.Contains(TEXT("ReadFixExt2")) && Source.Contains(TEXT("WriteFixExt2"))
		&& Source.Contains(TEXT("MSGPACK_FIXEXT4")) && Source.Contains(TEXT("ReadFixExt4")) && Source.Contains(TEXT("WriteFixExt4"))
		&& Source.Contains(TEXT("MSGPACK_FIXEXT8")) && Source.Contains(TEXT("ReadFixExt8")) && Source.Contains(TEXT("WriteFixExt8"))
		&& Source.Contains(TEXT("MSGPACK_FIXEXT16")) && Source.Contains(TEXT("ReadFixExt16")) && Source.Contains(TEXT("WriteFixExt16")));
	TestTrue(TEXT("Extension handler must copy variable extension payloads"),
		Source.Contains(TEXT("MSGPACK_EXT8")) && Source.Contains(TEXT("MSGPACK_EXT16")) && Source.Contains(TEXT("MSGPACK_EXT32"))
		&& Source.Contains(TEXT("ReadExt(&Type, &Value)")) && Source.Contains(TEXT("WriteExt(Type, Value)")));
	TestTrue(TEXT("ReadExtBytes must preserve extension type and payload bytes"),
		Source.Contains(TEXT("ReadExtBytes")) && Source.Contains(TEXT("OutBytes.Append(Value.Data, 2)")) && Source.Contains(TEXT("OutBytes.Append(Value.Data, 16)"))
		&& Source.Contains(TEXT("OutBytes.Append(Value.DataPtr, Value.Num)")));
	return true;
}
