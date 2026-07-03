// Copyright 2026 GameDevBench. DataConfig AnyStruct source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DataConfigAnyStructTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& OutSource)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(OutSource, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigAnyStruct_OwnershipSource,
	"TargetVector.DataConfigAnyStruct.ownership_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigAnyStruct_OwnershipSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates type-erased ownership, shared reference semantics, move invalidation, and destruction.
	FString Header;
	FString Source;
	if (!TestTrue(TEXT("DcAnyStruct header must be readable"),
		DataConfigAnyStructTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Public/DataConfig/Extra/Types/DcAnyStruct.h"), Header))
		|| !TestTrue(TEXT("DcAnyStruct source must be readable"),
			DataConfigAnyStructTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Private/DataConfig/Extra/Types/DcAnyStruct.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("AnyStruct must define a reference controller with payload pointer and struct class"),
		Header.Contains(TEXT("AnyStructReferenceController")) && Header.Contains(TEXT("void* DataPtr")) && Header.Contains(TEXT("UScriptStruct* StructClass")));
	TestTrue(TEXT("AnyStruct must create shared controllers for raw pointer and typed constructors"),
		Header.Contains(TEXT("new AnyStructReferenceController(this)")) && Header.Contains(TEXT("TBaseStructure<T>::Get()")));
	TestTrue(TEXT("Copy semantics must share the same reference controller"),
		Header.Contains(TEXT("SharedReferenceCount(InOther.SharedReferenceCount)")) && Header.Contains(TEXT("FDcAnyStruct Temp = InOther")) && Header.Contains(TEXT("Swap(Temp, *this)")));
	TestTrue(TEXT("Move semantics must transfer reference count and invalidate source"),
		Header.Contains(TEXT("MoveTemp(InOther.SharedReferenceCount)")) && Header.Contains(TEXT("InOther.DataPtr = nullptr")) && Header.Contains(TEXT("InOther.StructClass = nullptr")));
	TestTrue(TEXT("Validity and reference count must reflect the shared referencer"),
		Header.Contains(TEXT("GetSharedReferenceCount")) && Header.Contains(TEXT("SharedReferenceCount.GetSharedReferenceCount()")) && Header.Contains(TEXT("SharedReferenceCount.IsValid()")));
	TestTrue(TEXT("Typed access must validate the requested struct type"),
		Header.Contains(TEXT("GetChecked")) && Header.Contains(TEXT("check(TBaseStructure<T>::Get() == StructClass)")));
	TestTrue(TEXT("Last-reference destruction must destroy and free owned struct memory"),
		Source.Contains(TEXT("DestroyStruct(DataPtr)")) && Source.Contains(TEXT("FMemory::Free(DataPtr)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigAnyStruct_DeserializeSource,
	"TargetVector.DataConfigAnyStruct.deserialize_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigAnyStruct_DeserializeSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates None handling, $type resolution, allocation/initialization, property-state push, and recursive deserialization.
	FString Source;
	if (!TestTrue(TEXT("DcSerDeAnyStruct source must be readable"),
		DataConfigAnyStructTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Private/DataConfig/Extra/SerDe/DcSerDeAnyStruct.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Deserialization must handle None by resetting the destination"),
		Source.Contains(TEXT("EDcDataEntry::None")) && Source.Contains(TEXT("ReadNone")) && Source.Contains(TEXT("AnyStructPtr->Reset()")));
	TestTrue(TEXT("Deserialization must require $type metadata"),
		Source.Contains(TEXT("EDcDataEntry::MapRoot")) && Source.Contains(TEXT("\"$type\"")) && Source.Contains(TEXT("ExpectMetaType")));
	TestTrue(TEXT("Deserialization must resolve the script struct type"),
		Source.Contains(TEXT("FuncLocateStruct")) && Source.Contains(TEXT("TryLocateObject")) && Source.Contains(TEXT("UScriptStruct* LoadStruct")));
	TestTrue(TEXT("Deserialization must allocate and initialize struct memory"),
		Source.Contains(TEXT("FMemory::Malloc")) && Source.Contains(TEXT("GetStructureSize")) && Source.Contains(TEXT("InitializeStruct")));
	TestTrue(TEXT("Deserialization must install the struct payload in the AnyStruct destination"),
		Source.Contains(TEXT("FDcAnyStruct TmpAny")) && Source.Contains(TEXT("*AnyStructPtr = MoveTemp(TmpAny)")));
	TestTrue(TEXT("Deserialization must push top struct property state and recursively deserialize through putback reader"),
		Source.Contains(TEXT("PushTopStructPropertyState")) && Source.Contains(TEXT("FDcPutbackReader")) && Source.Contains(TEXT("RecursiveDeserialize")));
	TestTrue(TEXT("Unexpected token types must report map-or-none mismatch"),
		Source.Contains(TEXT("DataTypeMismatch2")) && Source.Contains(TEXT("EDcDataEntry::MapRoot")) && Source.Contains(TEXT("EDcDataEntry::None")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigAnyStruct_SerializeSource,
	"TargetVector.DataConfigAnyStruct.serialize_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigAnyStruct_SerializeSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates $type emission, invalid None emission, recursive serialization, and public type-name formatting.
	FString Source;
	if (!TestTrue(TEXT("DcSerDeAnyStruct source must be readable"),
		DataConfigAnyStructTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Private/DataConfig/Extra/SerDe/DcSerDeAnyStruct.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Serialization must read the AnyStruct property datum"),
		Source.Contains(TEXT("ReadDataEntry(FStructProperty::StaticClass(), Datum)")) && Source.Contains(TEXT("FDcAnyStruct* AnyStructPtr")));
	TestTrue(TEXT("Serialization must write $type for valid AnyStruct values"),
		Source.Contains(TEXT("AnyStructPtr->IsValid()")) && Source.Contains(TEXT("WriteMapRoot")) && Source.Contains(TEXT("WriteString(TEXT(\"$type\"))")));
	TestTrue(TEXT("Serialization must use caller-provided type formatting and DataConfig default formatter"),
		Source.Contains(TEXT("FuncWriteStructType(AnyStructPtr->StructClass)")) && Source.Contains(TEXT("DcSerDeUtils::FormatObjectName")));
	TestTrue(TEXT("Serialization must push reflected struct payload state and recurse through putback writer"),
		Source.Contains(TEXT("PushTopStructPropertyState")) && Source.Contains(TEXT("FDcPutbackWriter")) && Source.Contains(TEXT("RecursiveSerialize")));
	TestTrue(TEXT("Serialization must write None for invalid AnyStruct values"),
		Source.Contains(TEXT("WriteNone")));
	return true;
}
