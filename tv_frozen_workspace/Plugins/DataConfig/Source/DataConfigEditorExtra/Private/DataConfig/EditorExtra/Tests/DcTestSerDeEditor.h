#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataConfig/Extra/DcExtraCommon.h"
#include "DcTestSerDeEditor.generated.h"

UCLASS()
class DATACONFIGEDITOREXTRA_API UDcEditorExtraNativeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY() FString StrField;
	UPROPERTY() int IntField = 0;
};

USTRUCT()
struct FDcEditorExtraTestObjectRefs1
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcEditorExtraTestObjectRefs1)

	UPROPERTY() UObject* ObjField1;
	UPROPERTY() UObject* ObjField2;
	UPROPERTY() UObject* ObjField3;
	UPROPERTY() UObject* ObjField4;
};


