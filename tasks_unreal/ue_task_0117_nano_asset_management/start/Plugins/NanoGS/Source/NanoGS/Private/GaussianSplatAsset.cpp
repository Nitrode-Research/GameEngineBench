// Copyright Epic Games, Inc. All Rights Reserved.

#include "GaussianSplatAsset.h"
#include "GaussianSplatRenderData.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "GaussianClusterBuilder.h"
#include "PLYFileReader.h"

#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#endif

UGaussianSplatAsset::UGaussianSplatAsset()
{
	BoundingBox.Init();
}

TSharedPtr<FGaussianSplatRenderData> UGaussianSplatAsset::GetOrCreateRenderData()
{
	if (RenderData.IsValid() && RenderData->IsInitialized())
	{
		UE_LOG(LogTemp, Verbose, TEXT("GaussianSplatRenderData: Reusing existing shared data for asset '%s'"),
			*GetName());
		return RenderData;
	}

	RenderData = MakeShared<FGaussianSplatRenderData>();
	RenderData->Initialize(this);
	return RenderData;
}

void UGaussianSplatAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	int32 Magic = GAUSSIAN_SPLAT_ASSET_MAGIC;
	int32 Version = GAUSSIAN_SPLAT_ASSET_VERSION;
	Ar << Magic;
	Ar << Version;

	if (Ar.IsLoading() && (Magic != GAUSSIAN_SPLAT_ASSET_MAGIC || Version != GAUSSIAN_SPLAT_ASSET_VERSION))
	{
		UE_LOG(LogTemp, Error, TEXT("GaussianSplatAsset: Incompatible asset format (Magic=0x%08X, Version=%d). Please reimport the asset."), Magic, Version);
		return;
	}

	Ar << SplatCount;
	Ar << BoundingBox;
	Ar << PositionFormat;
	Ar << ColorFormat;
	Ar << SHFormat;
	Ar << SHBands;
	Ar << SourceFilePath;
	Ar << ImportQuality;
	Ar << ColorTextureWidth;
	Ar << ColorTextureHeight;
	Ar << ChunkData;

	PositionBulkData.Serialize(Ar, this);
	OtherBulkData.Serialize(Ar, this);
	SHBulkData.Serialize(Ar, this);
	ColorTextureBulkData.Serialize(Ar, this);

	// Nanite-related fields (Version 4+)
	Ar << bEnableNanite;
	Ar << OriginalSplatCount;
	Ar << ClusterHierarchy;
}

void UGaussianSplatAsset::PostLoad()
{
	Super::PostLoad();

	const int64 ColorDataSize = ColorTextureBulkData.GetBulkDataSize();
	UE_LOG(LogTemp, Log, TEXT("GaussianSplatAsset::PostLoad - SplatCount=%d, ColorTextureBulkData.Size=%lld, Width=%d, Height=%d"),
		SplatCount, ColorDataSize, ColorTextureWidth, ColorTextureHeight);

	// Recreate the ColorTexture from the stored bulk data
	if (ColorDataSize > 0 && ColorTextureWidth > 0 && ColorTextureHeight > 0)
	{
		CreateColorTextureFromData();
		UE_LOG(LogTemp, Log, TEXT("GaussianSplatAsset::PostLoad - ColorTexture recreated successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GaussianSplatAsset::PostLoad - No ColorTextureBulkData to restore (might be old asset format)"));
	}

#if WITH_EDITORONLY_DATA
	// Recreate the ThumbnailTexture from stored pixel data
	if (ThumbnailData.Num() > 0 && ThumbnailSize > 0)
	{
		CreateThumbnailTextureFromData();
		UE_LOG(LogTemp, Log, TEXT("GaussianSplatAsset::PostLoad - ThumbnailTexture recreated successfully"));
	}
#endif
}

#if WITH_EDITOR
void UGaussianSplatAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

int64 UGaussianSplatAsset::GetMemoryUsage() const
{
	return 0;
}

void UGaussianSplatAsset::InitializeFromSplatData(const TArray<FGaussianSplatData>& InSplats, EGaussianQualityLevel InQuality)
{
	SplatCount = 0;
	BoundingBox.Init();
	return;
}

