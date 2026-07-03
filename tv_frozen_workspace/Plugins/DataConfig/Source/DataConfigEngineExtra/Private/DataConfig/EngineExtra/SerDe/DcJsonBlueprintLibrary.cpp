#include "DataConfig/EngineExtra/SerDe/DcJsonBlueprintLibrary.h"
#include "DataConfig/Serialize/DcSerializer.h"
#include "DataConfig/Serialize/DcSerializerSetup.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"
#include "DataConfig/Serialize/DcSerializeUtils.h"
#include "DataConfig/Serialize/Handlers/Common/DcCommonSerializers.h"
#include "DataConfig/Deserialize/DcDeserializer.h"
#include "DataConfig/Deserialize/DcDeserializerSetup.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Deserialize/DcDeserializeUtils.h"
#include "DataConfig/Deserialize/Handlers/Common/DcCommonDeserializers.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Property/DcPropertyWriter.h"
#include "DataConfig/Json/DcJsonReader.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/SerDe/DcSerDeUtils.h"
#include "DataConfig/DcEnv.h"

#include "DataConfig/Extra/SerDe/DcSerDeAnyStruct.h"
#include "DataConfig/Extra/SerDe/DcSerDeBase64.h"
#include "DataConfig/Extra/SerDe/DcSerDeInlineStruct.h"
#include "DataConfig/Extra/Types/DcExtraTestFixtures.h"
#include "DataConfig/Extra/Misc/DcTestCommon.h"

#include "DataConfig/EngineExtra/SerDe/DcSerDeBlueprint.h"
#include "DataConfig/EngineExtra/SerDe/DcSerDeGameplayTags.h"
#if ENGINE_MAJOR_VERSION == 5
#include "DataConfig/EngineExtra/SerDe/DcSerDeInstancedStruct.h"
#endif // ENGINE_MAJOR_VERSION == 5

#include "DataConfig/Automation/DcAutomation.h"
#include "DataConfig/Automation/DcAutomationUtils.h"

#if ENGINE_MAJOR_VERSION == 5

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
#include "Blueprint/BlueprintExceptionInfo.h"
#endif // UE_VERSION_OLDER_THAN(5, 4, 0)

#else
#include "UObject/Script.h"
#endif // ENGINE_MAJOR_VERSION == 5

#include "Engine/UserDefinedEnum.h"
#include "GameplayTags.h"
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
#include "UObject/VerseValueProperty.h"
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

#if UE_VERSION_OLDER_THAN(5, 5, 0)
#include "Engine/UserDefinedStruct.h"
#else
#include "StructUtils/UserDefinedStruct.h"
#endif // UE_VERSION_OLDER_THAN(5, 5, 0)

namespace DcJsonBlueprintLibraryDetails
{

static TOptional<FDcSerializer> BPValueSerializer;
static TOptional<FDcSerializer> BPObjectSerializer;

static TOptional<FDcDeserializer> BPValueDeserializer;
static TOptional<FDcDeserializer> BPObjectDeserializer;

static void LazyInitializeSerializer(TOptional<FDcSerializer>& Serializer, bool bRoot)
{
	if (Serializer.IsSet())
		return;

	Serializer.Emplace();
	DcSetupJsonSerializeHandlers(Serializer.GetValue());
	DcSetupCoreTypesSerializeHandlers(Serializer.GetValue());

	if (bRoot)
	{
		Serializer->AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(DcSerializeUtils::PredicateIsRootProperty),
			FDcSerializeDelegate::CreateStatic(DcCommonHandlers::HandlerClassToMapSerialize)
		);
	}

