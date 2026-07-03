// MIT

#pragma once

#include "CoreMinimal.h"
#include "Settings/AlsxtCharacterAdvancedSettings.h"
#include "AlsxtCharacter.h"
#include "Interfaces/AlsxtCharacterCameraEffectsComponentInterface.h"
#include "Interfaces/AlsxtHeadLookAtInterface.h"
#include "Interfaces/AlsxtAcrobaticActionComponentInterface.h"
#include "Interfaces/AlsxtCombatInterface.h"
#include "Interfaces/AlsxtFirearmInterface.h"
#include "AlsCameraComponent.h"
#include "CineCameraComponent.h"
#include "Components/Character/AlsxtCharacterCameraEffectsComponent.h"
#include "Components/Character/AlsxtCombatComponent.h"
#include "Components/Character/AlsxtCharacterSoundComponent.h"
#include "Components/Character/AlsxtAcrobaticActionComponent.h"
#include "Components/Character/AlsxtCharacterCameraEffectsComponent.h"
#include "AlsxtCharacterAdvanced.generated.h"

class UAlsxtCharacterAdvancedSettings;

UCLASS()
class ALSXT_API AAlsxtCharacterAdvanced : public AAlsxtCharacter, public IAlsxtCharacterCameraEffectsComponentInterface, public IAlsxtAcrobaticActionComponentInterface, public IAlsxtCombatInterface, public IAlsxtFirearmInterface
{
	GENERATED_BODY()

public:
	AAlsxtCharacterAdvanced(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Advanced", Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtCharacterAdvancedSettings> ALSXTAdvancedSettings;

	void CalcADSCamera(FMinimalViewInfo& ViewInfo);
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> HoldBreathAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> PrimaryActionAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SecondaryActionAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AcrobaticAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> TargetLockAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SwitchTargetLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input Actions", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SwitchTargetRightAction;

	// Components
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	class UAlsxtCombatComponent* Combat;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	class UAlsxtAcrobaticActionComponent* AcrobaticActions;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	class UAlsxtCharacterCameraEffectsComponent* CameraEffects;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State|Aim")
	FTransform GetADSRelativeTransform() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State|Aim")
	FRotator CalculateRecoilControlRotation(FRotator AdditiveControlRotation) const;

// INPUTS

private:
	void InputHoldBreath(const FInputActionValue& ActionValue);
	void InputAcrobaticAction(const FInputActionValue& ActionValue);
	void InputTargetLock(const FInputActionValue& ActionValue);
	void InputSwitchTargetLeft();
	void InputSwitchTargetRight();
	void InputChargeFirearm();
	void InputReload();
	void InputReloadWithRetention();
	void ClearWeaponMalfunction();

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Als|Input Actions")
	void InputPrimaryAction();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Als|Input Actions")
	void InputSecondaryAction();

// Input Action Criteria

protected:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Als|Input Actions")
	bool CanPerformPrimaryAction() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Als|Input Actions")
	bool CanPerformSecondaryAction() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Als|Input Actions")
	bool CanPerformAcrobaticAction() const;

	virtual void OnDesiredAimingChanged_Implementation(bool bPreviousDesiredAiming) override;

	
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsxtFirearmAimState FirearmAimState;

	// Timers

	FTimerHandle HoldBreathTimerHandle;	// Timer Handle for Hold Breath
	FTimerDelegate HoldBreathTimerDelegate; // Delegate to bind function with parameters

	FTimerHandle BreathRecoveryTimerHandle;	// Timer Handle for Hold Breath
	FTimerDelegate BreathRecoveryTimerDelegate; // Delegate to bind function with parameters

	// Hold Breath
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	float CalculateHoldBreathTimer();

	// Hold Breath
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void BeginHoldBreathTimer();

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void HoldBreathTimer();

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	FGameplayTag CalculateBreathReleaseMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void EndHoldBreathTimer();

	// Breath Recovery
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	float CalculateBreathRecoveryTime();

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void BeginBreathRecoveryTimer();

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void BreathRecoveryTimer();

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void EndBreathRecoveryTimer();

	// Desired HoldingBreath

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	bool CanHoldBreath() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	const FGameplayTag& GetDesiredHoldingBreath() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewHoldingBreathTag"))
	void SetDesiredHoldingBreath(const FGameplayTag& NewHoldingBreathTag);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredHoldingBreath(const FGameplayTag& NewHoldingBreathTag);

	// HoldingBreath

public:
	const FGameplayTag& GetHoldingBreath() const;

private:
	void SetHoldingBreath(const FGameplayTag& NewHoldingBreathTag);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnHoldingBreathChanged(const FGameplayTag& PreviousHoldingBreathTag);

	// void UpdateADSTransforms() const;

private:
	// ReloadingType

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated, Meta = (AllowPrivateAccess))
	FGameplayTag DesiredReloadingType{ FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (AllowPrivateAccess))
	FGameplayTag ReloadingType{ FGameplayTag::EmptyTag };