int32 UGaussianSplatAsset::GetPositionBytesPerSplat(EGaussianPositionFormat Format)
{
	switch (Format)
	{
	case EGaussianPositionFormat::Float32: return 12; // 3 x 4 bytes
	case EGaussianPositionFormat::Norm16:  return 6;  // 3 x 2 bytes
	case EGaussianPositionFormat::Norm11:  return 4;  // 11+10+11 bits packed
	case EGaussianPositionFormat::Norm6:   return 2;  // 6+5+5 bits packed
	default: return 12;
	}
}

int32 UGaussianSplatAsset::GetColorBytesPerSplat(EGaussianColorFormat Format)
{
	switch (Format)
	{
	case EGaussianColorFormat::Float32x4: return 16; // 4 x 4 bytes
	case EGaussianColorFormat::Float16x4: return 8;  // 4 x 2 bytes
	case EGaussianColorFormat::Norm8x4:   return 4;  // 4 x 1 byte
	case EGaussianColorFormat::BC7:       return 1;  // Approximate
	default: return 8;
	}
}

int32 UGaussianSplatAsset::GetSHBytesPerSplat(EGaussianSHFormat Format, int32 Bands)
{
	// SH buffer now includes DC coefficient for view-dependent rendering
	// band0=0 (no buffer), band1=4 (DC + 3), band2=9 (DC + 8), band3=16 (DC + 15)
	int32 NumCoeffs = 0;
	switch (Bands)
	{
	case 0: NumCoeffs = 0; break;   // DC only (stored in color, no SH buffer)
	case 1: NumCoeffs = 4; break;   // DC + 3 coeffs
	case 2: NumCoeffs = 9; break;   // DC + 3 + 5 coeffs
	case 3: NumCoeffs = 16; break;  // DC + 3 + 5 + 7 coeffs
	default: NumCoeffs = 16; break;
	}

	// Each coefficient has 3 color channels
	int32 TotalValues = NumCoeffs * 3;

	switch (Format)
	{
	case EGaussianSHFormat::Float32: return TotalValues * 4;
	case EGaussianSHFormat::Float16: return TotalValues * 2;
	case EGaussianSHFormat::Norm11:  return (TotalValues * 11 + 7) / 8; // Approximate
	case EGaussianSHFormat::Norm6:   return (TotalValues * 6 + 7) / 8;  // Approximate
	default: return TotalValues * 2; // Assume Float16
	}
}

void UGaussianSplatAsset::CalculateBounds(const TArray<FGaussianSplatData>& InSplats)
{
	BoundingBox.Init();
	return;
}

TArray<FVector> UGaussianSplatAsset::GetDecompressedPositions() const
{
	TArray<FVector> Positions;

	const int64 DataSize = PositionBulkData.GetBulkDataSize();
	if (SplatCount == 0 || DataSize == 0)
	{
		return Positions;
	}

	Positions.SetNum(SplatCount);

	// Lock bulk data for reading
	const uint8* DataPtr = static_cast<const uint8*>(PositionBulkData.LockReadOnly());

	// Simplified: Always Float32 format (12 bytes per position)
	const int32 BytesPerSplat = 12;

	for (int32 i = 0; i < SplatCount; i++)
	{
		const float* FloatPtr = reinterpret_cast<const float*>(DataPtr + i * BytesPerSplat);
		Positions[i] = FVector(FloatPtr[0], FloatPtr[1], FloatPtr[2]);
	}

	PositionBulkData.Unlock();

	return Positions;
}

void UGaussianSplatAsset::CalculateChunkBounds(const TArray<FGaussianSplatData>& InSplats)
{
	ChunkData.Empty();
	return;
}

void UGaussianSplatAsset::CompressPositions(const TArray<FGaussianSplatData>& InSplats)
{
	PositionBulkData.RemoveBulkData();
	return;
}

void UGaussianSplatAsset::CompressRotationScale(const TArray<FGaussianSplatData>& InSplats)
{
	OtherBulkData.RemoveBulkData();
	return;
}

void UGaussianSplatAsset::CreateColorTextureData(const TArray<FGaussianSplatData>& InSplats)
{
	ColorTextureBulkData.RemoveBulkData();
	ColorTextureWidth = 0;
	ColorTextureHeight = 0;
	return;
}

void UGaussianSplatAsset::CreateColorTextureFromData()
{
	return;
}

void UGaussianSplatAsset::CompressSH(const TArray<FGaussianSplatData>& InSplats)
{
	SHBulkData.RemoveBulkData();
	return;
}

