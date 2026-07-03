#include "DataConfig/Serialize/DcSerializerSetup.h"
#include "DataConfig/MsgPack/DcMsgPackUtils.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/SerDe/DcSerDeUtils.h"
#include "DataConfig/Serialize/DcSerializer.h"
#include "DataConfig/Serialize/DcSerializeUtils.h"
#include "DataConfig/Serialize/Handlers/Common/DcCommonSerializers.h"
#include "DataConfig/Serialize/Handlers/Common/DcObjectSerializers.h"
#include "DataConfig/Serialize/Handlers/Common/DcCoreTypesSerializers.h"
#include "DataConfig/Serialize/Handlers/Property/DcPropertyPipeSerializers.h"
#include "DataConfig/Serialize/Handlers/MsgPack/DcMsgPackCommonSerializers.h"
#include "DataConfig/Serialize/Handlers/MsgPack/DcMsgPackTransientSerializers.h"
#include "UObject/TextProperty.h"
#include "Misc/EngineVersionComparison.h"
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
#include "UObject/PropertyOptional.h"
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

void DcSetupJsonSerializeHandlers(FDcSerializer& Serializer, EDcJsonSerializeType Type)
{
	using namespace DcCommonHandlers;
	{
		//	order significant
		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsScalarArrayProperty),
			FDcSerializeDelegate::CreateStatic(HandlerArraySerialize),
			FName(TEXT("Array"))
		);

		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsEnumProperty),
			FDcSerializeDelegate::CreateStatic(HandlerEnumToStringSerialize),
			FName(TEXT("Enum"))
		);

		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsSubObjectProperty),
			FDcSerializeDelegate::CreateStatic(HandlerInstancedSubObjectSerialize),
			FName(TEXT("SubObject"))
		);
	}

	AddNumericPipeDirectHandlers(Serializer);

	Serializer.AddDirectHandler(FBoolProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeBoolSerialize));
	Serializer.AddDirectHandler(FNameProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeNameSerialize));
	Serializer.AddDirectHandler(FStrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeStringSerialize));
	Serializer.AddDirectHandler(FTextProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeTextSerialize));
	Serializer.AddDirectHandler(FFieldPathProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerFieldPathToStringDeserialize));

	//	Containers
	Serializer.AddDirectHandler(FArrayProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerArraySerialize));
	Serializer.AddDirectHandler(FSetProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSetToArraySerialize));
	Serializer.AddDirectHandler(FMapProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStringKeyMapOrArrayOfKeyValueSerialize));

	//	Struct
	Serializer.AddDirectHandler(UScriptStruct::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructToMapSerialize));
	Serializer.AddDirectHandler(FStructProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructToMapSerialize));

	//	Class
	Serializer.AddDirectHandler(UClass::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassToMapSerialize));

	//	Object
	Serializer.AddDirectHandler(FObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerObjectReferenceSerialize));
	Serializer.AddDirectHandler(FClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassReferenceSerialize));

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3
	Serializer.AddDirectHandler(FObjectPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerObjectReferenceSerialize));
	Serializer.AddDirectHandler(FClassPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassReferenceSerialize));
#endif //ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3

	Serializer.AddDirectHandler(FWeakObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerWeakObjectReferenceSerialize));

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	Serializer.AddDirectHandler(FOptionalProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerOptionalSerialize));
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

	if (Type == EDcJsonSerializeType::Default)
	{
		Serializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftObjectReferenceSerialize));
		Serializer.AddDirectHandler(FSoftClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftClassReferenceSerialize));
		Serializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerLazyObjectReferenceSerialize));
	}
	else if (Type == EDcJsonSerializeType::StringSoftLazy)
	{
		Serializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftObjectToStringSerialize));
		Serializer.AddDirectHandler(FSoftClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftClassToStringSerialize));
		Serializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerLazyObjectToStringSerialize));
	}
	else
	{
		checkNoEntry();
	}
}

