// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AlsxtAttributeSetBase.h"
#include "AlsxtResistanceAttributeSet.generated.h"


// Contains Attributes related to Resistance. Any Character that uses Resistance will need this Attribute Set.
//
//	Contents:
//	Resistance - Primarily used as reduction / increase to damage or status
UCLASS()
class ALSXT_API UAlsxtResistanceAttributeSet : public UAlsxtAttributeSetBase
{
	GENERATED_BODY()

public:
	
	UAlsxtResistanceAttributeSet();

	static float RESISTANCE_BASE;
	static float RESISTANCE_MAX;
	static float RESISTANCE_MIN;

	// Attribute Set Overrides.
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;

	virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Holds the current value for Resistance.
	UPROPERTY(BlueprintReadOnly, Category = "Resistance Attribute Set", ReplicatedUsing = OnRep_Resistance)
	FGameplayAttributeData Resistance;
	ATTRIBUTE_ACCESSORS_BASIC(UAlsxtResistanceAttributeSet, Resistance)

	UFUNCTION(BlueprintPure, Category = "Resistance Attribute Set")
	static float GetBaseResistance() { return RESISTANCE_BASE; }

	UFUNCTION(BlueprintPure, Category = "Resistance Attribute Set")
	static FVector2D GetMinMaxResistance() { return FVector2D(RESISTANCE_MIN, RESISTANCE_MAX); }

protected:
	
	UFUNCTION()
	virtual void OnRep_Resistance(const FGameplayAttributeData& OldValue);
};