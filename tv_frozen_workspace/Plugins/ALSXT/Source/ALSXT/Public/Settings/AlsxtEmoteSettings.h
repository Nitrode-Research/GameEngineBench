#pragma once

#include "NativeGameplayTags.h"
#include "AlsxtEmoteSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtEmote
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
	TObjectPtr<UAnimMontage> Montage;
};

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtEmoteSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FAlsxtEmote> Emotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugMode {true};
};