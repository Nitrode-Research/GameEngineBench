#pragma once

#include "Settings/AlsxtHeldItemSettings.h"
#include "AlsxtHeldItemInterface.generated.h"



UINTERFACE(Blueprintable)
class UAlsxtHeldItemInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtHeldItemInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Held Item Interface")
  bool IsHoldingItem() const;
  
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Held Item Interface")
  bool IsHoldingAimableItem() const;
	
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  FAlsxtHeldItemSettings GetHeldItemSettings() const;

  // Get which current Modes, Positions and Stances for the Held Item
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALSXTCharacter Interface|Held Item")
  FAlsxtHeldItemState GetCharacterHeldItemState() const;

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALSXTCharacter Interface|Held Item")
  FAlsxtHeldItemViewTarget GetCharacterHeldItemViewTarget() const;

  // Get the current Transform, Socket Names and Positions for the Held Item
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALSXTCharacter Interface|Held Item")
  FAlsxtHeldItemGripStates GetHeldItemGripStates() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  FAlsxtHeldItemGrip GetHeldItemGrip() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  FAlsxtHeldItemGrip GetHeldItemForegrip() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  TArray<FAlsxtHeldItemEquipMontage> GetHeldItemEquipMontages(const FGameplayTag& Type) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  TArray<FAlsxtHeldItemActionMontage> GetHeldItemActionMontages(const FGameplayTag& Type) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Held Item Interface")
  TArray<FAlsxtHeldItemAttackMontage> GetHeldItemAttackMontages(const FGameplayTag& Type, const FGameplayTag& Strength) const;
};