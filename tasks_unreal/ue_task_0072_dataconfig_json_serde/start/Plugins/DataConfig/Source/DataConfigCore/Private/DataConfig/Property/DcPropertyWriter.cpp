#include "DataConfig/Property/DcPropertyWriter.h"
#include "DataConfig/Property/DcPropertyWriteStates.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticCommon.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"
#include "DataConfig/DcEnv.h"
#include "UObject/TextProperty.h"

#include "Misc/EngineVersionComparison.h"
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
#include "UObject/PropertyOptional.h"
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

static FORCEINLINE DcPropertyWriterDetails::FWriteState::ImplStorageType* GetTopStorage(FDcPropertyWriter* Self)
{
	return &Self->States.Top().ImplStorage;
}

static FORCEINLINE FDcBaseWriteState& AsWriteState(DcPropertyWriterDetails::FWriteState::ImplStorageType* Storage)
{
	return *reinterpret_cast<FDcBaseWriteState*>(Storage);
}

static FORCEINLINE FDcBaseWriteState& GetTopState(FDcPropertyWriter* Self)
{
	return AsWriteState(GetTopStorage(Self));
}

template<typename TState>
static TState& GetTopState(FDcPropertyWriter* Self) {
	return *GetTopState(Self).As<TState>();
}

template<typename TState>
static TState* TryGetTopState(FDcPropertyWriter* Self) {
	return GetTopState(Self).As<TState>();
}

static FDcWriteStateNone& PushNoneState(FDcPropertyWriter* Writer) {
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateNone>(GetTopStorage(Writer));
}

static FDcWriteStateClass& PushClassRootState(FDcPropertyWriter* Writer, UObject* InClassObject, UClass* InClass)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateClass>(GetTopStorage(Writer), InClassObject, InClass);
}

static FDcWriteStateClass& PushClassPropertyState(FDcPropertyWriter* Writer, void* InDataPtr, FObjectProperty* InObjProperty, FDcClassAccess::EControl InConfigControl)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateClass>(GetTopStorage(Writer), InDataPtr, InObjProperty, InConfigControl);
}

static FDcWriteStateStruct& PushStructPropertyState(FDcPropertyWriter* Writer, void* InStructPtr, UScriptStruct* InStructStruct, const FName& InStructName)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateStruct>(GetTopStorage(Writer), InStructPtr, InStructStruct, InStructName);
}

static FDcWriteStateMap& PushMappingPropertyState(FDcPropertyWriter* Writer, void* InMapPtr, FMapProperty* InMapProperty)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateMap>(GetTopStorage(Writer), InMapPtr, InMapProperty);
}

static FDcWriteStateMap& PushMappingPropertyState(FDcPropertyWriter* Writer, FProperty* InKeyProperty, FProperty* InValueProperty, void* InMap, EMapPropertyFlags InMapFlags)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateMap>(GetTopStorage(Writer), InKeyProperty, InValueProperty, InMap, InMapFlags);
}

static FDcWriteStateArray& PushArrayPropertyState(FDcPropertyWriter* Writer, void* InArrayPtr, FArrayProperty* InArrayProperty)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateArray>(GetTopStorage(Writer), InArrayPtr, InArrayProperty);
}

static FDcWriteStateArray& PushArrayPropertyState(FDcPropertyWriter* Writer, FProperty* InInnerProperty, void* InArray, EArrayPropertyFlags InArrayFlags)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateArray>(GetTopStorage(Writer), InInnerProperty, InArray, InArrayFlags);
}

static FDcWriteStateSet& PushSetPropertyState(FDcPropertyWriter* Writer, void* InSetPtr, FSetProperty* InSetProperty)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateSet>(GetTopStorage(Writer), InSetPtr, InSetProperty);
}

static FDcWriteStateSet& PushSetPropertyState(FDcPropertyWriter* Writer, FProperty* ElementProperty, void* InSet)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateSet>(GetTopStorage(Writer), ElementProperty, InSet);
}

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
static FDcWriteStateOptional& PushOptionalPropertyState(FDcPropertyWriter* Writer, void* InOptionalPtr, FOptionalProperty* InOptionalProperty)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateOptional>(GetTopStorage(Writer), InOptionalPtr, InOptionalProperty);
}
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