	// FirearmFingerAction

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated, Meta = (AllowPrivateAccess))
	FGameplayTag DesiredFirearmFingerAction{ AlsxtFirearmFingerActionTags::None };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (AllowPrivateAccess))
	FGameplayTag FirearmFingerAction{ AlsxtFirearmFingerActionTags::None };

	// FirearmFingerActionHand

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated, Meta = (AllowPrivateAccess))
	FGameplayTag DesiredFirearmFingerActionHand{ FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (AllowPrivateAccess))
	FGameplayTag FirearmFingerActionHand{ FGameplayTag::EmptyTag };

	// HoldingBreath

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated, Meta = (AllowPrivateAccess))
	FGameplayTag DesiredHoldingBreath{ FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (AllowPrivateAccess))
	FGameplayTag HoldingBreath{ FGameplayTag::EmptyTag };

	// Desired ReloadingType

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	const FGameplayTag& GetDesiredReloadingType() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewReloadingTypeTag"))
	void SetDesiredReloadingType(const FGameplayTag& NewReloadingTypeTag);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredReloadingType(const FGameplayTag& NewReloadingTypeTag);

	// ReloadingType

public:
	const FGameplayTag& GetReloadingType() const;

private:
	void SetReloadingType(const FGameplayTag& NewReloadingTypeTag);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnReloadingTypeChanged(const FGameplayTag& PreviousReloadingTypeTag);

	// Desired FirearmFingerAction

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	const FGameplayTag& GetDesiredFirearmFingerAction() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewFirearmFingerActionTag"))
	void SetDesiredFirearmFingerAction(const FGameplayTag& NewFirearmFingerActionTag);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredFirearmFingerAction(const FGameplayTag& NewFirearmFingerActionTag);

	// FirearmFingerAction

public:
	const FGameplayTag& GetFirearmFingerAction() const;

private:
	void SetFirearmFingerAction(const FGameplayTag& NewFirearmFingerActionTag);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnFirearmFingerActionChanged(const FGameplayTag& PreviousFirearmFingerActionTag);

	// Desired FirearmFingerActionHand

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	const FGameplayTag& GetDesiredFirearmFingerActionHand() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewFirearmFingerActionHandTag"))
	void SetDesiredFirearmFingerActionHand(const FGameplayTag& NewFirearmFingerActionHandTag);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredFirearmFingerActionHand(const FGameplayTag& NewFirearmFingerActionHandTag);

	// FirearmFingerActionHand

public:
	const FGameplayTag& GetFirearmFingerActionHand() const;

private:
	void SetFirearmFingerActionHand(const FGameplayTag& NewFirearmFingerActionHandTag);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnFirearmFingerActionHandChanged(const FGameplayTag& PreviousFirearmFingerActionHandTag);

public:
	virtual void OnAttackCollision_Implementation(FAttackDoubleHitResult Hit) override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	void GetFirearmCameraShakeInfo(TSubclassOf<UCameraShakeBase>& CameraShakeClass, float& Scale);

	// Character Sound Component Interface Functions
	virtual void PlayAttackSound_Implementation(bool MovementSound, bool AccentSound, bool WeaponSound, UPARAM(meta = (Categories = "Als.Sex"))const FGameplayTag& SoundSex, UPARAM(meta = (Categories = "Als.Voice Variant"))const FGameplayTag& Variant, UPARAM(meta = (Categories = "Als.OverlayMode"))const FGameplayTag& Overlay, UPARAM(meta = (Categories = "Als.Action Strength"))const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float Stamina) override;

