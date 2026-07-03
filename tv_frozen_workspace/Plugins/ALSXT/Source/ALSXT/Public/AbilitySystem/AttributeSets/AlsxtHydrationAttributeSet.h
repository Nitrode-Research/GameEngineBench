// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtHydrationAttributeSet.generated.h"

// Contains Attributes related to Hydration. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current Hydration - How much Hydration the Character has at this moment.
// Maximum Hydration - The maximum amount of Hydration the Character can have.
// Hydration Regeneration - How much Hydration is gained per period of Regeneration.
//
// Damage - A meta Attribute that negatively changes the value of Hydration.
// Healing - A meta Attribute that positively changes the value of Hydration.

UCLASS()
class ALSXT_API UAlsxtHydrationAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtHydrationAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Used to create a local copy of Damage which is then subtracted from Current Hydration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, Damage)

	// Used to create a local copy of Healing which is then added to Current Hydration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData Healing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, Healing)

	// Used to create a local copy of Bleed which is then added to Bleeding.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedBleed;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, ReceivedBleed)

	// Used to create a local copy of Direct Damage which is then added to Current Hydration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", meta = (HideFromLevelInfos))
	FGameplayAttributeData ReceivedDirectDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, ReceivedDirectDamage)
	
	// Holds the current value for Hydration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", ReplicatedUsing = OnRep_CurrentHydration)
	FGameplayAttributeData CurrentHydration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, CurrentHydration)

	// Holds the value for Maximum Hydration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", ReplicatedUsing = OnRep_MaximumHydration)
	FGameplayAttributeData MaximumHydration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, MaximumHydration)

	// Holds the value for Hydration Regeneration.
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", ReplicatedUsing = OnRep_HydrationRegeneration)
	FGameplayAttributeData HydrationRegeneration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, HydrationRegeneration)

	// Holds the current bleeding amount
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", ReplicatedUsing = OnRep_Bleeding)
	FGameplayAttributeData Bleeding;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, Bleeding)

	// Holds the current bleed healing rate
	UPROPERTY(BlueprintReadOnly, Category = "Hydration Attribute Set", ReplicatedUsing = OnRep_BleedHealing)
	FGameplayAttributeData BleedHealing;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHydrationAttributeSet, BleedHealing)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;
	
	UFUNCTION()
	virtual void OnRep_CurrentHydration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaximumHydration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_HydrationRegeneration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Bleeding(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BleedHealing(const FGameplayAttributeData& OldValue);
	
	
};