void UGaussianSplatAsset::GetPositionData(TArray<uint8>& OutData) const
{
	const int64 DataSize = PositionBulkData.GetBulkDataSize();
	if (DataSize > 0)
	{
		OutData.SetNum(static_cast<int32>(DataSize));
		const void* SrcData = PositionBulkData.LockReadOnly();
		FMemory::Memcpy(OutData.GetData(), SrcData, DataSize);
		PositionBulkData.Unlock();
	}
	else
	{
		OutData.Empty();
	}
}

void UGaussianSplatAsset::GetOtherData(TArray<uint8>& OutData) const
{
	const int64 DataSize = OtherBulkData.GetBulkDataSize();
	if (DataSize > 0)
	{
		OutData.SetNum(static_cast<int32>(DataSize));
		const void* SrcData = OtherBulkData.LockReadOnly();
		FMemory::Memcpy(OutData.GetData(), SrcData, DataSize);
		OtherBulkData.Unlock();
	}
	else
	{
		OutData.Empty();
	}
}

void UGaussianSplatAsset::GetSHData(TArray<uint8>& OutData) const
{
	const int64 DataSize = SHBulkData.GetBulkDataSize();
	if (DataSize > 0)
	{
		OutData.SetNum(static_cast<int32>(DataSize));
		const void* SrcData = SHBulkData.LockReadOnly();
		FMemory::Memcpy(OutData.GetData(), SrcData, DataSize);
		SHBulkData.Unlock();
	}
	else
	{
		OutData.Empty();
	}
}

void UGaussianSplatAsset::GetColorTextureData(TArray<uint8>& OutData) const
{
	const int64 DataSize = ColorTextureBulkData.GetBulkDataSize();
	if (DataSize > 0)
	{
		OutData.SetNum(static_cast<int32>(DataSize));
		const void* SrcData = ColorTextureBulkData.LockReadOnly();
		FMemory::Memcpy(OutData.GetData(), SrcData, DataSize);
		ColorTextureBulkData.Unlock();
	}
	else
	{
		OutData.Empty();
	}
}

#if WITH_EDITOR
bool UGaussianSplatAsset::BuildNaniteClusterHierarchy()
{
	return false;
}

void UGaussianSplatAsset::ClearNaniteClusterHierarchy()
{
	ClusterHierarchy.Reset();
	bEnableNanite = false;
	return;
}

