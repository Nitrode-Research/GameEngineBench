#pragma once

#include "NativeGameplayTags.h"
#include "Settings/AlsxtDefensiveModeSettings.h"
#include "AlsxtCharacterSoundStructs.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FCharacterActionSoundCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Sex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag VoiceVariant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	float Stamina {0.0f};

	bool operator==(const FCharacterActionSoundCriteria& other) const
	{
		return (other.Action == Action) && (other.Sex == Sex) && (other.VoiceVariant == VoiceVariant) && (other.Strength == Strength) && (other.Stamina == Stamina);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FCharacterAttackSoundCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Type;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Sex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag VoiceVariant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTag Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	float Stamina {0.0f};

	bool operator==(const FCharacterAttackSoundCriteria& other) const
	{
		return (other.Type == Type) && (other.Sex == Sex) && (other.VoiceVariant == VoiceVariant) && (other.Strength == Strength) && (other.Stamina == Stamina);
	}
};