	//	Extra
	{
		using namespace DcExtra;

#if WITH_EDITORONLY_DATA
		Serializer->AddPredicatedHandler(FDcSerializePredicate::CreateStatic(PredicateIsBase64Blob), FDcSerializeDelegate::CreateStatic(HandleBase64BlobSerialize));
#endif // WITH_EDITORONLY_DATA

		Serializer->AddStructHandler(TBaseStructure<FDcAnyStruct>::Get(), FDcSerializeDelegate::CreateStatic(HandlerDcAnyStructSerialize));

		using Inline64 = TDcInlineStructSerialize<FDcInlineStruct64>;
		Serializer->AddStructHandler(Inline64::StaticStruct(), FDcSerializeDelegate::CreateStatic(Inline64::HandlerDcInlineStructSerialize));

		using Inline128 = TDcInlineStructSerialize<FDcInlineStruct128>;
		Serializer->AddStructHandler(Inline128::StaticStruct(), FDcSerializeDelegate::CreateStatic(Inline128::HandlerDcInlineStructSerialize));

		using Inline256 = TDcInlineStructSerialize<FDcInlineStruct256>;
		Serializer->AddStructHandler(Inline256::StaticStruct(), FDcSerializeDelegate::CreateStatic(Inline256::HandlerDcInlineStructSerialize));

		using Inline512 = TDcInlineStructSerialize<FDcInlineStruct512>;
		Serializer->AddStructHandler(Inline512::StaticStruct(), FDcSerializeDelegate::CreateStatic(Inline512::HandlerDcInlineStructSerialize));
	}

	//	EngineExtra
	{
		using namespace DcEngineExtra;

		Serializer->FieldClassSerializerMap[FClassProperty::StaticClass()] = FDcSerializeDelegate::CreateStatic(HandlerBPClassReferenceSerialize);
		Serializer->FieldClassSerializerMap[FObjectProperty::StaticClass()] = FDcSerializeDelegate::CreateStatic(HandlerBPObjectReferenceSerialize);
		Serializer->FieldClassSerializerMap[FStructProperty::StaticClass()] = FDcSerializeDelegate::CreateStatic(HandlerBPStructSerialize);

		Serializer->PredicatedSerializers.FindByPredicate([](auto& Entry){
			return Entry.Name == FName(TEXT("Enum"));
		})->Handler = FDcSerializeDelegate::CreateStatic(HandlerBPEnumSerialize);

		Serializer->AddDirectHandler(UUserDefinedStruct::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerBPStructSerialize));

		Serializer->AddStructHandler(TBaseStructure<FGameplayTag>::Get(), FDcSerializeDelegate::CreateStatic(HandlerGameplayTagSerialize));
		Serializer->AddStructHandler(TBaseStructure<FGameplayTagContainer>::Get(), FDcSerializeDelegate::CreateStatic(HandlerGameplayTagContainerSerialize));
	}

#if ENGINE_MAJOR_VERSION == 5
	//	EngineExtra5
	{
		using namespace DcExtra;

		Serializer->AddStructHandler(TBaseStructure<FInstancedStruct>::Get(), FDcSerializeDelegate::CreateStatic(HandlerInstancedStructSerialize));
	}
#endif // ENGINE_MAJOR_VERSION == 5

}

