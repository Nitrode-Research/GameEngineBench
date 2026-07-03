#pragma once

#include "NativeGameplayTags.h"
#include "Settings/AlsxtFirearmSettings.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "TVFirearmStructs.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmRoundAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag Caliber{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag SonicType{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag NonLethal{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseVelocity { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BasePower { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	int DragCoEfficient { 700 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseMaxEffectiveRange { 550.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseMaxProjectileRangee { 900.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTagContainer Features{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	TObjectPtr<UALSXTFirearmRecoilAsset> FirearmRecoilAsset {nullptr};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmRound
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	TObjectPtr<UFirearmRoundAsset> RoundType {nullptr};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmRoundState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	float Emmission{0.0f};
	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmMagazineAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name {FGameplayTag::EmptyTag};
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Calibers {FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0), Category = "Format")
	int Capacity{ 8 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer CheckAmmoAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer RackAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer ReloadAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer ReloadWithRetentionAnimations{FGameplayTag::EmptyTag};

	// Conditions this weapon can be spawned in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	FGameplayTagContainer SpawnedConditions{FGameplayTag::EmptyTag};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmMagazine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmMagazineAsset> MagazineType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer Attachments {FGameplayTag::EmptyTag};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmMagazineState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	TArray<FFirearmRound> Rounds;
	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmComponentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type {FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Features{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Power { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Accuracy { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Distance { 1.0f };

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmComponentAsset> Component {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer Attachments {FGameplayTag::EmptyTag};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmComponentState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmAttachmentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Features{FGameplayTag::EmptyTag};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmRailAsset> RailType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer Attachments {FGameplayTag::EmptyTag};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmAttachmentState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int Rail{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmRailAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	int SlotMin{4};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	int SlotMax{35};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmRail
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmRailAsset> RailType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	int Slots{5};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	TArray<FFirearmAttachment> Attachments;
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmRailState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Location{FGameplayTag::EmptyTag};	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UOpticAttachmentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Features{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	TArray<float> Magnifications;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer AdjustmentAnimations{FGameplayTag::EmptyTag};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FOpticAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UOpticAttachmentAsset> OpticType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer Attachments {FGameplayTag::EmptyTag};
	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UGripAttachmentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag AttachmentType{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer MountTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FText FullName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight{ 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int AttachmentSlotLength{ 5 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int MountingSlotsStart{ 3 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int MountingSlotsEnd{ 4 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType{ SurfaceType_Default };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{ FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{ FGameplayTag::EmptyTag };

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	// FALSXTWeaponActionSound AdjustmentPoses;

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FGripAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UGripAttachmentAsset> GripType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer Attachments {FGameplayTag::EmptyTag};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FGripAttachmentState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int Position{0};	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmMuzzleAttachmentAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	FGameplayTag SonicType{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	TEnumAsByte<EPhysicalSurface> SurfaceType {SurfaceType_Default};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Features{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float FlashSuppression { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Power { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Accuracy { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float Distance { 1.0f };

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmMuzzleAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmMuzzleAttachmentAsset> MuzzleType {nullptr};
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmMuzzleAttachmentState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag SuppressionCondition{FGameplayTag::EmptyTag};	
};

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODEFIREARM_API UFirearmPlatformAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Name{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Type{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTag Overlay{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer Caiibers{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	FGameplayTagContainer FireModes{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTagContainer ViewModes{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	bool UseLeftHandIK {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Quality{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FGameplayTag Durability{FGameplayTag::EmptyTag};

	// Conditions this weapon can be spawned in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	FGameplayTagContainer SpawnedConditions{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseWeight { 550.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	int BaseRoundsPerMinute { 700 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseMaxEffectiveRange { 550.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	float BaseMaxProjectileRangee { 900.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer Components{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer EquipAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer UnequipAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer RackAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer MeleeAttackAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer ClearJamAnimations{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FGameplayTagContainer InspectAnimations{FGameplayTag::EmptyTag};

};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearm
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
	FGameplayTag Colorway{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	TObjectPtr<UFirearmPlatformAsset> FirearmType {nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Format")
	int Slots{5};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	TArray<FFirearmComponent> Components;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTagContainer RailInfo {FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	TArray<FFirearmAttachment> Attachments;
	
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FFirearmParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Condition{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FFirearmRound Chamber;
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODEFIREARM_API FGeneralFirearmSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Features", Meta = (AllowPrivateAccess))
	bool bEnable{ true };

};