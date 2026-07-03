// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtNutritionAttributeSet.generated.h"

// Contains Attributes related to Nutrition. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current Nutrition - How much Nutrition the Character has at this moment.
// Maximum Nutrition - The maximum amount of Nutrition the Character can have.
// Nutrition Regeneration - How much Nutrition is gained per period of Regeneration.
//
// Damage - A meta Attribute that negatively changes the value of Nutrition.
// Healing - A meta Attribute that positively changes the value of Nutrition.

UCLASS()
class ALSXT_API UAlsxtNutritionAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtNutritionAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Used to create a local copy of Damage which is then subtracted from Current Nutrition.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, Damage)

	// Used to create a local copy of Healing which is then added to Current Nutrition.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Healing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, Healing)

	// Used to create a local copy of Bleed which is then added to Bleeding.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedBleed;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, ReceivedBleed)

	// Used to create a local copy of Direct Damage which is then added to Current Nutrition.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedDirectDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, ReceivedDirectDamage)
	
	// Holds the current value for Nutrition.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", ReplicatedUsing = OnRep_CurrentNutrition)
	FGameplayAttributeData CurrentNutrition;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, CurrentNutrition)

	// Holds the value for Maximum Nutrition.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", ReplicatedUsing = OnRep_MaximumNutrition)
	FGameplayAttributeData MaximumNutrition;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, MaximumNutrition)

	// Holds the value for Nutrition Regeneration.
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", ReplicatedUsing = OnRep_NutritionRegeneration)
	FGameplayAttributeData NutritionRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, NutritionRegeneration)

	// Holds the current bleeding amount
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", ReplicatedUsing = OnRep_Bleeding)
	FGameplayAttributeData Bleeding;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, Bleeding)

	// Holds the current bleed healing rate
	UPROPERTY(BlueprintReadOnly, Category = "Nutrition Attribute Set", ReplicatedUsing = OnRep_BleedHealing)
	FGameplayAttributeData BleedHealing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtNutritionAttributeSet, BleedHealing)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;
	
	UFUNCTION()
	virtual void OnRep_CurrentNutrition(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaximumNutrition(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_NutritionRegeneration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Bleeding(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BleedHealing(const FGameplayAttributeData& OldValue);
	
	
};