void DcSetupPropertyPipeSerializeHandlers(FDcSerializer& Serializer)
{
	{
		//	order significant
		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(DcCommonHandlers::PredicateIsScalarArrayProperty),
			FDcSerializeDelegate::CreateStatic(DcCommonHandlers::HandlerArraySerialize),
			FName(TEXT("Array"))
		);
	}

	using namespace DcPropertyPipeHandlers;

	Serializer.AddDirectHandler(UClass::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassSerialize));
	Serializer.AddDirectHandler(FObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassSerialize));
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3
	Serializer.AddDirectHandler(FObjectPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassSerialize));
#endif //ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3

	Serializer.AddDirectHandler(UScriptStruct::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructSerialize));
	Serializer.AddDirectHandler(FStructProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructSerialize));

	Serializer.AddDirectHandler(FArrayProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(DcCommonHandlers::HandlerArraySerialize));
	Serializer.AddDirectHandler(FSetProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSetSerialize));
	Serializer.AddDirectHandler(FMapProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerMapSerialize));

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	Serializer.AddDirectHandler(FOptionalProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerOptionalSerialize));
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

	DcPropertyUtils::VisitAllEffectivePropertyClass([&](FFieldClass* FieldClass) {
		if (!Serializer.FieldClassSerializerMap.Contains(FieldClass))
			Serializer.AddDirectHandler(FieldClass, FDcSerializeDelegate::CreateStatic(DcCommonHandlers::HandlerPipeScalarSerialize));
	});
}

void DcSetupMsgPackSerializeHandlers(FDcSerializer& Serializer, EDcMsgPackSerializeType Type)
{
	using namespace DcCommonHandlers;

	{
		//	order significant
		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsScalarArrayProperty),
			FDcSerializeDelegate::CreateStatic(HandlerArraySerialize),
			FName(TEXT("Array"))
		);
	}

	AddNumericPipeDirectHandlers(Serializer);

	Serializer.AddDirectHandler(FBoolProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeBoolSerialize));
	Serializer.AddDirectHandler(FStrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeStringSerialize));

	//	Containers
	Serializer.AddDirectHandler(FArrayProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerArraySerialize));
	Serializer.AddDirectHandler(FSetProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSetToArraySerialize));
	Serializer.AddDirectHandler(FMapProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(DcMsgPackHandlers::HandlerMapSerialize));

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	Serializer.AddDirectHandler(FOptionalProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerOptionalSerialize));
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

	//	Struct
	Serializer.AddDirectHandler(UScriptStruct::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructToMapSerialize));
	Serializer.AddDirectHandler(FStructProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerStructToMapSerialize));

	//	Class
	Serializer.AddDirectHandler(UClass::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassToMapSerialize));

	//	Blob
	Serializer.AddPredicatedHandler(
		FDcSerializePredicate::CreateStatic(DcMsgPackHandlers::PredicateIsBlobProperty),
		FDcSerializeDelegate::CreateStatic(DcMsgPackHandlers::HandlerBlobSerialize),
		FName(TEXT("Blob"))
	);

	if (Type == EDcMsgPackSerializeType::Default
		|| Type == EDcMsgPackSerializeType::StringSoftLazy)
	{
		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsSubObjectProperty),
			FDcSerializeDelegate::CreateStatic(HandlerInstancedSubObjectSerialize),
			FName(TEXT("SubObject"))
		);

		Serializer.AddPredicatedHandler(
			FDcSerializePredicate::CreateStatic(PredicateIsEnumProperty),
			FDcSerializeDelegate::CreateStatic(HandlerEnumToStringSerialize),
			FName(TEXT("Enum"))
		);

		Serializer.AddDirectHandler(FNameProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeNameSerialize));
		Serializer.AddDirectHandler(FTextProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerPipeTextSerialize));
		Serializer.AddDirectHandler(FFieldPathProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerFieldPathToStringDeserialize));

		Serializer.AddDirectHandler(FObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerObjectReferenceSerialize));
		Serializer.AddDirectHandler(FClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassReferenceSerialize));

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3
		Serializer.AddDirectHandler(FObjectPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerObjectReferenceSerialize));
		Serializer.AddDirectHandler(FClassPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerClassReferenceSerialize));