static void LazyInitializeDeserializer(TOptional<FDcDeserializer>& Deserializer, bool bRoot)
{
	if (Deserializer.IsSet())
		return;

	Deserializer.Emplace();
	DcSetupJsonDeserializeHandlers(Deserializer.GetValue());
	DcSetupCoreTypesDeserializeHandlers(Deserializer.GetValue());

	if (bRoot)
	{
		Deserializer->AddPredicatedHandler(
			FDcDeserializePredicate::CreateStatic(DcDeserializeUtils::PredicateIsRootProperty),
			FDcDeserializeDelegate::CreateStatic(DcCommonHandlers::HandlerMapToClassDeserialize)
		);
	}

	//	Extra
	{
		using namespace DcExtra;

#if WITH_EDITORONLY_DATA
		Deserializer->AddPredicatedHandler(FDcDeserializePredicate::CreateStatic(PredicateIsBase64Blob), FDcDeserializeDelegate::CreateStatic(HandleBase64BlobDeserialize));
#endif // WITH_EDITORONLY_DATA

		Deserializer->AddStructHandler(TBaseStructure<FDcAnyStruct>::Get(), FDcDeserializeDelegate::CreateStatic(HandlerDcAnyStructDeserialize));

		using Inline64 = TDcInlineStructDeserialize<FDcInlineStruct64>;
		Deserializer->AddStructHandler(Inline64::StaticStruct(), FDcDeserializeDelegate::CreateStatic(Inline64::HandlerDcInlineStructDeserialize));

		using Inline128 = TDcInlineStructDeserialize<FDcInlineStruct128>;
		Deserializer->AddStructHandler(Inline128::StaticStruct(), FDcDeserializeDelegate::CreateStatic(Inline128::HandlerDcInlineStructDeserialize));

		using Inline256 = TDcInlineStructDeserialize<FDcInlineStruct256>;
		Deserializer->AddStructHandler(Inline256::StaticStruct(), FDcDeserializeDelegate::CreateStatic(Inline256::HandlerDcInlineStructDeserialize));

		using Inline512 = TDcInlineStructDeserialize<FDcInlineStruct512>;
		Deserializer->AddStructHandler(Inline512::StaticStruct(), FDcDeserializeDelegate::CreateStatic(Inline512::HandlerDcInlineStructDeserialize));
	}

	//	EngineExtra
	{
		using namespace DcEngineExtra;

		Deserializer->FieldClassDeserializerMap[FClassProperty::StaticClass()] = FDcDeserializeDelegate::CreateStatic(HandlerBPClassReferenceDeserialize);
		Deserializer->FieldClassDeserializerMap[FObjectProperty::StaticClass()] = FDcDeserializeDelegate::CreateStatic(HandlerBPObjectReferenceDeserialize);

		Deserializer->PredicatedDeserializers.FindByPredicate([](auto& Entry){
			return Entry.Name == FName(TEXT("Enum"));
		})->Handler = FDcDeserializeDelegate::CreateStatic(HandlerBPEnumDeserialize);

		Deserializer->AddDirectHandler(UUserDefinedStruct::StaticClass(), FDcDeserializeDelegate::CreateStatic(DcCommonHandlers::HandlerMapToStructDeserialize));

		Deserializer->AddStructHandler(TBaseStructure<FGameplayTag>::Get(), FDcDeserializeDelegate::CreateStatic(HandlerGameplayTagDeserialize));
		Deserializer->AddStructHandler(TBaseStructure<FGameplayTagContainer>::Get(), FDcDeserializeDelegate::CreateStatic(HandlerGameplayTagContainerDeserialize));
	}

#if ENGINE_MAJOR_VERSION == 5
	//	EngineExtra5
	{
		using namespace DcExtra;

		Deserializer->AddStructHandler(TBaseStructure<FInstancedStruct>::Get(), FDcDeserializeDelegate::CreateStatic(HandlerInstancedStructDeserialize));
	}
#endif // ENGINE_MAJOR_VERSION == 5
}


static bool _BlueprintLibPropertyPredicate(FProperty* Property)
{
#if WITH_EDITORONLY_DATA
	if (Property->HasMetaData(DcPropertyUtils::DC_META_SKIP))
		return false;
#endif // WITH_EDITORONLY_DATA

	if (Property->IsA<FInterfaceProperty>()
		|| Property->IsA<FDelegateProperty>()
		|| Property->IsA<FMulticastInlineDelegateProperty>()
		|| Property->IsA<FMulticastSparseDelegateProperty>()
#if !UE_VERSION_OLDER_THAN(5, 4, 0)

#if UE_VERSION_OLDER_THAN(5, 5, 0)
		|| Property->IsA<FVerseValueProperty>()
#else

#if WITH_VERSE_VM
		|| Property->IsA<FVValueProperty>()
#endif // WITH_VERSE_VM

#endif // UE_VERSION_OLDER_THAN(5, 5, 0)

#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)
		)
		return false;

	return true;
}

