#pragma once

#include "NativeGameplayTags.h"
#include "Settings/AlsxtCharacterSoundSettings.h"
#include "Utility/AlsxtStructs.h"
#include "AlsxtCharacterCustomizationComponentInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtCharacterCustomizationComponentInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtCharacterCustomizationComponentInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	FALSXTCharacterVoiceParameters GetVoiceParameters();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	void SetVoiceParameters(FALSXTCharacterVoiceParameters NewVoiceParameters);
  
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	void SetCustomizationMorphTarget(FALSXTCharacterMorphParameter MorphTargetName);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	float GetCustomizationMorphTarget(FName MorphTargetName);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	FALSXTCharacterMorphParameters GetAllCustomMorphTargets();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	TArray<UAlsxtPaintableSkeletalMeshComponent*> GetHighlightableComponents();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	void SetEquipmentMorphTarget(FALSXTCharacterMorphParameter MorphTarget);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character Customization Component Interface")
	void SetEquipmentCustomMorphTargets(FALSXTCharacterMorphParameters MorphTargetName);
};