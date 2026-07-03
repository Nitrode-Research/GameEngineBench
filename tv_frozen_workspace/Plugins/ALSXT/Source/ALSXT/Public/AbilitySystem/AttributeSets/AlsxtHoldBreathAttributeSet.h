// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AbilitySystem/AttributeSets/AlsxtAttributeSetBase.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "AlsxtHoldBreathAttributeSet.generated.h"

// Contains Attributes related to HoldBreath. Any Character that takes damage will need this Attribute Set.
//
// Contents:
//
// Current HoldBreath - How much HoldBreath the Character has at this moment.
// Max HoldBreath - The Max amount of HoldBreath the Character can have.
// HoldBreath Regen - How much HoldBreath is gained per period of Regen.
//
// Damage - A meta Attribute that negatively changes the value of HoldBreath.
// Healing - A meta Attribute that positively changes the value of HoldBreath.

UCLASS()
class ALSXT_API UAlsxtHoldBreathAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()
	
	public:
	UAlsxtHoldBreathAttributeSet();
	
	// Attribute Set Overrides.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Holds the current value for HoldBreath.
	UPROPERTY(BlueprintReadOnly, Category = "HoldBreath Attribute Set", ReplicatedUsing = OnRep_CurrentHoldBreathDuration)
	FGameplayAttributeData CurrentHoldBreathDuration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHoldBreathAttributeSet, CurrentHoldBreathDuration)

	// Holds the value for Max HoldBreath.
	UPROPERTY(BlueprintReadOnly, Category = "HoldBreath Attribute Set", ReplicatedUsing = OnRep_MaxHoldBreathDuration)
	FGameplayAttributeData MaxHoldBreathDuration;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHoldBreathAttributeSet, MaxHoldBreathDuration)

	// Holds the value for HoldBreath Regen.
	UPROPERTY(BlueprintReadOnly, Category = "HoldBreath Attribute Set", ReplicatedUsing = OnRep_CurrentHoldBreathDurationRegen)
	FGameplayAttributeData CurrentHoldBreathDurationRegen;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHoldBreathAttributeSet, CurrentHoldBreathDurationRegen)

	// Holds the value for Max HoldBreath.
	UPROPERTY(BlueprintReadOnly, Category = "HoldBreath Attribute Set", ReplicatedUsing = OnRep_MaxHoldBreathDurationRegen)
	FGameplayAttributeData MaxHoldBreathDurationRegen;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtHoldBreathAttributeSet, MaxHoldBreathDurationRegen)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeMaxValue> AttributeMaxValue;

	UFUNCTION()
	virtual void OnRep_CurrentHoldBreathDuration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHoldBreathDuration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CurrentHoldBreathDurationRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHoldBreathDurationRegen(const FGameplayAttributeData& OldValue);	
	
};