// INTERFACE FUNCTIONS

public:
	// Character Interface
	virtual FGameplayTag GetCharacterHoldingBreath_Implementation() const override;

	// Held Item Interface Functions
	virtual FGameplayTag GetCharacterReloadingType_Implementation() const override;
	virtual FGameplayTag GetCharacterFirearmFingerAction_Implementation() const override;
	virtual FGameplayTag GetCharacterFirearmFingerActionHand_Implementation() const override;
	
	//Character Camera Effects Component Interface
	virtual UAlsCameraComponent* GetCameraComponent_Implementation() const override;
	virtual UAlsxtCharacterCameraEffectsComponent* GetCameraEffectsComponent_Implementation() const override;
	virtual FAlsxtGeneralCameraEffectsSettings GetCameraEffectsSettings_Implementation() const override;
	virtual void SetRadialBlurEffect_Implementation(float Amount) override;
	virtual void ResetRadialBlurEffect_Implementation() override;
	virtual void SetFocusEffect_Implementation(bool NewFocus) override;
	virtual void AddDrunkEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength) override;
	virtual void ResetDrunkEffect_Implementation() override;
	virtual void AddHighEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength) override;
	virtual void ResetHighEffect_Implementation() override;
	virtual void AddSuppressionEffect_Implementation(float Magnitude, float PostDelay) override;
	virtual void ResetSuppressionEffect_Implementation() override;
	virtual void AddBlindnessEffect_Implementation(float Magnitude, float Length, float FadeOutLength) override;
	virtual void ResetBlindnessEffect_Implementation() override;
	virtual void AddProjectileFlyByEffect_Implementation(USoundBase* Sound, FVector Location, FRotator Rotation) override;
	virtual void AddConcussionEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength) override;
	virtual void ResetConcussionEffect_Implementation() override;
	virtual void AddDamageEffect_Implementation(float Damage, const FGameplayTag& DamageType, const FHitResult& HitResult, float PostDelay) override;
	virtual void ResetDamageEffect_Implementation() override;
	virtual void AddNearDeathEffect_Implementation(float Damage, const FGameplayTag& DamageType) override;
	virtual void ResetNearDeathEffect_Implementation() override;
	virtual void AddDeathEffect_Implementation(float Damage, const FGameplayTag& DamageType, const FHitResult& HitResult, float PostDelay) override;
	virtual void ResetDeathEffect_Implementation() override;

	//Head Look At Interface
	virtual void IsInFrontOf_Implementation(bool& IsInFrontOf, FVector LookAtActorLocation) const override;

	// Combat Interface
	virtual FAlsxtGeneralCombatSettings GetGeneralCombatSettings_Implementation() override;
	virtual FAlsxtCombatAttackTraceSettings GetCombatAttackTraceSettings_Implementation() override;
	virtual void BeginCombatAttackCollisionTrace_Implementation(FAlsxtCombatAttackTraceSettings TraceSettings) override;
	virtual void EndCombatAttackCollisionTrace_Implementation() override;

	//Firearm Interface
	virtual FAlsxtFirearmAimState GetFirearmAimState_Implementation() const override;
	virtual void InitializeFirearm_Implementation() override;
	virtual void OnFirearmDischarge_Implementation() override;
	virtual void OnFirearmDischargeEnd_Implementation() override;
};

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetDesiredReloadingType() const
{
	return DesiredReloadingType;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetReloadingType() const
{
	return ReloadingType;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetDesiredFirearmFingerAction() const
{
	return DesiredFirearmFingerAction;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetFirearmFingerAction() const
{
	return FirearmFingerAction;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetDesiredFirearmFingerActionHand() const
{
	return DesiredFirearmFingerActionHand;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetFirearmFingerActionHand() const
{
	return FirearmFingerActionHand;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetDesiredHoldingBreath() const
{
	return DesiredHoldingBreath;
}

inline const FGameplayTag& AAlsxtCharacterAdvanced::GetHoldingBreath() const
{
	return HoldingBreath;
}
