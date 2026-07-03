#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Property/DcPropertyReadStates.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticCommon.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"
#include "DataConfig/DcEnv.h"
#include "CoreMinimal.h"
#include "UObject/TextProperty.h"

#include "Misc/EngineVersionComparison.h"
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
#include "UObject/PropertyOptional.h"
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

static FORCEINLINE DcPropertyReaderDetails::FReadState::ImplStorageType* GetTopStorage(FDcPropertyReader* Self)
{
	return &Self->States.Top().ImplStorage;
}

static FORCEINLINE FDcBaseReadState& AsReadState(DcPropertyReaderDetails::FReadState::ImplStorageType* Storage)
{
	return *reinterpret_cast<FDcBaseReadState*>(Storage);
}

static FORCEINLINE FDcBaseReadState& GetTopState(FDcPropertyReader* Self)
{
	return AsReadState(GetTopStorage(Self));
}

template<typename TState>
static TState& GetTopState(FDcPropertyReader* Self) {
	return *GetTopState(Self).As<TState>();
}

template<typename TState>
static TState* TryGetTopState(FDcPropertyReader* Self) {
	return GetTopState(Self).As<TState>();
}

FDcReadStateNone& PushNoneState(FDcPropertyReader* Reader)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateNone>(GetTopStorage(Reader));
}

FDcReadStateClass& PushClassPropertyState(FDcPropertyReader* Reader, UObject* InClassObject, UClass* InClass, FDcReadStateClass::EType InType, const FName& InObjectName)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateClass>(GetTopStorage(Reader), InClassObject, InClass, InType, InObjectName);
}

FDcReadStateStruct& PushStructPropertyState(FDcPropertyReader* Reader, void* InStructPtr, UScriptStruct* InStructClass, const FName& InStructName)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateStruct>(GetTopStorage(Reader), InStructPtr, InStructClass, InStructName);
}

FDcReadStateMap& PushMappingPropertyState(FDcPropertyReader* Reader, void* InMapPtr, FMapProperty* InMapProperty)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateMap>(GetTopStorage(Reader), InMapPtr, InMapProperty);
}

FDcReadStateMap& PushMappingPropertyState(FDcPropertyReader* Reader, FProperty* InKeyProperty, FProperty* InValueProperty, void* InMap, EMapPropertyFlags InMapFlags)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateMap>(GetTopStorage(Reader), InKeyProperty, InValueProperty, InMap, InMapFlags);
}

FDcReadStateArray& PushArrayPropertyState(FDcPropertyReader* Reader, void* InArrayPtr, FArrayProperty* InArrayProperty)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateArray>(GetTopStorage(Reader), InArrayPtr, InArrayProperty);
}

FDcReadStateArray& PushArrayPropertyState(FDcPropertyReader* Reader, FProperty* InInnerProperty, void* InArray, EArrayPropertyFlags InArrayFlags)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateArray>(GetTopStorage(Reader), InInnerProperty, InArray, InArrayFlags);
}

FDcReadStateSet& PushSetPropertyState(FDcPropertyReader* Reader, void* InSetPtr, FSetProperty* InSetProperty)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateSet>(GetTopStorage(Reader), InSetPtr, InSetProperty);
}

FDcReadStateSet& PushSetPropertyState(FDcPropertyReader* Reader, FProperty* ElementProperty, void* InSet)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateSet>(GetTopStorage(Reader), ElementProperty, InSet);
}

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
FDcReadStateOptional& PushOptionalPropertyState(FDcPropertyReader* Reader, void* InOptionalPtr, FOptionalProperty* InOptionalProperty)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateOptional>(GetTopStorage(Reader), InOptionalPtr, InOptionalProperty);
}
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

FDcReadStateScalar& PushScalarPropertyState(FDcPropertyReader* Reader, void* InPtr, FProperty* InField)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateScalar>(GetTopStorage(Reader), InPtr, InField);
}

