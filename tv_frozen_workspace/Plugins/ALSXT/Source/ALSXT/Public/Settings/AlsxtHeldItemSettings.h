#pragma once

#include "Utility/AlsxtStructs.h"
#include "Animation/AnimSequence.h"
#include "AlsxtHeldItemSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGeneralHeldItemSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firearm", Meta = (AllowPrivateAccess))
	FName GripSocketName{ "Grip" };

};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtItemSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Type {FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Mass{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool Physics{ false };
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtItemSettings ItemSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Overlay {FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AvailableCombatStances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AvailableWeaponReadyPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DefaultWeaponReadyPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AvailableWeaponCarryPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AvailableGripPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool UsesLeftHandIK {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool Aimable{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool UsesRecoil{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool CanAttack{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool PhysicsWhenHeld{ false };
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemMontage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UAnimMontage> Montage { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UCurveFloat> BlendInCurve { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0))
	FVector2D StartTime{0.5f, 0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (ClampMin = 0))
	FVector2D PlayRate{1.0f, 1.0f};

	bool operator==(const FAlsxtHeldItemMontage& other) const
	{
		return (other.Montage == Montage) && (other.BlendInCurve == BlendInCurve) && (other.StartTime == StartTime) && (other.PlayRate == PlayRate);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemEquipMontage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag WeaponCarryPosition{FGameplayTag::EmptyTag};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FAlsxtHeldItemMontage Montage;

	bool operator==(const FAlsxtHeldItemEquipMontage& other) const
	{
		return (other.Montage == Montage);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemActionMontage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Type{FGameplayTag::EmptyTag};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FAlsxtHeldItemMontage Montage;

	bool operator==(const FAlsxtHeldItemActionMontage& other) const
	{
		return (other.Montage == Montage);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemFingerAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Animation{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firearm", Meta = (AllowPrivateAccess))
	TObjectPtr<UAnimMontage> Montage;

	bool operator==(const FAlsxtHeldItemFingerAnimation& other) const
	{
		return (other.Montage == Montage);
	}

};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemAttackMontage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Type{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Strength{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	float BaseStaminaCost{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FName AttackTraceStartSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FName AttackTraceEndSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FAlsxtHeldItemMontage Montage;

	bool operator==(const FAlsxtHeldItemAttackMontage& other) const
	{
		return (other.Montage == Montage);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemGrip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Type{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UAnimSequence> Pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FName HandSocketName{ "Grip" };

	bool operator==(const FAlsxtHeldItemGrip& other) const
	{
		return (other.Pose == Pose);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemGripState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtHeldItemGrip Grip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	FName AttachmentSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	FTransform GripTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parent")
	FGameplayTag GripPosition{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.Foregrip Position", TitleProperty = "{Position}", AllowPrivateAccess), Category = "Parent")
	FGameplayTag ComponentPosition;

	bool operator==(const FAlsxtHeldItemGripState& other) const
	{
		return (other.Grip == Grip) && (other.AttachmentSocketName == AttachmentSocketName);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemGripStates
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtHeldItemGripState Grip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtHeldItemGripState Foregrip;

	bool operator==(const FAlsxtHeldItemGripStates& other) const
	{
		return (other.Grip == Grip) && (other.Foregrip == Foregrip);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsxtHeldItemGripStates GripState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag FingerPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag FingerAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag WeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag WeaponAttack;

	bool operator==(const FAlsxtHeldItemState& other) const
	{
		return (other.GripState == GripState) && (other.FingerPosition == FingerPosition) && (other.FingerAction == FingerAction) && (other.WeaponAction == WeaponAction) && (other.WeaponAttack == WeaponAttack);
	}
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtHeldItemViewTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FOV {90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VignetteIntensity {0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Attachment {false};

	bool operator==(const FAlsxtHeldItemViewTarget& other) const
	{
		return (other.Transform.Equals(Transform, 9.99)) && (other.FOV == FOV) && (other.VignetteIntensity == VignetteIntensity) && (other.Attachment == Attachment);
	}
};