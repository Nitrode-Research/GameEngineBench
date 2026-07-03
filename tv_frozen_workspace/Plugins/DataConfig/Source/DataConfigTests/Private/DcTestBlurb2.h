#pragma once

#include "CoreMinimal.h"
#include "DcTestProperty.h"
#include "DcTestBlurb2.generated.h"

USTRUCT()
struct FDcTestExample2
{
	GENERATED_BODY()

	UPROPERTY() FString StrField;
	UPROPERTY() FString StrArrField[3];

	UPROPERTY() UDcBaseShape* InlineField = nullptr;
	UPROPERTY() UDcTestClass1* RefField = nullptr;

	void MakeFixture();
};
