#pragma once

#include "GameplayTags.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"
#include "DcSerDeGameplayTags.generated.h"

/// GameplayTag and GameplayTagContainer SerDe

namespace DcEngineExtra {

DATACONFIGENGINEEXTRA_API FDcResult HandlerGameplayTagDeserialize(FDcDeserializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerGameplayTagContainerDeserialize(FDcDeserializeContext& Ctx);

DATACONFIGENGINEEXTRA_API FDcResult HandlerGameplayTagSerialize(FDcSerializeContext& Ctx);
DATACONFIGENGINEEXTRA_API FDcResult HandlerGameplayTagContainerSerialize(FDcSerializeContext& Ctx);


} // namespace DcEngineExtra

USTRUCT()
struct FDcEngineExtraTestStructWithGameplayTag1
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTag TagField1;
	UPROPERTY() FGameplayTag TagField2;

};

USTRUCT()
struct FDcEngineExtraTestStructWithGameplayTag2
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTagContainer TagContainerField1;
	UPROPERTY() FGameplayTagContainer TagContainerField2;
};
