// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "ClimaMicroClimateStructs.generated.h"

USTRUCT(Blueprintable)
struct FCompressedMicroclimateShapeData
{
	GENERATED_BODY()

	// Parameters needed to recreate the mesh shape
	UPROPERTY()
	TArray<FVector> CompressedVertexData;

	UPROPERTY()
	TArray<int> CompressedIndexData;

	UPROPERTY()
	FVector DominantWindVector {FVector(0.0f,0.0f,0.0f)};

	UPROPERTY()
	float PreciseGasFactor {0.0f};

	// Optional: add parameters for the microclimate effect
	UPROPERTY()
	FVector MicroclimateCenter {FVector(0.0f,0.0f,0.0f)};

	UPROPERTY()
	float MicroclimateRadius {0.0f};
    
	// Custom serialization function for replicating the struct
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		/**
		 *@todo Replace this with your optimized compression logic, using FArchive and a custom compression algorithm
		 * */
		Ar << CompressedVertexData;
		Ar << CompressedIndexData;
		Ar << MicroclimateCenter;
		Ar << MicroclimateRadius;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FCompressedMicroclimateShapeData> : public TStructOpsTypeTraitsBase2<FCompressedMicroclimateShapeData>
{
	enum { WithNetSerializer = true };
};

USTRUCT(Blueprintable)
struct FCompressedMicroclimateData
{
	GENERATED_BODY()

	// Parameters needed to recreate the mesh shape
	UPROPERTY()
	FCompressedMicroclimateShapeData MicroclimateShapeData;

	UPROPERTY()
	FVector DominantWindVector {FVector(0.0f, 0.0f, 0.0f)};

	UPROPERTY()
	float PreciseGasFactor {0.0f};

	// Optional: add parameters for the microclimate effect
	UPROPERTY()
	FVector MicroclimateCenter {FVector(0.0f,0.0f,0.0f)};

	UPROPERTY()
	float MicroclimateRadius {0.0f};
    
	// Custom serialization function for replicating the struct
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		/**
		 *@todo Replace this with your optimized compression logic, using FArchive and a custom compression algorithm
		 * */
		Ar << DominantWindVector;
		Ar << MicroclimateCenter;
		Ar << MicroclimateRadius;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FCompressedMicroclimateData> : public TStructOpsTypeTraitsBase2<FCompressedMicroclimateData>
{
	enum { WithNetSerializer = true };
};
