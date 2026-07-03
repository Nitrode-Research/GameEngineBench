// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "RicochetAttributeSetBase.h"
#include "RicochetReceiverAttributeSet.generated.h"

/**
* @file RicochetReceiverAttributeSet.h
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
UCLASS()
class RICOCHET_API URicochetReceiverAttributeSet : public URicochetAttributeSetBase
{
	GENERATED_BODY()

public:
	URicochetReceiverAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMin)
	FGameplayAttributeData MuzzleVelocityMin;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, MuzzleVelocityMin);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMax)
	FGameplayAttributeData MuzzleVelocityMax;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, MuzzleVelocityMax);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_InheritVelocity)
	FGameplayAttributeData InheritVelocity;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, InheritVelocity);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMultiplierMin)
	FGameplayAttributeData MuzzleVelocityMultiplierMin;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMin);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMultiplierMax)
	FGameplayAttributeData MuzzleVelocityMultiplierMax;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, MuzzleVelocityMultiplierMax);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_AdditionalVelocityX)
	FGameplayAttributeData AdditionalVelocityX;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, AdditionalVelocityX);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_AdditionalVelocityY)
	FGameplayAttributeData AdditionalVelocityY;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, AdditionalVelocityY);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_AdditionalVelocityZ)
	FGameplayAttributeData AdditionalVelocityZ;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, AdditionalVelocityZ);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_FireRateMin)
	FGameplayAttributeData FireRateMin;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, FireRateMin);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_FireRateMax)
	FGameplayAttributeData FireRateMax;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, FireRateMax);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_FailureToExtractProbability)
	FGameplayAttributeData FailureToExtractProbability;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, FailureToExtractProbability);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_FailureToEjectProbability)
	FGameplayAttributeData FailureToEjectProbability;
	ATTRIBUTE_ACCESSORS(URicochetReceiverAttributeSet, FailureToEjectProbability);
	
	// Convert to Gameplay Tag that will be added to ASC
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", Meta = (AllowPrivateAccess)) 
	// bool Shotgun=false;

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMin(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_InheritVelocity(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMultiplierMin(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMultiplierMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AdditionalVelocityX(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AdditionalVelocityY(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AdditionalVelocityZ(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_FireRateMin(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_FireRateMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_FailureToExtractProbability(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_FailureToEjectProbability(const FGameplayAttributeData& OldValue);	
};
