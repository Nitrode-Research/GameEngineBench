#pragma once

#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"
#include "Engine/DataAsset.h"
#include "DataConfig/Extra/DcExtraCommon.h"
#include "DcSerDeBlueprint.generated.h"

///	Blueprint SerDe, including:
/// - Blueprint class/object reference
/// - Blueprint class instances
/// - Blueprint struct with FDcAnyStruct
/// - Blueprint enum

namespace DcEngineExtra {

DATACONFIGENGINEEXTRA_API FDcResult HandlerBPEnumSerialize(FDcSerializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerBPEnumDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGENGINEEXTRA_API FDcResult HandlerBPClassReferenceSerialize(FDcSerializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerBPClassReferenceDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGENGINEEXTRA_API FDcResult HandlerBPObjectReferenceSerialize(FDcSerializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerBPObjectReferenceDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGENGINEEXTRA_API FDcResult HandlerBPStructSerialize(FDcSerializeContext& Ctx);
// no `HandlerBPStructDeserialize`, use `DcCommonHandlers::HandlerMapToStructDeserialize` would be enough

DATACONFIGENGINEEXTRA_API FDcResult HandlerBPDcAnyStructSerialize(FDcSerializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerBPStructRootSerializeSkipEmptyAnyStruct(FDcSerializeContext& Ctx);

} // namespace DcEngineExtra


UCLASS(Blueprintable, BlueprintType)
class DATACONFIGENGINEEXTRA_API UDcTestBPClassBase : public UDataAsset
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	FString GetClassID();
	FString GetClassID_Implementation();
};

UCLASS()
class DATACONFIGENGINEEXTRA_API UDcTestNativeDerived1 : public UDcTestBPClassBase
{
	GENERATED_BODY()
};


USTRUCT()
struct DATACONFIGENGINEEXTRA_API FDcEngineExtraTestStructWithBPClass
{
	GENERATED_BODY()

	UPROPERTY() TSubclassOf<UDcTestBPClassBase> ClassField1;
	UPROPERTY() TSubclassOf<UDcTestBPClassBase> ClassField2;
	UPROPERTY() TSubclassOf<UDcTestBPClassBase> ClassField3;
};

USTRUCT()
struct DATACONFIGENGINEEXTRA_API FDcEngineExtraTestStructWithBPInstance
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcEngineExtraTestStructWithBPInstance)

	UPROPERTY() UDcTestBPClassBase* InstanceField1;
	UPROPERTY() UDcTestBPClassBase* InstanceField2;
	UPROPERTY() UDcTestBPClassBase* InstanceField3;
};

