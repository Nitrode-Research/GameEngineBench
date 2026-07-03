// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "RicochetAttributeSetBase.h"
#include "RicochetReceiverGatlingAttributeSet.generated.h"

/**
* @file RicochetReceiverGatlingAttributeSet.h
* @brief Attribute Set for Rounds
* @attribute Mass - 
* @attribute Diameter - 
* @attribute FormFactor - 
*/
UCLASS()
class RICOCHET_API URicochetReceiverGatlingAttributeSet : public URicochetAttributeSetBase
{
	GENERATED_BODY()

public:
	URicochetReceiverGatlingAttributeSet();

	// Attribute Set Overrides.
	// virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// virtual void ClampAttributes(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The attributes for the bullet round
	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_GatlingRPS)
	FGameplayAttributeData GatlingRps;
	ATTRIBUTE_ACCESSORS(URicochetReceiverGatlingAttributeSet, GatlingRps);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_GatlingSpoolUpTime)
	FGameplayAttributeData GatlingSpoolUpTime;
	ATTRIBUTE_ACCESSORS(URicochetReceiverGatlingAttributeSet, GatlingSpoolUpTime);

	UPROPERTY(BlueprintReadOnly, Category = "Bullet", ReplicatedUsing = OnRep_GatlingSpoolDownTime)
	FGameplayAttributeData GatlingSpoolDownTime;
	ATTRIBUTE_ACCESSORS(URicochetReceiverGatlingAttributeSet, GatlingSpoolDownTime);
	
	// Convert to Gameplay Tag that will be added to ASC
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", Meta = (AllowPrivateAccess)) 
	// bool Shotgun=false;

protected:
	// Replication functions
	UFUNCTION()
	virtual void OnRep_GatlingRps(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_GatlingSpoolUpTime(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_GatlingSpoolDownTime(const FGameplayAttributeData& OldValue);
	
};
