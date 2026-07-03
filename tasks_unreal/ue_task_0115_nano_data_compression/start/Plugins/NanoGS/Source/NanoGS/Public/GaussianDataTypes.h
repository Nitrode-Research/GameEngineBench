// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GaussianDataTypes.generated.h"

/**
 * Position data compression format
 */
UENUM(BlueprintType)
enum class EGaussianPositionFormat : uint8
{
	Float32		UMETA(DisplayName = "Float32 (12 bytes)"),
	Norm16		UMETA(DisplayName = "Norm16 (6 bytes)"),
	Norm11		UMETA(DisplayName = "Norm11 (4 bytes)"),
	Norm6		UMETA(DisplayName = "Norm6 (2 bytes)")
};

/**
 * Color/opacity data compression format
 */
UENUM(BlueprintType)
enum class EGaussianColorFormat : uint8
{
	Float32x4	UMETA(DisplayName = "Float32x4 (16 bytes)"),
	Float16x4	UMETA(DisplayName = "Float16x4 (8 bytes)"),
	Norm8x4		UMETA(DisplayName = "Norm8x4 (4 bytes)"),
	BC7			UMETA(DisplayName = "BC7 (~1 byte)")
};

/**
 * Spherical harmonics compression format
 */
UENUM(BlueprintType)
enum class EGaussianSHFormat : uint8
{
	Float32		UMETA(DisplayName = "Float32"),
	Float16		UMETA(DisplayName = "Float16"),
	Norm11		UMETA(DisplayName = "Norm11"),
	Norm6		UMETA(DisplayName = "Norm6"),
	Cluster4k	UMETA(DisplayName = "Cluster 4k"),
	Cluster8k	UMETA(DisplayName = "Cluster 8k"),
	Cluster16k	UMETA(DisplayName = "Cluster 16k"),
	Cluster32k	UMETA(DisplayName = "Cluster 32k"),
	Cluster64k	UMETA(DisplayName = "Cluster 64k")
};

/**
 * Quality level preset for import
 */
UENUM(BlueprintType)
enum class EGaussianQualityLevel : uint8
{
	VeryHigh	UMETA(DisplayName = "Very High (~48 bytes/splat)"),
	High		UMETA(DisplayName = "High (~24 bytes/splat)"),
	Medium		UMETA(DisplayName = "Medium (~12 bytes/splat)"),
	Low			UMETA(DisplayName = "Low (~8 bytes/splat)"),
	VeryLow		UMETA(DisplayName = "Very Low (~4 bytes/splat)")
};

/**
 * Raw splat data read from PLY file (CPU-side, before compression)
 * Total: ~248 bytes per splat
 */
USTRUCT()
struct FGaussianSplatData
{
	GENERATED_BODY()

	/** World position */
	FVector3f Position = FVector3f::ZeroVector;

	/** Orientation quaternion */
	FQuat4f Rotation = FQuat4f::Identity;

	/** 3D scale factors */
	FVector3f Scale = FVector3f::OneVector;

	/** Alpha opacity [0,1] */
	float Opacity = 1.0f;

	/** Spherical harmonic band 0 (base color, DC term) */
	FVector3f SH_DC = FVector3f::ZeroVector;

	/** Spherical harmonic bands 1-3 (15 coefficients, each RGB) */
	FVector3f SH[15];

	FGaussianSplatData()
	{
		for (int32 i = 0; i < 15; i++)
		{
			SH[i] = FVector3f::ZeroVector;
		}
	}

	/** Serialization */
	friend FArchive& operator<<(FArchive& Ar, FGaussianSplatData& Splat)
	{
		Ar << Splat.Position;
		Ar << Splat.Rotation;
		Ar << Splat.Scale;
		Ar << Splat.Opacity;
		Ar << Splat.SH_DC;
		for (int32 i = 0; i < 15; i++)
		{
			Ar << Splat.SH[i];
		}
		return Ar;
	}
};

/**
 * Per-frame view data computed by compute shader, used by vertex shader
 * This structure must match the HLSL definition in GaussianDataTypes.ush
 * Total: 64 bytes per splat (with TranslatedWorldPos for velocity, 16-byte aligned)
 */
USTRUCT()
struct FGaussianSplatViewData
{
	GENERATED_BODY()

	/** Clip space position (xyz/w) */
	FVector4f ClipPosition = FVector4f::Zero();

	/** Translated world position for velocity calculation (world + PreViewTranslation) */
	FVector3f TranslatedWorldPos = FVector3f::ZeroVector;

	/** Padding after TranslatedWorldPos for alignment */
	float TranslatedWorldPosPad = 0.0f;

	/** Half-float packed R,G channels */
	uint32 PackedColorRG = 0;

	/** Half-float packed B,A channels */
	uint32 PackedColorBA = 0;

	/** 2D covariance principal axis 1 (screen space) */
	FVector2f Axis1 = FVector2f::ZeroVector;

	/** 2D covariance principal axis 2 (screen space) */
	FVector2f Axis2 = FVector2f::ZeroVector;

	/** Cluster ID for debug visualization (Nanite-style) */
	uint32 ClusterID = 0;

	/** Padding for 16-byte alignment */
	uint32 Padding = 0;
};

/**
 * Chunk info for quantized/compressed data
 * Each chunk contains 256 splats with shared min/max bounds for dequantization
 * Total: 72 bytes per chunk
 */
USTRUCT()
struct FGaussianChunkInfo
{
	GENERATED_BODY()

	/** Position min/max per axis (for dequantization) */
	FVector2f PosMinMaxX = FVector2f::ZeroVector;
	FVector2f PosMinMaxY = FVector2f::ZeroVector;
	FVector2f PosMinMaxZ = FVector2f::ZeroVector;

