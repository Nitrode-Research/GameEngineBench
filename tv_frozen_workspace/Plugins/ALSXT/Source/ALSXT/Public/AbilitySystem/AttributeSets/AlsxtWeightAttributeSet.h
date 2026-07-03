// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AlsxtWeightAttributeSet.generated.h"


// Contains Attributes related to Weight. Any Character that uses Weight will need this Attribute Set.
//
//	Contents:
//	Weight - Primarily used as reduction / increase to damage or status
UCLASS()
class ALSXT_API UAlsxtWeightAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()

public:
	
	UAlsxtWeightAttributeSet();

	static float Weight_BASE;
	static float Weight_MAX;
	static float Weight_MIN;

	// Attribute Set Overrides.
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Holds the current value for Weight.
	UPROPERTY(BlueprintReadOnly, Category = "Weight Attribute Set", ReplicatedUsing = OnRep_Weight)
	FGameplayAttributeData Weight;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtWeightAttributeSet, Weight)

	UFUNCTION(BlueprintPure, Category = "Weight Attribute Set")
	static float GetBaseWeight() { return Weight_BASE; }

	UFUNCTION(BlueprintPure, Category = "Weight Attribute Set")
	static FVector2D GetMinMaxWeight() { return FVector2D(Weight_MIN, Weight_MAX); }

protected:
	
	UFUNCTION()
	virtual void OnRep_Weight(const FGameplayAttributeData& OldValue);
};