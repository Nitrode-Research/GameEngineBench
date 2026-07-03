#pragma once

#include "NativeGameplayTags.h"
#include "Utility/ALSXTStructs.h"
#include "AlsxtCharacterEquipmentComponentInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtCharacterEquipmentComponentInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtCharacterEquipmentComponentInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Equipment Component Interface")
  void SetEquipmentMorphTarget(FALSXTCharacterMorphParameter MorphTarget);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Equipment Component Interface")
  void SetEquipmentCustomMorphTargets(FALSXTCharacterMorphParameters MorphTargetName);
};