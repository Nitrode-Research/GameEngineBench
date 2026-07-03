// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Components/ClimaGameStateComponent.h"
#include "Data/ClimaMicroClimateStructs.h"
#include "Data/ClimaEnums.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/BufferArchive.h"

/**
* @file ClimaGameStateComponent.cpp
* @brief Component for Game State to Manage In-Game Weather
*/
UClimaGameStateComponent::UClimaGameStateComponent()
{
	// Enable replication for this Actor
	SetIsReplicatedByDefault(true);
}

void UClimaGameStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate the microclimate data using the OnRep function
	DOREPLIFETIME(UClimaGameStateComponent, MicroclimateData);
}


void UClimaGameStateComponent::DecompressStructFromArchive(const TArray<uint8>& CompressedData)
{
	// FMyData DecompressedData;
	// ECompressionFlags CompressionFormat = COMPRESS_ForPackaging; // Must match the compression used to save
// 
	// // Create the loading proxy with the compressed data and format
	// FArchiveLoadCompressedProxy Uncompressor(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
// 
	// if (Uncompressor.IsError())
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Failed to create the FArchiveLoadCompressedProxy."));
	// 	return;
	// }
// 
	// // Deserialize the data directly into your struct variables
	// Uncompressor << DecompressedData.Name;
	// Uncompressor << DecompressedData.Value;
// 
	// // The data is now available in your struct
	// if (!Uncompressor.IsError())
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("Decompressed Name: %s"), *DecompressedData.Name);
	// 	UE_LOG(LogTemp, Log, TEXT("Decompressed Value: %d"), DecompressedData.Value);
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Failed to decompress and load data from archive."));
	// }
}

void UClimaGameStateComponent::OnRep_MicroclimateData()
{
	// Server has updated the array, so clients must regenerate their meshes
	for (FCompressedMicroclimateData& Data : MicroclimateData)
	{
		// Find or create a Procedural Mesh Component actor and update its mesh
		// This logic will vary based on your game's needs.
		// Example:
		// AProceduralMicroclimateActor* MicroclimateActor = FindMicroclimateActor(Data);
		// if (MicroclimateActor)
		// {
		//    MicroclimateActor->UpdateMeshFromData(Data);
		// }
	}
}

float UClimaGameStateComponent::GetAirDensityAtAltitude(float Altitude) const
{
	// Implement linear interpolation based on AirDensityCurve
	if (AirDensityCurvePoint.Num() < 2)
	{
		return AirDensityCurvePoint.Num() > 0 ? AirDensityCurvePoint[0].AirDensity : 1.225f;
	}

	// Find the two data points to interpolate between
	for (int32 i = 0; i < AirDensityCurvePoint.Num() - 1; ++i)
	{
		if (Altitude >= AirDensityCurvePoint[i].Altitude && Altitude < AirDensityCurvePoint[i+1].Altitude)
		{
			const FAtmosphereDataPoint& P1 = AirDensityCurvePoint[i];
			const FAtmosphereDataPoint& P2 = AirDensityCurvePoint[i+1];
			const float Alpha = (Altitude - P1.Altitude) / (P2.Altitude - P1.Altitude);
			return FMath::Lerp(P1.AirDensity, P2.AirDensity, Alpha);
		}
	}

	return AirDensityCurvePoint.Last().AirDensity;
}

float UClimaGameStateComponent::GetSpeedOfSoundAtAltitude(float Altitude) const
{
	// Implement linear interpolation based on SpeedOfSoundCurve, similar to GetAirDensity
	if (SpeedOfSoundCurvePoint.Num() < 2)
	{
		return SpeedOfSoundCurvePoint.Num() > 0 ? SpeedOfSoundCurvePoint[0].SpeedOfSound : 343.0f;
	}

	for (int32 i = 0; i < SpeedOfSoundCurvePoint.Num() - 1; ++i)
	{
		if (Altitude >= SpeedOfSoundCurvePoint[i].Altitude && Altitude < SpeedOfSoundCurvePoint[i+1].Altitude)
		{
			const FAtmosphereDataPoint& P1 = SpeedOfSoundCurvePoint[i];
			const FAtmosphereDataPoint& P2 = SpeedOfSoundCurvePoint[i+1];
			const float Alpha = (Altitude - P1.Altitude) / (P2.Altitude - P1.Altitude);
			return FMath::Lerp(P1.SpeedOfSound, P2.SpeedOfSound, Alpha);
		}
	}
    
	return SpeedOfSoundCurvePoint.Last().SpeedOfSound;
}

bool UClimaGameStateComponent::IsLocationInMicroclimate(FVector InVector, const FCompressedMicroclimateData& Microclimate) const
{
	// FBufferArchive CompressedDataArchive(CompressedBytes); // CompressedBytes is the TArray<uint8> holding the compressed data
	// FArchiveLoadCompressedProxy Decompressor(CompressedDataArchive, NAME_Zlib); // Or the same compression format used during saving
	// FCompressedMicroclimateData.NetSerialize(Decompressor, Ar, bIsLoading); // Ar is the FArchive used for replication
	// 
	// if (!MicroclimateData.IsEmpty())
	// {
	// 	for (FCompressedMicroclimateData Data : MicroclimateData)
	// 	{
	// 		TArray<FVector> Vertices = Data.MicroclimateShapeData.CompressedVertexData;
	// 		TArray<int> Indices = Data.MicroclimateShapeData.CompressedIndexData;
	// 		
	// 		for (int32 i = 0; i < Indices.Num(); i += 3)
	// 		{
	// 			FVector V0 = Vertices[Indices[i]];
	// 			FVector V1 = Vertices[Indices[i+1]];
	// 			FVector V2 = Vertices[Indices[i+2]];
    //     
	// 			// Transform vertices to world space
	// 			// V0 = GetActorTransform().TransformPosition(V0);
	// 			// V1 = GetActorTransform().TransformPosition(V1);
	// 			// V2 = GetActorTransform().TransformPosition(V2);
// 
	// 			// Perform sphere-triangle intersection test.
	// 			// This is a simplified example. For robustness, use more advanced
	// 			// math library functions.
	// 			FVector ClosestPointOnTriangle;
	// 			float SphereRadius = 1.0;
	// 			FMath::ClosestPointOnTriangleToPoint(InVector, V0, V1, V2, ClosestPointOnTriangle);
	// 			if (FVector::DistSquared(InVector, ClosestPointOnTriangle) <= SphereRadius * SphereRadius)
	// 			{
	// 				return true;
	// 			}
	// 	}
	// }
	// else
	// {
	// 	return false;
	// }
	return true;
}

FVector UClimaGameStateComponent::GetWindAtLocation(const FVector& InLocation) const
{
	for (const FCompressedMicroclimateData& Microclimate : MicroclimateData)
	{
		// Implement logic to check if Location is within Microclimate's bounds
		// e.g., using a bounding box or a custom shape intersection test
		if (IsLocationInMicroclimate(InLocation, Microclimate))
		{
			return Microclimate.DominantWindVector;
		}
	}
	return GlobalWind; // Fallback to global wind
}

void OnRep_MicroclimateData()
{
	
}
