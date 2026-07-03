// Copyright 2026 GameDevBench. DataConfig property-path and JSON-converter source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DataConfigPropertyJsonTests
{
	static bool LoadSource(const TCHAR* RelativePath, FString& OutSource)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		return FFileHelper::LoadFileToString(OutSource, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigPropertyJson_PropertyPathSource,
	"TargetVector.DataConfigPropertyJson.property_path_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigPropertyJson_PropertyPathSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates path parsing, reflected traversal over structs/classes/containers/maps, diagnostics, and helper APIs.
	FString Header;
	FString Source;
	if (!TestTrue(TEXT("Property-path header must be readable"),
		DataConfigPropertyJsonTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Public/DataConfig/Extra/Types/DcPropertyPathAccess.h"), Header))
		|| !TestTrue(TEXT("Property-path source must be readable"),
			DataConfigPropertyJsonTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Private/DataConfig/Extra/Types/DcPropertyPathAccess.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Traversal must split dot-delimited path segments"),
		Source.Contains(TEXT("_SplitPath")) && Source.Contains(TEXT("FindChar(TCHAR('.'), Ix)")) && Source.Contains(TEXT("Remaining")));
	TestTrue(TEXT("Traversal must support struct and class roots"),
		Source.Contains(TEXT("ReadStructRoot")) && Source.Contains(TEXT("ReadClassRootAccess")) && Source.Contains(TEXT("PeekReadProperty")));
	TestTrue(TEXT("Traversal must match reflected field names and skip non-matching fields"),
		Source.Contains(TEXT("ReadName(&FieldName)")) && Source.Contains(TEXT("FieldName == Cur")) && Source.Contains(TEXT("SkipRead")));
	TestTrue(TEXT("Traversal must report missing fields through diagnostics"),
		Source.Contains(TEXT("CantFindPropertyByName")) && Source.Contains(TEXT("FormatHighlight")));
	TestTrue(TEXT("Traversal must support arrays and sets through numeric indexes"),
		Source.Contains(TEXT("ReadArrayRoot")) && Source.Contains(TEXT("ReadSetRoot")) && Source.Contains(TEXT("FCString::Strtoi")) && Source.Contains(TEXT("Index")));
	TestTrue(TEXT("Traversal must support map keys and reject non-name/string key tokens"),
		Source.Contains(TEXT("ReadMapRoot")) && Source.Contains(TEXT("ReadString(&FieldStr)")) && Source.Contains(TEXT("ReadName(&FieldName)")) && Source.Contains(TEXT("DataTypeMismatch2")));
	TestTrue(TEXT("Final datum lookup must read the selected reflected field"),
		Source.Contains(TEXT("GetDatumPropertyByPath")) && Source.Contains(TEXT("FDcPropertyReader Reader(Datum)"))
		&& Source.Contains(TEXT("Reader.ReadDataEntry(Property.ToFieldUnsafe()->GetClass(), OutDatum)")));
	TestTrue(TEXT("Header helpers must provide typed pointer and assignment access"),
		Header.Contains(TEXT("TEnableIf<DcTypeUtils::TIsUClass<T>::Value, T*>"))
		&& Header.Contains(TEXT("TEnableIf<DcTypeUtils::TIsUStruct<T>::Value, T*>"))
		&& Header.Contains(TEXT("TEnableIf<DcPropertyUtils::TIsInPropertyMap<T>::Value, T*>"))
		&& Header.Contains(TEXT("SetDatumPropertyByPath")) && Header.Contains(TEXT("SetPropertyValue")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigPropertyJson_JsonConverterPipelineSource,
	"TargetVector.DataConfigPropertyJson.json_converter_pipeline_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigPropertyJson_JsonConverterPipelineSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates DataConfig-backed JSON conversion, lazy serializer setup, camel-case root handling, and reader/writer usage.
	FString Header;
	FString Source;
	if (!TestTrue(TEXT("JSON converter header must be readable"),
		DataConfigPropertyJsonTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Public/DataConfig/Extra/Types/DcJsonConverter.h"), Header))
		|| !TestTrue(TEXT("JSON converter source must be readable"),
			DataConfigPropertyJsonTests::LoadSource(TEXT("Plugins/DataConfig/Source/DataConfigExtra/Private/DataConfig/Extra/Types/DcJsonConverter.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Public helpers must use FDcJsonReader and FDcJsonWriter"),
		Header.Contains(TEXT("FDcJsonReader Reader(JsonString)")) && Header.Contains(TEXT("FDcJsonWriter Writer")) && Header.Contains(TEXT("FDcJsonWriter::DefaultConfig")));
	TestTrue(TEXT("Conversion must lazily initialize JSON serializer and deserializer"),
		Source.Contains(TEXT("LazyInitializeSerializer")) && Source.Contains(TEXT("LazyInitializeDeserializer"))
		&& Source.Contains(TEXT("DcSetupJsonSerializeHandlers")) && Source.Contains(TEXT("DcSetupJsonDeserializeHandlers")));
	TestTrue(TEXT("Serialization must customize struct and class roots for standardized JSON field names"),
		Source.Contains(TEXT("HandlerStructRootSerializeCamelCase")) && Source.Contains(TEXT("HandlerClassRootSerializeCamelCase"))
		&& Source.Contains(TEXT("FJsonObjectConverter::StandardizeCase")));
	TestTrue(TEXT("Serializer maps must override struct/class handling"),
		Source.Contains(TEXT("UClassSerializerMap[UScriptStruct::StaticClass()]")) && Source.Contains(TEXT("UClassSerializerMap[UClass::StaticClass()]"))
		&& Source.Contains(TEXT("FieldClassSerializerMap[FStructProperty::StaticClass()]")));
	TestTrue(TEXT("JSON deserialization must route through FDcPropertyWriter and FDcDeserializeContext"),
		Source.Contains(TEXT("FDcPropertyWriter Writer(Datum)")) && Source.Contains(TEXT("FDcDeserializeContext Ctx")) && Source.Contains(TEXT("Deserializer->Deserialize(Ctx)")));
	TestTrue(TEXT("JSON serialization must route through FDcPropertyReader and FDcSerializeContext"),
		Source.Contains(TEXT("FDcPropertyReader Reader(Datum)")) && Source.Contains(TEXT("FDcSerializeContext Ctx")) && Source.Contains(TEXT("Serializer->Serialize(Ctx)")));
	TestTrue(TEXT("Converter tests must compare reflected datum equality after roundtrip"),
		Source.Contains(TEXT("DcAutomationUtils::TestReadDatumEqual")));
	return true;
}
