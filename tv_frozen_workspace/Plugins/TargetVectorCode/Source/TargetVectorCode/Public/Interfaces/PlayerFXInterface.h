#pragma once

#include "Sound/SoundBase.h"
#include "NativeGameplayTags.h"
#include "Engine/EngineTypes.h"
#include "PlayerFXInterface.generated.h"

UINTERFACE(Blueprintable)
class UPlayerFXInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IPlayerFXInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Sprinting")
	void AddSprintingEffect(float Magnitude, float PostDelay);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Sprinting")
	void ResetSprintingEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Focus")
	void SetFocusEffect(bool Focus);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Focus")
	void ResetFocusEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Suppression")
	void AddSuppressionEffect(float Magnitude, float PostDelay);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Suppression")
	void ResetSuppressionEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Blinded")
	void AddBlindedEffect(float Magnitude, float Length, float FadeOutLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Blinded")
	void ResetBlindedEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Flashbang")
	void AddFlashbangEffect(float Magnitude, float Length, float FadeOutLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Flashbang")
	void ResetFlashbangEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Fly By")
	void AddProjectileFlyByEffect(USoundBase* Sound);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Damage")
	void AddDamageEffect(float Damage, const FGameplayTag& DamageType, const FHitResult& HitResult, float PostDelay);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Damage")
	void ResetDamageEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Concussion")
	void AddConcussionEffect(float Magnitude, float Length, float FadeInLength, float FadeOutLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Concussion")
	void ResetConcussionEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Drunken")
	void AddDrunkEffect(float Magnitude, float Length, float FadeInLength, float FadeOutLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|Drunken")
	void ResetDrunkEffect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|High")
	void AddHighEffect(float Magnitude, float Length, float FadeInLength, float FadeOutLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player FX|High")
	void ResetHighEffect();
};