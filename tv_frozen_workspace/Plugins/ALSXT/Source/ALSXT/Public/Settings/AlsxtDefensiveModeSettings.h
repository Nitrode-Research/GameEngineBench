#pragma once

#include "Engine/EngineTypes.h"
#include "Utility/AlsxtAnimationStructs.h"
#include "AlsxtDefensiveModeSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDefensiveModeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	TArray<TEnumAsByte<EObjectTypeQuery>> AnticipationTraceObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	TArray<TEnumAsByte<EObjectTypeQuery>> FallingAnticipationTraceObjectTypes;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDefensivePoseStanceSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtPoseSingleFrame Front;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtPoseSingleFrame Back;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtPoseSingleFrame Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtPoseSingleFrame Right;

	bool operator==(const FAlsxtDefensivePoseStanceSet& other) const
	{
		return (other.Front == Front) && (other.Back == Back) && (other.Left == Left) && (other.Right == Right);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAnticipationMovementModePoseSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet Low;

	bool operator==(const FAlsxtAnticipationMovementModePoseSet& other) const
	{
		return (other.High == High) && (other.Middle == Middle) && (other.Low == Low);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtObstacleCollisionAnticipationPoseSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet Standing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet Crouched;

	bool operator==(const FAlsxtObstacleCollisionAnticipationPoseSet& other) const
	{
		return (other.Standing == Standing) && (other.Crouched == Crouched);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDefensivePoseSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtAnticipationMovementModePoseSet Standing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseStanceSet Crouched;

	bool operator==(const FAlsxtDefensivePoseSet& other) const
	{
		return (other.Standing == Standing) && (other.Crouched == Crouched);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAnticipationPoseSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtDefensivePoseSet Grounded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtAnticipationMovementModePoseSet InAir;

	bool operator==(const FAlsxtAnticipationPoseSet& other) const
	{
		return (other.Grounded == Grounded) && (other.InAir == InAir);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAnticipationPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTagContainer Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTagContainer Form;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Locomotion Variant", AllowPrivateAccess))
	FGameplayTagContainer Variant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (TitleProperty = "{Standing} {Crouched} {InAir}", AllowPrivateAccess))
	FAlsxtAnticipationPoseSet Poses;

	bool operator==(const FAlsxtAnticipationPoses& other) const
	{
		return (other.Overlay == Overlay) && (other.Form == Form) && (other.Variant == Variant) && (other.Poses == Poses);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAvoidingPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTagContainer Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTagContainer Form;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Locomotion Variant", AllowPrivateAccess))
	FGameplayTagContainer Variant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (TitleProperty = "{Standing} {Crouched}", AllowPrivateAccess))
	FAlsxtDefensivePoseSet Poses;

	bool operator==(const FAlsxtAvoidingPoses& other) const
	{
		return (other.Overlay == Overlay) && (other.Form == Form) && (other.Variant == Variant) && (other.Poses == Poses);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtBraceForImpactPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTagContainer Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTagContainer Form;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Locomotion Variant", AllowPrivateAccess))
	FGameplayTagContainer Variant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (TitleProperty = "{High} {Middle} {Low}", AllowPrivateAccess))
	FAlsxtDefensivePoseSet Poses;

	bool operator==(const FAlsxtBraceForImpactPoses& other) const
	{
		return (other.Overlay == Overlay) && (other.Form == Form) && (other.Variant == Variant) && (other.Poses == Poses);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtObstacleCollisionAnticipationPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode", AllowPrivateAccess))
	FGameplayTagContainer Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTagContainer Form;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Locomotion Variant", AllowPrivateAccess))
	FGameplayTagContainer Variant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (TitleProperty = "{High} {Middle} {Low}", AllowPrivateAccess))
	FAlsxtObstacleCollisionAnticipationPoseSet Poses;

	bool operator==(const FAlsxtObstacleCollisionAnticipationPoses& other) const
	{
		return (other.Overlay == Overlay) && (other.Form == Form) && (other.Variant == Variant) && (other.Poses == Poses);
	}
};