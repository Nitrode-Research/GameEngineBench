#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GaussianSplatAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoAssetInitializeTest, "Nano.AssetManagement.InitializeBuildsCompressedData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoAssetInitializeTest::RunTest(const FString& Parameters)
{
	TArray<FGaussianSplatData> Splats;
	for (int32 Index = 0; Index < 300; ++Index)
	{
		FGaussianSplatData Splat;
		Splat.Position = FVector3f(Index, Index % 9, Index % 5);
		Splat.Scale = FVector3f(1.0f + 0.01f * Index);
		Splat.Rotation = FQuat4f::Identity;
		Splat.Opacity = 1.0f;
		Splat.SH_DC = FVector3f(0.1f, 0.2f, 0.3f);
		Splats.Add(Splat);
	}
	UGaussianSplatAsset* Asset = NewObject<UGaussianSplatAsset>();
	Asset->InitializeFromSplatData(Splats, EGaussianQualityLevel::Medium);
	TestEqual(TEXT("Splat count is preserved"), Asset->GetSplatCount(), Splats.Num());
	TestTrue(TEXT("Asset becomes valid after initialization"), Asset->IsValid());
	TestEqual(TEXT("Color texture width uses fixed swizzled width"), Asset->ColorTextureWidth, GaussianSplattingConstants::ColorTextureWidth);
	TestTrue(TEXT("Compressed position data is populated"), Asset->GetPositionDataSize() > 0);
	TestTrue(TEXT("Memory usage accounts for stored payload"), Asset->GetMemoryUsage() > 0);
	return true;
}
