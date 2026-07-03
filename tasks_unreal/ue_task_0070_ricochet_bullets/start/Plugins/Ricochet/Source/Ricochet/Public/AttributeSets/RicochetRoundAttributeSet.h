// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "RicochetAttributeSetBase.h"
#include "RicochetRoundAttributeSet.generated.h"

/**
* @file RicochetRoundAttributeSet.h
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
UCLASS()
class RICOCHET_API URicochetRoundAttributeSet : public URicochetAttributeSetBase
{
	GENERATED_BODY()

public:
	URicochetRoundAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Mass)
	FGameplayAttributeData Mass;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, Mass);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Diameter)
	FGameplayAttributeData Diameter;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, Diameter);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_FormFactor)
	FGameplayAttributeData FormFactor;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, FormFactor);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_EffectiveRange)
	FGameplayAttributeData EffectiveRange;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, EffectiveRange);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_BaseEffectiveRangeMultiplier)
	FGameplayAttributeData BaseEffectiveRangeMultiplier;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, BaseEffectiveRangeMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MaxEffectiveRangeMultiplier)
	FGameplayAttributeData MaxEffectiveRangeMultiplier;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MaxEffectiveRangeMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_BaseImpactEnergyMultiplier)
	FGameplayAttributeData BaseImpactEnergyMultiplier;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, BaseImpactEnergyMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MaxImpactEnergyMultiplier)
	FGameplayAttributeData MaxImpactEnergyMultiplier;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MaxImpactEnergyMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_InitialVelocityX)
	FGameplayAttributeData InitialVelocityX;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, InitialVelocityX);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_InitialVelocityY)
	FGameplayAttributeData InitialVelocityY;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, InitialVelocityY);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_InitialVelocityZ)
	FGameplayAttributeData InitialVelocityZ;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, InitialVelocityZ);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMin)
	FGameplayAttributeData MuzzleVelocityMin;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MuzzleVelocityMin);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MuzzleVelocityMax)
	FGameplayAttributeData MuzzleVelocityMax;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MuzzleVelocityMax);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_Spread)
	FGameplayAttributeData Spread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, Spread);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_GrazingAngleExponent)
	FGameplayAttributeData GrazingAngleExponent;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, GrazingAngleExponent);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MinPenetration)
	FGameplayAttributeData MinPenetration;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MinPenetration);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_MaxPenetration)
	FGameplayAttributeData MaxPenetration;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, MaxPenetration);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_PenetrationNormalization)
	FGameplayAttributeData PenetrationNormalization;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, PenetrationNormalization);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_PenetrationNormalizationGrazing)
	FGameplayAttributeData PenetrationNormalizationGrazing;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, PenetrationNormalizationGrazing);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_PenetrationEntryAngleSpread)
	FGameplayAttributeData PenetrationEntryAngleSpread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, PenetrationEntryAngleSpread);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_RicochetProbability)
	FGameplayAttributeData RicochetProbability;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, RicochetProbability);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_RicochetProbabilityGrazing)
	FGameplayAttributeData RicochetProbabilityGrazing;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, RicochetProbabilityGrazing);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_PenetrationExitAngleSpread)
	FGameplayAttributeData PenetrationExitAngleSpread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, PenetrationExitAngleSpread);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_RicochetRestitution)
	FGameplayAttributeData RicochetRestitution;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, RicochetRestitution);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_RicochetFriction)
	FGameplayAttributeData RicochetFriction;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, RicochetFriction);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_RicochetSpread)
	FGameplayAttributeData RicochetSpread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, RicochetSpread);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_ShotgunSpread)
	FGameplayAttributeData ShotgunSpread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, ShotgunSpread);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_ShotgunVelocitySpread)
	FGameplayAttributeData ShotgunVelocitySpread;
	ATTRIBUTE_ACCESSORS(URicochetRoundAttributeSet, ShotgunVelocitySpread);

	// Convert to Gameplay Tag that will be added to ASC
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", Meta = (AllowPrivateAccess)) 
	// bool Shotgun=false;

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_Mass(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Diameter(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_FormFactor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_EffectiveRange(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BaseEffectiveRangeMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxEffectiveRangeMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BaseImpactEnergyMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxImpactEnergyMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_InitialVelocityX(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_InitialVelocityY(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_InitialVelocityZ(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMin(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MuzzleVelocityMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Spread(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_GrazingAngleExponent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MinPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_PenetrationNormalization(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_PenetrationNormalizationGrazing(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_PenetrationEntryAngleSpread(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_RicochetProbability(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_RicochetProbabilityGrazing(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_PenetrationExitAngleSpread(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_RicochetRestitution(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_RicochetFriction(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_RicochetSpread(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_ShotgunSpread(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_ShotgunVelocitySpread(const FGameplayAttributeData& OldValue);
	
};
