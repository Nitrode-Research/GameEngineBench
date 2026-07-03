#pragma once

#include "Settings/AlsxtFirearmSettings.h"
#include "AlsxtFirearmInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtFirearmInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtFirearmInterface {
	GENERATED_BODY()

public:
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|Settings")
  FAlsxtFirearmDischargeEffectsSettings GetFirearmEffectsSettings() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|Settings")
  FAlsxtFirearmRecoilSettings GetFirearmRecoilSettings() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  TSoftObjectPtr<UAlsxtFirearmRoundEffectsAsset> GetFirearmRoundEffectsAsset() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  void SetFirearmRoundEffectsAsset(UAlsxtFirearmRoundEffectsAsset* NewFirearmRoundEffectsAsset);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmDischargeEffectsModifiersState GetFirearmDischargeEffectsModifiersState() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmDischargeEffectsModifiersState GetCharacterFirearmDischargeEffectsModifiersState() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmDischargeEffectsModifiersState GetCalculatedDischargeEffectsModifiersState() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmSightsState GetFirearmState() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FGameplayTag GetFirearmFireMode() const;
	
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmSightsState GetFirearmSightsState() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmAimState GetFirearmAimState() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtRecoilState GetRecoilState() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void InitializeFirearm();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischarge();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeAnimation();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeControlRotation();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeCameraShake();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeMuzzleFlash();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeSound();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeEjectionPort();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|State", Meta = (ForceAsFunction))
  void OnFirearmDischargeEnd();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  FAlsxtFirearmDischargeEffectsState GetFirearmDischargeEffectsState() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface|State")
  void SetFirearmDischargeEffectsState(FAlsxtFirearmDischargeEffectsState NewFirearmDischargeEffectsState);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Firearm Interface")
  TArray <FAlsxtFirearmActionSound> GetFirearmActionSounds(const FGameplayTag& ActionType) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|Actions")
  void ChargeFirearm();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|Actions")
  void Reload();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|Actions")
  void ReloadWithRetention();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Firearm Interface|Actions")
  void ClearWeaponMalfunction();
};