static bool _BlueprintLibShouldExpandSubObject(FObjectProperty* ObjectProperty)
{
	//	DataConfig have very limited cycle control and support, properly supporting
	//	these is hard and adds overhead on top of everything. For now just skip well
	//	known classes that'll cause it it overflow.
	if (ObjectProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
		return false;

	return DcPropertyUtils::IsSubObjectProperty(ObjectProperty);
}

FDcPropertyConfig MakeBPPropertyConfig()
{
	FDcPropertyConfig Ret;

	Ret.ProcessPropertyPredicate = FDcProcessPropertyPredicateDelegate::CreateStatic(_BlueprintLibPropertyPredicate);
	Ret.ExpandObjectPredicate = FDcExpandObjectPredicateDelegate::CreateStatic(_BlueprintLibShouldExpandSubObject);

	return Ret;
}

static FDcResult _Dump(bool bRoot, TOptional<FDcSerializer>& Serializer, FDcPropertyDatum Datum, FString& OutJsonString)
{
	LazyInitializeSerializer(Serializer, bRoot);

	FDcPropertyReader Reader(Datum);
	DC_TRY(Reader.SetConfig(MakeBPPropertyConfig()));

	FDcJsonWriter Writer;

	FDcSerializeContext Ctx;
	Ctx.Reader = &Reader;
	Ctx.Writer = &Writer;
	Ctx.Serializer = &Serializer.GetValue();

	DC_TRY(Ctx.Prepare());
	DC_TRY(Ctx.Serializer->Serialize(Ctx));

	OutJsonString = Writer.Sb.ToString();
	return DcOk();
}

static FDcResult _Load(bool bRoot, TOptional<FDcDeserializer>& Deserializer, FDcPropertyDatum Datum, const FString& InJsonString)
{
	LazyInitializeDeserializer(Deserializer, bRoot);

	FDcJsonReader Reader(InJsonString);

	FDcPropertyWriter Writer(Datum);
	DC_TRY(Writer.SetConfig(MakeBPPropertyConfig()));

	FDcDeserializeContext Ctx;
	Ctx.Reader = &Reader;
	Ctx.Writer = &Writer;
	Ctx.Deserializer = &Deserializer.GetValue();

	DC_TRY(Ctx.Prepare());
	DC_TRY(Ctx.Deserializer->Deserialize(Ctx));

	return DcOk();
}

}

