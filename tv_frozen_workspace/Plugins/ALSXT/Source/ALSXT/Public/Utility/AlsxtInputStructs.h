#pragma once

#include "NativeGameplayTags.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Animation/AnimMontage.h"
#include "AlsxtInputStructs.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (AllowPrivateAccess))
	UInputAction* InputAction {nullptr};

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (AllowPrivateAccess))
	// FInputActionValue InputActionValue;

	bool operator==(const FAlsxtInputAction& other) const
	{
		return (other.InputAction == InputAction);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtInputComboEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (AllowPrivateAccess))
	TArray<FAlsxtInputAction> InputActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (Categories = "Als.Overlays", AllowPrivateAccess))
	FGameplayTagContainer Overlays;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (AllowPrivateAccess))
	UAnimMontage* Montage {nullptr};

	bool operator==(const FAlsxtInputComboEntry& other) const
	{
		return (other.InputActions == InputActions) && (other.Overlays == Overlays) && (other.Montage == Montage);
	}
};