#pragma once

#include "NativeGameplayTags.h"
#include "Utility/AlsxtCollisionGameplayTags.h"
#include "Settings/AlsxtDefensiveModeSettings.h"
#include "AlsxtDefensiveModeState.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDefensiveModeState
{
	GENERATED_BODY()

	// Deprecate
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Mode {AlsxtPhysicalAnimationModeTags::None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ImpactMode{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AnticipationMode{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ObstacleMode{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform ObstacleTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Side", AllowPrivateAccess))
	FGameplayTag ObstacleSide{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Height", AllowPrivateAccess))
	FGameplayTag ObstacleHeight{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform AnticipationTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Side", AllowPrivateAccess))
	FGameplayTag AnticipationSide{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Height", AllowPrivateAccess))
	FGameplayTag AnticipationHeight{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTag AnticipationForm{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform ImpactTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Side", AllowPrivateAccess))
	FGameplayTag ImpactSide{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Height", AllowPrivateAccess))
	FGameplayTag ImpactHeight{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Form", AllowPrivateAccess))
	FGameplayTag ImpactForm{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Impact Velocity", AllowPrivateAccess))
	FGameplayTag ImpactVelocity{ FGameplayTag::EmptyTag };

	bool operator==(const FAlsxtDefensiveModeState& other) const
	{
		return (other.Mode == Mode) && (other.AnticipationMode == AnticipationMode) && (other.ObstacleMode == ObstacleMode) && (other.ObstacleSide == ObstacleSide) && (other.ObstacleHeight == ObstacleHeight) && (other.ObstacleTransform.GetLocation() == ObstacleTransform.GetLocation()) && (other.AnticipationSide == AnticipationSide) && (other.AnticipationHeight == AnticipationHeight) && (other.AnticipationTransform.GetRotation() == AnticipationTransform.GetRotation()) && (other.ImpactSide == ImpactSide) && (other.ImpactHeight == ImpactHeight) && (other.ImpactTransform.GetRotation() == ImpactTransform.GetRotation());
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDefensiveModeAnimations
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequenceBase> ImpactMontage {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtDefensivePoseSet ObstaclePoseSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtDefensivePoseStanceSet CrowdNavigationPoseSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtAnticipationPoseSet AnticipationPoseSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtDefensivePoseSet BraceForImpactPoseSet;

	bool operator==(const FAlsxtDefensiveModeAnimations& other) const
	{
		return (other.ImpactMontage == ImpactMontage) && (other.ObstaclePoseSet == ObstaclePoseSet) && (other.CrowdNavigationPoseSet == CrowdNavigationPoseSet) && (other.AnticipationPoseSet == AnticipationPoseSet) && (other.BraceForImpactPoseSet == BraceForImpactPoseSet);
	}
};