FDcReadStateScalar& PushScalarArrayPropertyState(FDcPropertyReader* Reader, void* InPtr, FProperty* InField)
{
	Reader->States.AddUninitialized();
	return Emplace<FDcReadStateScalar>(GetTopStorage(Reader), FDcReadStateScalar::Array, InPtr, InField);
}

void PopState(FDcPropertyReader* Reader)
{
	Reader->States.Pop();
	check(Reader->States.Num() >= 1);
}


template<typename TState>
static void PopState(FDcPropertyReader* Reader)
{
	check(TState::ID == GetTopState(Reader).GetType());
	PopState(Reader);
}

//	for read datum -> scalar
template<typename TProperty, typename TScalar>
void ReadPropertyValueConversion(FField* Property, void const* Ptr, TScalar* OutPtr)
{
	*OutPtr = (const TScalar&)(CastFieldChecked<TProperty>(Property)->GetPropertyValue(Ptr));
}

template<>
void ReadPropertyValueConversion<FBoolProperty, bool>(FField* Property, void const* Ptr, bool* OutPtr)
{
	*OutPtr = CastFieldChecked<FBoolProperty>(Property)->GetPropertyValue(Ptr);
}

template<typename TScalar>
FORCEINLINE FDcResult ReadTopStateScalarProperty(FDcPropertyReader* Self, TScalar* OutPtr)
{
	using TProperty = typename DcPropertyUtils::TPropertyTypeMap<TScalar>::Type;

	FDcPropertyDatum Datum;
	DC_TRY(GetTopState(Self).ReadDataEntry(Self, TProperty::StaticClass(), Datum));

	if (OutPtr)
	{
		check(!Datum.Property.IsUObject())
		ReadPropertyValueConversion<TProperty, TScalar>(Datum.Property.ToFieldUnsafe(), Datum.DataPtr, OutPtr);
	}

	return DcOk();
}

FDcPropertyReader::FDcPropertyReader()
{
	Config = FDcPropertyConfig::MakeDefault();
	PushNoneState(this);
}

FDcPropertyReader::FDcPropertyReader(FDcPropertyDatum Datum)
	: FDcPropertyReader()
{
	if (Datum.IsNone())
	{
		//	pass
	}
	else if (Datum.Property.IsA<UClass>())
	{
		UObject* Obj = (UObject*)(Datum.DataPtr);
		check(IsValid(Obj));
		PushClassPropertyState(
			this,
			Obj,
			Datum.CastUClassChecked(),
			FDcReadStateClass::EType::Root,
			Obj->GetFName()
		);
	}
	else if (Datum.Property.IsA<UScriptStruct>())
	{
		PushStructPropertyState(this, Datum.DataPtr, Datum.CastUScriptStructChecked(), FName(TEXT("$root")));
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
		PushScalarPropertyState(this, Datum.DataPtr, Datum.CastFieldChecked<FProperty>());
	}
}

FDcPropertyReader::FDcPropertyReader(EArrayReader, FProperty* InInnerProperty, void* InArray, EArrayPropertyFlags InArrayFlags)
	: FDcPropertyReader()
{
	PushArrayPropertyState(this, InInnerProperty, InArray, InArrayFlags);
}

FDcPropertyReader::FDcPropertyReader(ESetReader, FProperty* InElementProperty, void* InSet)
	: FDcPropertyReader()
{
	PushSetPropertyState(this, InElementProperty, InSet);
}

FDcPropertyReader::FDcPropertyReader(FProperty* InKeyProperty, FProperty* InValueProperty, void* InMap, EMapPropertyFlags InMapFlags)
	: FDcPropertyReader()
{
	PushMappingPropertyState(this, InKeyProperty, InValueProperty, InMap, InMapFlags);
}

