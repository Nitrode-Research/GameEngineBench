#pragma once

#include "NativeGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "AlsxtCombatStructs.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAttackImpactCameraShake
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (Categories = "Als.Impact Velocity", AllowPrivateAccess))
	FGameplayTagContainer Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Criteria", Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTagContainer Form;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TSubclassOf<UCameraShakeBase> CameraShake{ nullptr };

	bool operator==(const FAlsxtAttackImpactCameraShake& other) const
	{
		return (other.Velocity == Velocity) && (other.Form == Form) && (other.CameraShake == CameraShake);
	}
};