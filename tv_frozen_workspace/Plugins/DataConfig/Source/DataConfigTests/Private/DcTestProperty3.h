#pragma once

#include "CoreMinimal.h"
#include "DcTestProperty.h"
#include "DataConfig/Extra/DcExtraCommon.h"
#include "DcTestProperty3.generated.h"


USTRUCT()
struct FDcTestMeta1
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcTestMeta1)

	UPROPERTY(meta = (DcSkip)) int SkipField1;
};


USTRUCT()
struct FDcTestClassAccessControl
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcTestClassAccessControl)

	UPROPERTY() UDcBaseShape* InlineField;

	UPROPERTY() UDcTestClass1* RefField;
};
