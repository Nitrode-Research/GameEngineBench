// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtBodyTempAttributeSet.generated.h"

// Contains Attributes related to BodyTemp. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current BodyTemp - How much BodyTemp the Character has at this moment.
// Maximum BodyTemp - The maximum amount of BodyTemp the Character can have.
// BodyTemp Regeneration - How much BodyTemp is gained per period of Regeneration.
//
// Damage - A meta Attribute that negatively changes the value of BodyTemp.
// Healing - A meta Attribute that positively changes the value of BodyTemp.

UCLASS()
class ALSXT_API UAlsxtBodyTempAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtBodyTempAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Used to create a local copy of Damage which is then subtracted from Current BodyTemp.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, Damage)

	// Used to create a local copy of Healing which is then added to Current BodyTemp.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Healing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, Healing)

	// Used to create a local copy of Bleed which is then added to Bleeding.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedBleed;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, ReceivedBleed)

	// Used to create a local copy of Direct Damage which is then added to Current BodyTemp.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedDirectDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, ReceivedDirectDamage)
	
	// Holds the current value for BodyTemp.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", ReplicatedUsing = OnRep_CurrentBodyTemp)
	FGameplayAttributeData CurrentBodyTemp;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, CurrentBodyTemp)

	// Holds the value for Maximum BodyTemp.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", ReplicatedUsing = OnRep_MaximumBodyTemp)
	FGameplayAttributeData MaximumBodyTemp;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, MaximumBodyTemp)

	// Holds the value for BodyTemp Regeneration.
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", ReplicatedUsing = OnRep_BodyTempRegeneration)
	FGameplayAttributeData BodyTempRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, BodyTempRegeneration)

	// Holds the current bleeding amount
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", ReplicatedUsing = OnRep_Bleeding)
	FGameplayAttributeData Bleeding;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, Bleeding)

	// Holds the current bleed healing rate
	UPROPERTY(BlueprintReadOnly, Category = "BodyTemp Attribute Set", ReplicatedUsing = OnRep_BleedHealing)
	FGameplayAttributeData BleedHealing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtBodyTempAttributeSet, BleedHealing)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;
	
	UFUNCTION()
	virtual void OnRep_CurrentBodyTemp(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaximumBodyTemp(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BodyTempRegeneration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Bleeding(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BleedHealing(const FGameplayAttributeData& OldValue);
	
	
};
