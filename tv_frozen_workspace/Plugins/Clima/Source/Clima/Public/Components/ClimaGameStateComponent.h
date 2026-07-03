// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ClimaMicroClimateStructs.h"
#include "Data/ClimaEnums.h"
#include "ClimaGameStateComponent.generated.h"

USTRUCT(BlueprintType)
struct FAtmosphereDataPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Altitude {0.0f};
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AirDensity {0.0f}; // kg/m^3
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedOfSound {0.0f}; // m/s
};

/**
* @file ClimaGameStateComponent.h
* @brief Component for Game State to Manage In-Game Weather
*/
UCLASS(BlueprintType, Blueprintable)
class CLIMA_API UClimaGameStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClimaGameStateComponent();
	
	void DecompressStructFromArchive(const TArray<uint8>& CompressedData);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	FVector GlobalWind;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Select atmosphere model")) 
	EClimaAtmosphereType AtmosphereType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World", meta = (ToolTip = "Air Density at sea level - in KG/m^3")) 
	float SeaLevelAirDensity = 1.21;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World", meta = (ToolTip = "in cm/s")) 
	float SeaLevelSpeedOfSound = 34300;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World", meta = (ToolTip = "Used for Density Curve atmosphere model")) 
	UCurveFloat* AirDensityCurve;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	bool SpeedOfSoundVariesWithAltitude = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	UCurveFloat* SpeedOfSoundCurve;

	// UPROPERTY to allow editing in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	TArray<FAtmosphereDataPoint> AirDensityCurvePoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	TArray<FAtmosphereDataPoint> SpeedOfSoundCurvePoint;

	// A curve table or data asset is best for data-driven design
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	class UCurveFloat* MachDragCurve;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Atmosphere pressure at 0,0,0 - in millibars")) 
	float SeaLevelAirPressure = 1012.5f; // Pa

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	float SeaLevelAirPressureNew = 101325.0f; // Pa

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Specific Gas Constant, dry air = 287.058")) 
	float SpecificGasConstant = 287.058; // J/(kg·K)
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	float SpecificGasConstantNew = 287.058f; // J/(kg·K)

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	float WorldScale = 1.0;
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Atmosphere Temperature at 0,0,0 - in degrees C")) 
	float SeaLevelAirTemperature = 20.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Temperature Decrease With Altitude, degrees per meter")) 
	float TemperatureLapseRate = 0.00649f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Altitude at which temperature stops decreasing, in meters"))
	float TropopauseAltitude = 11000.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "World Origin Location")) 
	FVector WorldCenterLocation = FVector(0, 0, 0);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Use spherical planet model to get altitude")) 
	bool SphericalAltitude = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, SaveGame, Category = "World", meta = (ToolTip = "Planet radius"), Meta = (EditCondition = "SphericalAltitude")) 
	float SeaLevelRadius = 637100000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	bool OverrideGravity = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World") 
	FVector Gravity = FVector(0,0,-980);

	// Replicated array to store the compressed microclimate data
	UPROPERTY(ReplicatedUsing=OnRep_MicroclimateData)
	TArray<FCompressedMicroclimateData> MicroclimateData;
	
	// Function to get interpolated air density at a given altitude
	float GetAirDensityAtAltitude(float Altitude) const;
    
	// Function to get interpolated speed of sound at a given altitude
	float GetSpeedOfSoundAtAltitude(float Altitude) const;

	bool IsLocationInMicroclimate(FVector InVector, const FCompressedMicroclimateData& Microclimate) const;
	
	// Function to get a local wind vector
	FVector GetWindAtLocation(const FVector& Location) const;

protected:
	UFUNCTION()
	void OnRep_MicroclimateData();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};