static FDcWriteStateScalar& PushScalarPropertyState(FDcPropertyWriter* Writer, void* InPtr, FProperty* InField)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateScalar>(GetTopStorage(Writer), InPtr, InField);
}

static FDcWriteStateScalar& PushScalarArrayPropertyState(FDcPropertyWriter* Writer, void* InPtr, FProperty* InField)
{
	Writer->States.AddUninitialized();
	return Emplace<FDcWriteStateScalar>(GetTopStorage(Writer), FDcWriteStateScalar::Array, InPtr, InField);
}

static void PopState(FDcPropertyWriter* Writer)
{
	Writer->States.Pop();
	check(Writer->States.Num() >= 1);
}

template<typename TState>
static void PopState(FDcPropertyWriter* Writer)
{
	check(TState::ID == GetTopState(Writer).GetType());
	PopState(Writer);
}

template<typename TScalar>
FORCEINLINE FDcResult WriteTopStateScalarProperty(FDcPropertyWriter* Self, const TScalar& Value)
{
	using TProperty = typename DcPropertyUtils::TPropertyTypeMap<TScalar>::Type;

	return WriteValue<TProperty, TScalar>(Self, GetTopState(Self), Value);
}

FDcPropertyWriter::FDcPropertyWriter()
{
	Config = FDcPropertyConfig::MakeDefault();
	PushNoneState(this);
}

FDcPropertyWriter::FDcPropertyWriter(FDcPropertyDatum Datum)
	: FDcPropertyWriter()
{
	if (Datum.IsNone())
	{
		//	pass
	}
	else if (Datum.Property.IsA<UClass>())
	{
		UObject* Obj = (UObject*)(Datum.DataPtr);
		check(IsValid(Obj));
		PushClassRootState(this, Obj, Datum.CastUClassChecked());
	}
	else if (Datum.Property.IsA<UScriptStruct>())
	{
		PushStructPropertyState(this,
			Datum.DataPtr,
			Datum.CastUScriptStructChecked(),
			FName(TEXT("$root"))
		);
	}
	else if (Datum.Property.IsA<FArrayProperty>())
	{
		PushArrayPropertyState(this, Datum.DataPtr, Datum.CastFieldChecked<FArrayProperty>());
	}
	else if (Datum.Property.IsA<FSetProperty>())
	{
		PushSetPropertyState(this, Datum.DataPtr, Datum.CastFieldChecked<FSetProperty>());
	}
	else if (Datum.Property.IsA<FMapProperty>())
	{
		PushMappingPropertyState(this, Datum.DataPtr, Datum.CastFieldChecked<FMapProperty>());
	}
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	else if (Datum.Property.IsA<FOptionalProperty>())
	{
		PushOptionalPropertyState(this, Datum.DataPtr, Datum.CastFieldChecked<FOptionalProperty>());
	}
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)
	else
	{
		PushScalarPropertyState(this, Datum.DataPtr, Datum.CastField<FProperty>());
	}
}

FDcPropertyWriter::FDcPropertyWriter(EArrayWriter, FProperty* InInnerProperty, void* InArray, EArrayPropertyFlags InArrayFlags)
	: FDcPropertyWriter()
{
	PushArrayPropertyState(this, InInnerProperty, InArray, InArrayFlags);
}

FDcPropertyWriter::FDcPropertyWriter(ESetWriter, FProperty* InElementProperty, void* InSet)
	: FDcPropertyWriter()
{
	PushSetPropertyState(this, InElementProperty, InSet);
}

FDcPropertyWriter::FDcPropertyWriter(FProperty* InKeyProperty, FProperty* InValueProperty, void* InMap, EMapPropertyFlags InMapFlags)
	: FDcPropertyWriter()
{
	PushMappingPropertyState(this, InKeyProperty, InValueProperty, InMap, InMapFlags);
}

