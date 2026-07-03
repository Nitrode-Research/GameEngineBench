#pragma once

#include "GameplayTagContainer.h"
#include "AlsxtGestureSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGestureMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
	TObjectPtr<UAnimMontage> LeftMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
	TObjectPtr<UAnimMontage> RightMontage{ nullptr };
};

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtGestureSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ExitSectionName{ "Exit" };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FAlsxtGestureMontages> Gestures;
};