#endif //ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3

		Serializer.AddDirectHandler(FWeakObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerWeakObjectReferenceSerialize));

		if (Type == EDcMsgPackSerializeType::StringSoftLazy)
		{
			Serializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftObjectToStringSerialize));
			Serializer.AddDirectHandler(FSoftClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftClassToStringSerialize));
			Serializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerLazyObjectToStringSerialize));
		}
		else
		{
			Serializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftObjectReferenceSerialize));
			Serializer.AddDirectHandler(FSoftClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerSoftClassReferenceSerialize));
			Serializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerLazyObjectReferenceSerialize));
		}
	}
	else if (Type == EDcMsgPackSerializeType::InMemory)
	{
		using namespace DcMsgPackHandlers;
		Serializer.AddDirectHandler(FNameProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientNameSerialize));
		Serializer.AddDirectHandler(FTextProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientTextSerialize));
		Serializer.AddDirectHandler(FObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientObjectSerialize));
		Serializer.AddDirectHandler(FClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientClassSerialize));
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3
		Serializer.AddDirectHandler(FObjectPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientObjectSerialize));
		Serializer.AddDirectHandler(FClassPtrProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientClassSerialize));
#endif //ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3
		Serializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientSoftObjectSerialize));
		Serializer.AddDirectHandler(FSoftClassProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientSoftClassSerialize));
		Serializer.AddDirectHandler(FWeakObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientWeakObjectSerialize));
		Serializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientLazyObjectSerialize));
		Serializer.AddDirectHandler(FInterfaceProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientInterfaceSerialize));
		Serializer.AddDirectHandler(FDelegateProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientDelegateSerialize));
		Serializer.AddDirectHandler(FMulticastInlineDelegateProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientMulticastInlineDelegateSerialize));
		Serializer.AddDirectHandler(FMulticastSparseDelegateProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientMulticastSparseDelegateSerialize));
		Serializer.AddDirectHandler(FFieldPathProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientFieldPathSerialize));
		Serializer.AddDirectHandler(FEnumProperty::StaticClass(), FDcSerializeDelegate::CreateStatic(HandlerTransientEnumSerialize));
	}
	else
	{
		checkNoEntry();
	}
}

void DcSetupCoreTypesSerializeHandlers(FDcSerializer& Serializer)
{
	using namespace DcCoreTypesHandlers;

	Serializer.AddStructHandler(TBaseStructure<FGuid>::Get(), FDcSerializeDelegate::CreateStatic(HandlerGuidSerialize));
	Serializer.AddStructHandler(TBaseStructure<FColor>::Get(), FDcSerializeDelegate::CreateStatic(HandlerColorSerialize));
	Serializer.AddStructHandler(TBaseStructure<FDateTime>::Get(), FDcSerializeDelegate::CreateStatic(HandlerDateTimeSerialize));
	Serializer.AddStructHandler(DcSerDeUtils::FindFirstObject<UScriptStruct>(TEXT("Timespan"), true), FDcSerializeDelegate::CreateStatic(HandlerTimespanSerialize));

	Serializer.FieldClassSerializerMap[FTextProperty::StaticClass()] = FDcSerializeDelegate::CreateStatic(HandlerTextSerialize);

	Serializer.AddStructHandler(DcSerDeUtils::FindFirstObject<UScriptStruct>(TEXT("IntPoint"), true), FDcSerializeDelegate::CreateStatic(HandlerIntPointSerialize));
	Serializer.AddStructHandler(DcSerDeUtils::FindFirstObject<UScriptStruct>(TEXT("IntVector"), true), FDcSerializeDelegate::CreateStatic(HandlerIntVectorSerialize));
#if !UE_VERSION_OLDER_THAN(5, 1, 0)
	Serializer.AddStructHandler(DcSerDeUtils::FindFirstObject<UScriptStruct>(TEXT("IntVector4"), true), FDcSerializeDelegate::CreateStatic(HandlerIntVector4Serialize));
#endif // !UE_VERSION_OLDER_THAN(5, 1, 0)

#if ENGINE_MAJOR_VERSION == 5
	Serializer.AddStructHandler(TBaseStructure<FVector2D>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector2dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FVector2f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector2fSerialize));
	/// note there's no `FVector2d`