FDcResult FDcPropertyWriter::PeekWrite(EDcDataEntry Next, bool* bOutOk)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteBool(bool Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteString(const FString& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteText(const FText& Value) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyWriter::WriteName(const FName& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteEnum(const FDcEnumData& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteStructRootAccess(FDcStructAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteStructEndAccess(FDcStructAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::PushTopClassPropertyState(const FDcPropertyDatum& Datum)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::PushTopStructPropertyState(const FDcPropertyDatum& Datum, const FName& StructName)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

bool FDcPropertyWriter::IsWritingScalarArrayItem()
{
	if (FDcWriteStateScalar* ScalarState = GetTopState(this).As<FDcWriteStateScalar>())
		return ScalarState->State == FDcWriteStateScalar::EState::ExpectArrayItem;

	return false;
}

FDcResult FDcPropertyWriter::SetConfig(FDcPropertyConfig InConfig)
{
	Config = MoveTemp(InConfig);
	return Config.Prepare();
}

FDcResult FDcPropertyWriter::WriteClassRootAccess(FDcClassAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteClassEndAccess(FDcClassAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteMapRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteMapEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteArrayRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteArrayEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteSetRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteSetEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteOptionalRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteOptionalEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteNone()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteObjectReference(const UObject* Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteClassReference(const UClass* Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteFieldPath(const FFieldPath& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteDelegate(const FScriptDelegate& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteMulticastInlineDelegate(const FMulticastScriptDelegate& Value) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyWriter::WriteMulticastSparseDelegate(const FMulticastScriptDelegate& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteWeakObjectReference(const FWeakObjectPtr& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteLazyObjectReference(const FLazyObjectPtr& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteInterfaceReference(const FScriptInterface& Value) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyWriter::WriteSoftObjectReference(const FSoftObjectPtr& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteSoftClassReference(const FSoftObjectPtr& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteInt8(const int8& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteInt16(const int16& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteInt32(const int32& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteInt64(const int64& Value) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyWriter::WriteUInt8(const uint8& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteUInt16(const uint16& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteUInt32(const uint32& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteUInt64(const uint64& Value) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyWriter::WriteFloat(const float& Value) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyWriter::WriteDouble(const double& Value) { return DC_FAIL(DcDCommon, NotImplemented); }


FDcResult FDcPropertyWriter::WriteBlob(const FDcBlobViewData& Value)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::SkipWrite()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::PeekWriteProperty(FFieldVariant* OutProperty)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyWriter::WriteDataEntry(FFieldClass* ExpectedPropertyClass, FDcPropertyDatum& OutDatum)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcDiagnosticHighlight FDcPropertyWriter::FormatHighlight()
{
	FDcDiagnosticHighlight OutHighlight(this, ClassId().ToString());
	TArray<FString> Segments;

	bool bLastIsContainer = false;
	int Num = States.Num();
	for (int Ix = 1; Ix < Num; Ix++)
	{
		FDcBaseWriteState& WriteState = AsWriteState(&States[Ix].ImplStorage);
		DcPropertyHighlight::EFormatSeg Seg;
		if (bLastIsContainer)
			Seg = DcPropertyHighlight::EFormatSeg::ParentIsContainer;
		else if (Ix == Num - 1)
			Seg = DcPropertyHighlight::EFormatSeg::Last;
		else
			Seg = DcPropertyHighlight::EFormatSeg::Normal;

		WriteState.FormatHighlightSegment(Segments, Seg);

		EDcPropertyWriteType StateType = WriteState.GetType();
		bLastIsContainer = StateType == EDcPropertyWriteType::ArrayProperty
			|| StateType == EDcPropertyWriteType::SetProperty
			|| StateType == EDcPropertyWriteType::MapProperty;
	}

	FString Path = Segments.Num() == 0
		? TEXT("<none>")
		: FString::Join(Segments, TEXT("."));
	OutHighlight.Formatted = FString::Printf(TEXT("Writing property: %s"), *Path);
	return OutHighlight;
}

void FDcPropertyWriter::FormatDiagnostic(FDcDiagnostic& Diag)
{
	Diag << FormatHighlight();
}

FName FDcPropertyWriter::ClassId() { return FName(TEXT("DcPropertyWriter")); }
FName FDcPropertyWriter::GetId() { return ClassId(); }