DEFINE_FUNCTION(UDcJsonBlueprintLibrary::execDumpValue)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	PARAM_PASSED_BY_REF(OutJsonString, FStrProperty, FString);

	P_FINISH;

	if (!ValueProperty || !ValuePtr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			NSLOCTEXT("DataConfigEngineExtra", "DcEditorExtra_MissingInputProperty_DumpValue", "Failed to resolve the input parameter for DumpValue.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult = false;
	if (ValueProperty && ValuePtr)
	{
		using namespace DcJsonBlueprintLibraryDetails;

		P_NATIVE_BEGIN
		bResult = _Dump(
			false, BPValueSerializer,
			FDcPropertyDatum(ValueProperty, ValuePtr),
			OutJsonString
		).Ok();
		if (!bResult)
		{
			DcEnv().FlushDiags();
		}
		P_NATIVE_END
	}

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}

DEFINE_FUNCTION(UDcJsonBlueprintLibrary::execDumpObject)
{
	P_GET_OBJECT(UObject, Obj);
	PARAM_PASSED_BY_REF(OutJsonString, FStrProperty, FString);

	P_FINISH;

	if (!Obj)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			NSLOCTEXT("DataConfigEngineExtra", "DcEditorExtra_MissingInputObject_DumpObject", "Failed to resolve the input Object for DumpObject.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult = false;
	if (Obj)
	{
		using namespace DcJsonBlueprintLibraryDetails;

		P_NATIVE_BEGIN
		bResult = _Dump(
			true, BPObjectSerializer,
			FDcPropertyDatum(Obj),
			OutJsonString
		).Ok();
		if (!bResult)
		{
			DcEnv().FlushDiags();
		}
		P_NATIVE_END
	}

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}

DEFINE_FUNCTION(UDcJsonBlueprintLibrary::execLoadValue)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, InJsonString);

	P_FINISH;

	if (!ValueProperty || !ValuePtr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			NSLOCTEXT("DataConfigEngineExtra", "DcEditorExtra_MissingInputProperty_LoadValue", "Failed to resolve the input parameter for LoadValue.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult = false;
	if (ValueProperty && ValuePtr)
	{
		using namespace DcJsonBlueprintLibraryDetails;

		P_NATIVE_BEGIN
		bResult = _Load(
			false, BPValueDeserializer,
			FDcPropertyDatum(ValueProperty, ValuePtr),
			InJsonString
		).Ok();
		if (!bResult)
		{
			DcEnv().FlushDiags();
		}
		P_NATIVE_END
	}

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}


DEFINE_FUNCTION(UDcJsonBlueprintLibrary::execLoadObject)
{
	P_GET_OBJECT(UObject, Obj);
	P_GET_PROPERTY(FStrProperty, InJsonString);

	P_FINISH;

	if (!Obj)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			NSLOCTEXT("DataConfigEngineExtra", "DcEditorExtra_MissingInputObject_LoadObject", "Failed to resolve the input Object for LoadObject.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult = false;
	if (Obj)
	{
		using namespace DcJsonBlueprintLibraryDetails;

		P_NATIVE_BEGIN
		bResult = _Load(
			true, BPObjectDeserializer,
			FDcPropertyDatum(Obj),
			InJsonString
		).Ok();
		if (!bResult)
		{
			DcEnv().FlushDiags();
		}
		P_NATIVE_END
	}

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}

DC_TEST("DataConfig.EditorExtra.JsonBlueprintLibrary")
{
	UClass* BPLibClass = UDcJsonBlueprintLibrary::StaticClass();

	struct FStructDumpObjectParams
	{
		UObject* Obj;
		FString JsonString;
		bool bResult;
	};

	UFunction* DumpObjectFunc = BPLibClass->FindFunctionByName(TEXT("DumpObject"));
	UTEST_NOT_NULL("JsonBlueprintLibrary", DumpObjectFunc);

	UDcShapeBox* Box = NewObject<UDcShapeBox>();
	Box->ShapeName = TEXT("MyBox");
	Box->Height = 12.3f;
	Box->Width = 23.4f;

	FStructDumpObjectParams Params{Box};

	BPLibClass->ProcessEvent(DumpObjectFunc, &Params);

	Params.JsonString += TCHAR('\n');
	UTEST_TRUE("JsonBlueprintLibrary", Params.bResult);
	UTEST_EQUAL("JsonBlueprintLibrary", Params.JsonString, DcAutomationUtils::DcReindentStringLiteral(TEXT(R"(

		{
		    "Height" : 12.3,
		    "Width" : 23.4,
		    "ShapeName" : "MyBox"
		}

	)")));

	UFunction* LoadObjectFunc = BPLibClass->FindFunctionByName(TEXT("LoadObject"));
	UTEST_NOT_NULL("JsonBlueprintLibrary", LoadObjectFunc);

	UDcShapeBox* Dest = NewObject<UDcShapeBox>();
	Params.Obj = Dest;
	Params.bResult = false;

	BPLibClass->ProcessEvent(LoadObjectFunc, &Params);

	UTEST_TRUE("JsonBlueprintLibrary", Params.bResult);
	UTEST_OK("JsonBlueprintLibrary", DcPropertyPipeVisitAndTestEqual(
		FDcPropertyDatum(Box),
		FDcPropertyDatum(Dest)
	));

	return true;
}



