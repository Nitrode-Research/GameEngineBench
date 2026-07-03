// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "RicochetAttributeSetBase.h"
#include "RicochetStatusAttributeSet.generated.h"

/**
* @file RicochetStatusAttributeSet.h
* @brief Attribute Set for Rounds
* @attribute Health - 
* @attribute Dirtiness - 
* @attribute Wetness - 
*/
UCLASS()
class RICOCHET_API URicochetStatusAttributeSet : public URicochetAttributeSetBase
{
	GENERATED_BODY()

public:
	URicochetStatusAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(URicochetStatusAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Dirtiness)
	FGameplayAttributeData Dirtiness;
	ATTRIBUTE_ACCESSORS(URicochetStatusAttributeSet, Dirtiness);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Wetness)
	FGameplayAttributeData Wetness;
	ATTRIBUTE_ACCESSORS(URicochetStatusAttributeSet, Wetness);

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Dirtiness(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Wetness(const FGameplayAttributeData& OldValue);
	
};