#else
	Serializer.AddStructHandler(TBaseStructure<FVector2D>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector2fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5


#if ENGINE_MAJOR_VERSION == 5
	Serializer.AddStructHandler(TBaseStructure<FVector>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FVector3f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FVector3d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3dSerialize));
#else
	Serializer.AddStructHandler(TBaseStructure<FVector>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

#if ENGINE_MAJOR_VERSION == 5
	Serializer.AddStructHandler(TBaseStructure<FVector4>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FVector4f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FVector4d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
#else
	Serializer.AddStructHandler(TBaseStructure<FVector4>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

#if ENGINE_MAJOR_VERSION == 5
	Serializer.AddStructHandler(TBaseStructure<FPlane>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FPlane4f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FPlane4d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
#else
	Serializer.AddStructHandler(TBaseStructure<FPlane>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

#if ENGINE_MAJOR_VERSION == 5
	Serializer.AddStructHandler(TBaseStructure<FMatrix>::Get(), FDcSerializeDelegate::CreateStatic(HandlerMatrix44dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FMatrix44f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerMatrix44fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FMatrix44d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerMatrix44dSerialize));
#else
	//	UE4 `FMatrix` ScriptStruct has to be loaded this way.
	Serializer.AddStructHandler(DcSerDeUtils::FindFirstObject<UScriptStruct>(TEXT("Matrix"), true), FDcSerializeDelegate::CreateStatic(HandlerMatrix44fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

#if ENGINE_MAJOR_VERSION == 5
	static_assert(sizeof(FRotator) == sizeof(FVector), "FRotator stale");
	static_assert(sizeof(FRotator3f) == sizeof(FVector3f), "FRotator stale");
	static_assert(sizeof(FRotator3d) == sizeof(FVector3d), "FRotator stale");
	Serializer.AddStructHandler(TBaseStructure<FRotator>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FRotator3f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FRotator3d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3dSerialize));
#else
	static_assert(sizeof(FRotator) == sizeof(FVector), "FRotator stale");
	Serializer.AddStructHandler(TBaseStructure<FRotator>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector3fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

#if ENGINE_MAJOR_VERSION == 5
	static_assert(sizeof(FQuat) == sizeof(FVector4), "FQuat stale");
	static_assert(sizeof(FQuat4f) == sizeof(FVector4f), "FQuat stale");
	static_assert(sizeof(FQuat4d) == sizeof(FVector4d), "FQuat stale");
	Serializer.AddStructHandler(TBaseStructure<FQuat>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
	Serializer.AddStructHandler(TVariantStructure<FQuat4f>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
	Serializer.AddStructHandler(TVariantStructure<FQuat4d>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4dSerialize));
#else
	static_assert(sizeof(FQuat) == sizeof(FVector4), "FQuat stale");
	Serializer.AddStructHandler(TBaseStructure<FQuat>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5


#if ENGINE_MAJOR_VERSION == 5
	static_assert(sizeof(FLinearColor) == sizeof(FVector4f), "FColor stale");
	Serializer.AddStructHandler(TBaseStructure<FLinearColor>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
#else
	static_assert(sizeof(FLinearColor) == sizeof(FVector4), "FColor stale");
	Serializer.AddStructHandler(TBaseStructure<FLinearColor>::Get(), FDcSerializeDelegate::CreateStatic(HandlerVector4fSerialize));
#endif // ENGINE_MAJOR_VERSION == 5

}