void UGaussianSplatAsset::GenerateThumbnail(const TArray<FGaussianSplatData>& InSplats)
{
	if (InSplats.IsEmpty()) return;

	ThumbnailSize = 256;

	// Initialize pixel buffer with a dark background
	TArray<FColor> Pixels;
	Pixels.Init(FColor(30, 30, 30, 255), ThumbnailSize * ThumbnailSize);

	// Depth buffer: keep the front-most (min depth) splat per pixel
	TArray<float> DepthBuffer;
	DepthBuffer.Init(TNumericLimits<float>::Max(), ThumbnailSize * ThumbnailSize);

	// ------------------------------------------------------------------
	// Fixed camera: 45° yaw, -25° pitch, orthographic projection
	// ------------------------------------------------------------------
	const float PitchRad = FMath::DegreesToRadians(-25.f);
	const float YawRad   = FMath::DegreesToRadians(45.f);

	// Camera's forward vector (direction it is looking)
	const FVector CamForward(
		FMath::Cos(PitchRad) * FMath::Cos(YawRad),
		FMath::Cos(PitchRad) * FMath::Sin(YawRad),
		FMath::Sin(PitchRad)
	);
	const FVector CamRight = FVector::CrossProduct(CamForward, FVector::UpVector).GetSafeNormal();
	const FVector CamUp    = FVector::CrossProduct(CamRight,   CamForward).GetSafeNormal();

	// Scale: find the half-extent of the bounds projected onto the camera plane
	const FVector Center = BoundingBox.GetCenter();
	const FVector Extent = BoundingBox.GetExtent();

	float MaxExtent = 0.f;
	for (int32 sx : {-1, 1}) for (int32 sy : {-1, 1}) for (int32 sz : {-1, 1})
	{
		const FVector Corner = Extent * FVector(sx, sy, sz);
		MaxExtent = FMath::Max(MaxExtent, FMath::Abs(FVector::DotProduct(Corner, CamRight)));
		MaxExtent = FMath::Max(MaxExtent, FMath::Abs(FVector::DotProduct(Corner, CamUp)));
	}
	MaxExtent = FMath::Max(MaxExtent, 1.f) * 1.1f; // 10 % padding

	// ------------------------------------------------------------------
	// Rasterize: stride so we don't spend more than ~200k iterations
	// ------------------------------------------------------------------
	const int32 Step = FMath::Max(1, InSplats.Num() / 200000);

	for (int32 i = 0; i < InSplats.Num(); i += Step)
	{
		const FGaussianSplatData& Splat = InSplats[i];
		const FVector Rel = FVector(Splat.Position) - Center;

		const float U     =  FVector::DotProduct(Rel, CamRight);
		const float V     =  FVector::DotProduct(Rel, CamUp);
		const float Depth =  FVector::DotProduct(Rel, CamForward); // smaller = closer

		const float NormU = (U / MaxExtent + 1.f) * 0.5f;
		const float NormV = (1.f - (V / MaxExtent + 1.f) * 0.5f); // flip Y

		const int32 PX = FMath::Clamp((int32)(NormU * ThumbnailSize), 0, ThumbnailSize - 1);
		const int32 PY = FMath::Clamp((int32)(NormV * ThumbnailSize), 0, ThumbnailSize - 1);
		const int32 PixIdx = PY * ThumbnailSize + PX;

		if (Depth < DepthBuffer[PixIdx])
		{
			DepthBuffer[PixIdx] = Depth;

			// SH DC → sRGB-ish base color (clamped, linear → sRGB gamma)
			const FVector3f BaseColor = GaussianSplattingUtils::SHDCToColor(Splat.SH_DC);
			const auto ToSRGB = [](float Lin) -> uint8
			{
				float Clamped = FMath::Clamp(Lin, 0.f, 1.f);
				return (uint8)(FMath::Pow(Clamped, 1.f / 2.2f) * 255.f + 0.5f);
			};

			Pixels[PixIdx] = FColor(ToSRGB(BaseColor.X), ToSRGB(BaseColor.Y), ToSRGB(BaseColor.Z), 255);
		}
	}

	// ------------------------------------------------------------------
	// Store pixel data persistently (will be serialized with the asset)
	// ------------------------------------------------------------------
	const int32 NumBytes = ThumbnailSize * ThumbnailSize * 4;
	ThumbnailData.SetNum(NumBytes);

	// FColor is RGBA; we store as BGRA for PF_B8G8R8A8 format
	for (int32 Idx = 0; Idx < ThumbnailSize * ThumbnailSize; ++Idx)
	{
		ThumbnailData[Idx * 4 + 0] = Pixels[Idx].B;
		ThumbnailData[Idx * 4 + 1] = Pixels[Idx].G;
		ThumbnailData[Idx * 4 + 2] = Pixels[Idx].R;
		ThumbnailData[Idx * 4 + 3] = Pixels[Idx].A;
	}

	// Create the texture from the stored data
	CreateThumbnailTextureFromData();

	UE_LOG(LogTemp, Log, TEXT("GaussianSplatAsset: Thumbnail generated (%dx%d)"), ThumbnailSize, ThumbnailSize);
}

void UGaussianSplatAsset::CreateThumbnailTextureFromData()
{
	if (ThumbnailData.Num() == 0 || ThumbnailSize <= 0)
	{
		return;
	}

	// Create a transient texture (not saved, recreated on load)
	ThumbnailTexture = NewObject<UTexture2D>(this, NAME_None, RF_Transient);

	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	PlatformData->SizeX       = ThumbnailSize;
	PlatformData->SizeY       = ThumbnailSize;
	PlatformData->PixelFormat = PF_B8G8R8A8;
	ThumbnailTexture->SetPlatformData(PlatformData);

	ThumbnailTexture->SRGB             = true;
	ThumbnailTexture->CompressionSettings = TC_Default;
	ThumbnailTexture->Filter           = TF_Bilinear;
	ThumbnailTexture->NeverStream      = true;
	ThumbnailTexture->MipGenSettings   = TMGS_NoMipmaps;

	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	PlatformData->Mips.Add(Mip);
	Mip->SizeX = ThumbnailSize;
	Mip->SizeY = ThumbnailSize;

	// Copy the stored pixel data to the mip
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* MipData = reinterpret_cast<uint8*>(Mip->BulkData.Realloc(ThumbnailData.Num()));
	FMemory::Memcpy(MipData, ThumbnailData.GetData(), ThumbnailData.Num());
	Mip->BulkData.Unlock();

	ThumbnailTexture->UpdateResource();
}
#endif
