// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Settings/AlsxtCharacterSoundSettings.h"
#include "Components/Mesh/AlsxtPaintableSkeletalMeshComponent.h"
#include "AlsxtCharacterCustomizationComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSXT_API UAlsxtCharacterCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAlsxtCharacterCustomizationComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// const bool DoesComponentHaveMorphTarget(UALSXTPaintableSkeletalMeshComponent Component, FName MorphTargetName);
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// const bool IsMorphTargetAdded(UALSXTPaintableSkeletalMeshComponent Component, FName MorphTargetName);
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// void SetCustomizationMorphTarget(FALSXTCharacterMorphParameter MorphTargetName);
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// void SetCustomizationMorphTarget(FALSXTCharacterMorphParameter MorphTargetName);
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// float GetCustomizationMorphTarget(FName MorphTargetName);
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// FALSXTCharacterMorphParameters GetAllCustomMorphTargets();
	// 
	// UFUNCTION(BlueprintCallable, Category = "Character Customization")
	// void ResetAllCustomMorphTargets();

	UFUNCTION(BlueprintCallable, Category = "Character Customization Interface|Parameters")
	FALSXTCharacterVoiceParameters GetVoiceParameters();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	FALSXTCharacterVoiceParameters VoiceParameters;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	FALSXTCharacterMorphParameters CharacterMorphParameters;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtPaintableSkeletalMeshComponent> Hair;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USkeletalMeshComponent> HairDummyShadow;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtPaintableSkeletalMeshComponent> FacialHair;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess))
	TObjectPtr<USkeletalMeshComponent> FacialHairDummyShadow;

	TArray<UAlsxtPaintableSkeletalMeshComponent*> GetAllComponents();
	
	TArray<UAlsxtPaintableSkeletalMeshComponent*> GetHighlightableComponents();
};