	/** Color channel min/max (packed) */
	uint32 ColorMinMaxR = 0;
	uint32 ColorMinMaxG = 0;
	uint32 ColorMinMaxB = 0;
	uint32 ColorMinMaxA = 0;

	/** Scale min/max (packed) */
	uint32 ScaleMinMaxX = 0;
	uint32 ScaleMinMaxY = 0;
	uint32 ScaleMinMaxZ = 0;

	/** SH min/max (packed) */
	uint32 SHMinMaxR = 0;
	uint32 SHMinMaxG = 0;
	uint32 SHMinMaxB = 0;

	/** Serialization */
	friend FArchive& operator<<(FArchive& Ar, FGaussianChunkInfo& Chunk)
	{
		Ar << Chunk.PosMinMaxX;
		Ar << Chunk.PosMinMaxY;
		Ar << Chunk.PosMinMaxZ;
		Ar << Chunk.ColorMinMaxR;
		Ar << Chunk.ColorMinMaxG;
		Ar << Chunk.ColorMinMaxB;
		Ar << Chunk.ColorMinMaxA;
		Ar << Chunk.ScaleMinMaxX;
		Ar << Chunk.ScaleMinMaxY;
		Ar << Chunk.ScaleMinMaxZ;
		Ar << Chunk.SHMinMaxR;
		Ar << Chunk.SHMinMaxG;
		Ar << Chunk.SHMinMaxB;
		return Ar;
	}
};

/**
 * Constants for Gaussian Splatting
 */
namespace GaussianSplattingConstants
{
	/** Number of splats per chunk for quantization */
	constexpr int32 SplatsPerChunk = 256;

	/** Color texture width (Morton-swizzled) */
	constexpr int32 ColorTextureWidth = 2048;

	/** Morton tile size for texture swizzling */
	constexpr int32 MortonTileSize = 16;

	/** Number of SH coefficients per color channel (bands 1-3) */
	constexpr int32 NumSHCoefficients = 15;

	/** SH C0 coefficient for converting SH to color */
	constexpr float SH_C0 = 0.28209479177387814f;

	/** Maximum supported SH order (0-3) */
	constexpr int32 MaxSHOrder = 3;

	/** Log-scale encoding range for packed splat format (in centimeters) */
	constexpr float LN_SCALE_MIN = -8.0f;   // covers ~0.0003 cm
	constexpr float LN_SCALE_MAX = 14.0f;    // covers ~1.2 million cm

	/** Packed splat stride in bytes (4 x uint32) */
	constexpr int32 PackedSplatStride = 16;
}

/**
 * Helper functions for bit packing/unpacking
 */
namespace GaussianSplattingUtils
{
	/** Pack a float to 16-bit half float representation */
	inline uint16 FloatToHalf(float Value)
	{
	return 0;
}

	/** Unpack 16-bit half float to float */
	inline float HalfToFloat(uint16 Value)
	{
	return 0.0f;
}

	/** Pack two half floats into a uint32 */
	inline uint32 PackHalf2x16(float A, float B)
	{
	return 0;
}

	/** Unpack uint32 to two floats */
	inline void UnpackHalf2x16(uint32 Packed, float& OutA, float& OutB)
	{
	OutA = 0.0f;
	OutB = 0.0f;
	return;
}

	/** Encode 2D coordinate to Morton code (16x16 tile) */
	inline uint32 EncodeMorton2D_16x16(uint32 X, uint32 Y)
	{
	return 0;
}

	/** Decode Morton code to 2D coordinate (16x16 tile) */
	inline void DecodeMorton2D_16x16(uint32 Morton, uint32& OutX, uint32& OutY)
	{
	OutX = 0;
	OutY = 0;
	return;
}

	/** Convert splat index to pixel coordinates in color texture (Morton-swizzled) */
	inline void SplatIndexToTextureCoord(int32 SplatIndex, int32 TextureWidth, int32& OutX, int32& OutY)
	{
	OutX = 0;
	OutY = 0;
	return;
}

	/** Apply sigmoid function: 1 / (1 + exp(-x)) */
	inline float Sigmoid(float X)
	{
		return 1.0f / (1.0f + FMath::Exp(-X));
	}

	/** Convert SH DC component to base color */
	inline FVector3f SHDCToColor(const FVector3f& SHDC)
	{
	return FVector3f::ZeroVector;
}

	/** Normalize quaternion */
	inline FQuat4f NormalizeQuat(const FQuat4f& Q)
	{
	return FQuat4f::Identity;
}

	/** Encode a linear scale value to uint8 using log encoding */
	inline uint8 EncodeScaleUint8(float Scale)
	{
	return 0;
}

	/** Encode quaternion to octahedral axis-angle representation (3 bytes: U, V, Angle) */
	inline void EncodeQuatOctahedral(FQuat4f Q, uint8& OutU, uint8& OutV, uint8& OutAngle)
	{
	OutU = 0;
	OutV = 0;
	OutAngle = 0;
	return;
}

	/**
	 * Pack a single splat into 4 uint32 words (16 bytes total).
	 * Layout matches SparkJS PackedSplats format.
	 * @param Position UE centimeter coordinates (float16 covers ±655m from origin)
	 * @param Rotation UE convention quaternion (X,Y,Z,W)
	 * @param Scale UE centimeter linear scale
	 * @param ColorR/G/B sRGB color [0,1] (clamped to uint8)
	 * @param Opacity Linear opacity [0,1]
	 */
	inline void PackSplatToUint4(
		const FVector3f& Position,
		const FQuat4f& Rotation,
		const FVector3f& Scale,
		float ColorR, float ColorG, float ColorB, float Opacity,
		uint32 OutWords[4])
	{
	OutWords[0] = OutWords[1] = OutWords[2] = OutWords[3] = 0;
	return;
}
}
