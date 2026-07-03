#pragma once

#include "NativeGameplayTags.h"
#include "TVGameplayTags.h"
#include "Utility/ALSXTStructs.h"
#include "TVAudioStructs.generated.h"

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FPlaceholderStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Int {0};

	bool operator==(const FPlaceholderStruct& other) const
	{
		return (other.Int == Int);
	}
};