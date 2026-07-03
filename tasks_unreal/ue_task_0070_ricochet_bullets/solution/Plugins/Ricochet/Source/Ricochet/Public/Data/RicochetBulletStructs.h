// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "RicochetBulletStructs.generated.h"

USTRUCT(BlueprintType)
struct RICOCHET_API FRicochetRoundData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discharge", Meta = (AllowPrivateAccess))
	FVector InitialVelocity {FVector(0,0,0)};
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", Meta = (AllowPrivateAccess)) 
	bool Shotgun=false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", meta = (EditCondition = "Shotgun", AllowPrivateAccess)) 
	float ShotSpread=0.01;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Discharge|Shotgun", meta = (EditCondition = "Shotgun", AllowPrivateAccess)) 
	float ShotVelocitySpread = 0.01;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight", Meta = (AllowPrivateAccess)) 
	float MuzzleVelocityMin = 100000.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight", Meta = (AllowPrivateAccess)) 
	float MuzzleVelocityMax = 100000.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flight", Meta = (AllowPrivateAccess)) 
	float Spread = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact", Meta = (AllowPrivateAccess)) 
	float GrazingAngleExponent = 2.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float MinPenetration = 10.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float MaxPenetration = 20.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float PenetrationNormalization = 0.5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float PenetrationNormalizationGrazing = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float PenetrationEntryAngleSpread = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Penetration", Meta = (AllowPrivateAccess)) 
	float PenetrationExitAngleSpread = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Ricochet", Meta = (AllowPrivateAccess)) 
	float RicochetProbability = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Ricochet", Meta = (AllowPrivateAccess)) 
	float RicochetProbabilityGrazing = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Ricochet", Meta = (AllowPrivateAccess)) 
	float RicochetRestitution = 0.1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Ricochet", Meta = (AllowPrivateAccess)) 
	float RicochetFriction = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Impact|Ricochet", Meta = (AllowPrivateAccess)) 
	float RicochetSpread = 0.1;

	bool operator==(const FRicochetRoundData& other) const
	{
		return (other.InitialVelocity == InitialVelocity) && (other.Shotgun == Shotgun) && (other.ShotSpread == ShotSpread);
	}

};