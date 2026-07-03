// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtHealthAttributeSet.generated.h"

// Contains Attributes related to Health. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current Health - How much Health the Character has at this moment.
// Maximum Health - The maximum amount of Health the Character can have.
// Health Regeneration - How much Health is gained per period of Regeneration.
//
// Damage - A meta Attribute that negatively changes the value of Health.
// Healing - A meta Attribute that positively changes the value of Health.

UCLASS()
class ALSXT_API UAlsxtHealthAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtHealthAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Used to create a local copy of Damage which is then subtracted from Current Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, Damage)

	// Used to create a local copy of Healing which is then added to Current Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Healing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, Healing)

	// Used to create a local copy of Bleed which is then added to Bleeding.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedBleed;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, ReceivedBleed)

	// Used to create a local copy of Direct Damage which is then added to Current Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedDirectDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, ReceivedDirectDamage)
	
	// Holds the current value for Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", ReplicatedUsing = OnRep_CurrentHealth)
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, CurrentHealth)

	// Holds the value for Maximum Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", ReplicatedUsing = OnRep_MaximumHealth)
	FGameplayAttributeData MaximumHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, MaximumHealth)

	// Holds the value for Health Regeneration.
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", ReplicatedUsing = OnRep_HealthRegeneration)
	FGameplayAttributeData HealthRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, HealthRegeneration)

	// Holds the current bleeding amount
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", ReplicatedUsing = OnRep_Bleeding)
	FGameplayAttributeData Bleeding;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, Bleeding)

	// Holds the current bleed healing rate
	UPROPERTY(BlueprintReadOnly, Category = "Health Attribute Set", ReplicatedUsing = OnRep_BleedHealing)
	FGameplayAttributeData BleedHealing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHealthAttributeSet, BleedHealing)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;
	
	UFUNCTION()
	virtual void OnRep_CurrentHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaximumHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_HealthRegeneration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Bleeding(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BleedHealing(const FGameplayAttributeData& OldValue);
	
	
};