FDcResult FDcPropertyReader::Coercion(EDcDataEntry ToEntry, bool* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::PeekRead(EDcDataEntry* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadBool(bool* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadString(FString* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadText(FText* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadEnum(FDcEnumData* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadName(FName* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadStructRootAccess(FDcStructAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadStructEndAccess(FDcStructAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadClassRootAccess(FDcClassAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadClassEndAccess(FDcClassAccess& Access)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadMapRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadMapEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadArrayRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadArrayEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadSetRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadSetEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadOptionalRoot()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadOptionalEnd()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadObjectReference(UObject** OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadClassReference(UClass** OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadWeakObjectReference(FWeakObjectPtr* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadLazyObjectReference(FLazyObjectPtr* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadInterfaceReference(FScriptInterface* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadSoftObjectReference(FSoftObjectPtr* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadSoftClassReference(FSoftObjectPtr* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadFieldPath(FFieldPath* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadDelegate(FScriptDelegate* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadMulticastInlineDelegate(FMulticastScriptDelegate* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadMulticastSparseDelegate(FMulticastScriptDelegate* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadInt8(int8* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadInt16(int16* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadInt32(int32* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadInt64(int64* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadUInt8(uint8* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadUInt16(uint16* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadUInt32(uint32* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadUInt64(uint64* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadFloat(float* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }
FDcResult FDcPropertyReader::ReadDouble(double* OutPtr) { return DC_FAIL(DcDCommon, NotImplemented); }

FDcResult FDcPropertyReader::ReadBlob(FDcBlobViewData* OutPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::SkipRead()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::PeekReadProperty(FFieldVariant* OutProperty)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::PeekReadDataPtr(void** OutDataPtr)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::ReadDataEntry(FFieldClass* ExpectedPropertyClass, FDcPropertyDatum& OutDatum)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::PushTopClassPropertyState(const FDcPropertyDatum& Datum)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcResult FDcPropertyReader::PushTopStructPropertyState(const FDcPropertyDatum& Datum, const FName& StructName)
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

bool FDcPropertyReader::IsReadingScalarArrayItem()
{
	if (FDcReadStateScalar* ScalarState = GetTopState(this).As<FDcReadStateScalar>())
		return ScalarState->State == FDcReadStateScalar::EState::ExpectArrayItem;

	return false;
}

FDcResult FDcPropertyReader::SetConfig(FDcPropertyConfig InConfig)
{
	Config = MoveTemp(InConfig);
	return Config.Prepare();
}

FDcResult FDcPropertyReader::ReadNone()
{
	return DC_FAIL(DcDCommon, NotImplemented);
}

FDcDiagnosticHighlight FDcPropertyReader::FormatHighlight()
{
	FDcDiagnosticHighlight OutHighlight(this, ClassId().ToString());
	TArray<FString> Segments;

	bool bLastIsContainer = false;
	int Num = States.Num();
	for (int Ix = 1; Ix < Num; Ix++)
	{
		FDcBaseReadState& ReadState = AsReadState(&States[Ix].ImplStorage);
		DcPropertyHighlight::EFormatSeg Seg;
		if (bLastIsContainer)
			Seg = DcPropertyHighlight::EFormatSeg::ParentIsContainer;
		else if (Ix == Num - 1)
			Seg = DcPropertyHighlight::EFormatSeg::Last;
		else
			Seg = DcPropertyHighlight::EFormatSeg::Normal;

		ReadState.FormatHighlightSegment(Segments, Seg);

		EDcPropertyReadType StateType = ReadState.GetType();
		bLastIsContainer = StateType == EDcPropertyReadType::ArrayProperty
			|| StateType == EDcPropertyReadType::SetProperty
			|| StateType == EDcPropertyReadType::MapProperty;
	}

	FString Path = Segments.Num() == 0
		? TEXT("<none>")
		: FString::Join(Segments, TEXT("."));
	OutHighlight.Formatted = FString::Printf(TEXT("Reading property: %s"), *Path);
	return OutHighlight;
}

void FDcPropertyReader::FormatDiagnostic(FDcDiagnostic& Diag)
{
	Diag << FormatHighlight();
}

FName FDcPropertyReader::ClassId() { return FName(TEXT("DcPropertyReader")); }
FName FDcPropertyReader::GetId() { return ClassId(); }
