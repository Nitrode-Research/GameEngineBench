// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ClimaAttributeSetBase.h"
#include "ClimaTemperatureAttributeSet.generated.h"

/**
* @file ClimaTemperatureAttributeSet.h
* @brief Attribute Set for Temperature
* @attribute Temperature - 
*/
UCLASS()
class CLIMA_API UClimaTemperatureAttributeSet : public UClimaAttributeSetBase
{
	GENERATED_BODY()

public:
	UClimaTemperatureAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Temperature)
	FGameplayAttributeData Temperature;
	ATTRIBUTE_ACCESSORS(UClimaTemperatureAttributeSet, Temperature);

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_Temperature(const FGameplayAttributeData& OldValue);	
};
