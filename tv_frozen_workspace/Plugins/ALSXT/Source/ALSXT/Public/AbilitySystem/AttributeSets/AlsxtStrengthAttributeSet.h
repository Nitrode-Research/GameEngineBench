// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtStrengthAttributeSet.generated.h"

// Contains Attributes related to Strength. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current Strength - How much Strength the Character has at this moment.
// Maximum Strength - The maximum amount of Strength the Character can have.
// Strength Regeneration - How much Strength is gained per period of Regeneration.
//
// Damage - A meta Attribute that negatively changes the value of Strength.
// Healing - A meta Attribute that positively changes the value of Strength.

UCLASS()
class ALSXT_API UAlsxtStrengthAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtStrengthAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Used to create a local copy of Damage which is then subtracted from Current Strength.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, Damage)

	// Used to create a local copy of Healing which is then added to Current Strength.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Healing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, Healing)

	// Used to create a local copy of Bleed which is then added to Bleeding.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedBleed;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, ReceivedBleed)

	// Used to create a local copy of Direct Damage which is then added to Current Strength.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedDirectDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, ReceivedDirectDamage)
	
	// Holds the current value for Strength.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", ReplicatedUsing = OnRep_CurrentStrength)
	FGameplayAttributeData CurrentStrength;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, CurrentStrength)

	// Holds the value for Maximum Strength.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", ReplicatedUsing = OnRep_MaximumStrength)
	FGameplayAttributeData MaximumStrength;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, MaximumStrength)

	// Holds the value for Strength Regeneration.
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", ReplicatedUsing = OnRep_StrengthRegeneration)
	FGameplayAttributeData StrengthRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, StrengthRegeneration)

	// Holds the current bleeding amount
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", ReplicatedUsing = OnRep_Bleeding)
	FGameplayAttributeData Bleeding;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, Bleeding)

	// Holds the current bleed healing rate
	UPROPERTY(BlueprintReadOnly, Category = "Strength Attribute Set", ReplicatedUsing = OnRep_BleedHealing)
	FGameplayAttributeData BleedHealing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtStrengthAttributeSet, BleedHealing)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;
	
	UFUNCTION()
	virtual void OnRep_CurrentStrength(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaximumStrength(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_StrengthRegeneration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Bleeding(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BleedHealing(const FGameplayAttributeData& OldValue);
	
	
};
