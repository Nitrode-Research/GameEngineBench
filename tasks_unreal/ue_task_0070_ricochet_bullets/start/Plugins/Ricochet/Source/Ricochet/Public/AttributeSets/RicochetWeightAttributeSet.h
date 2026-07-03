// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "RicochetAttributeSetBase.h"
#include "RicochetWeightAttributeSet.generated.h"

/**
* @file RicochetWeightAttributeSet.h
* @brief Attribute Set for Weight
* @attribute Weight - 
*/
UCLASS()
class RICOCHET_API URicochetWeightAttributeSet : public URicochetAttributeSetBase
{
	GENERATED_BODY()

public:
	URicochetWeightAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Weight)
	FGameplayAttributeData Weight;
	ATTRIBUTE_ACCESSORS(URicochetWeightAttributeSet, Weight);

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_Weight(const FGameplayAttributeData& OldValue);	
};
