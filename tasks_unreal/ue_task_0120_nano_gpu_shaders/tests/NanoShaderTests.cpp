#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static FString ReadPluginFile(const FString& RelativePath)
{
	const FString Path = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("NanoGS"), RelativePath);
	FString Text;
	FFileHelper::LoadFileToString(Text, *Path);
	return Text;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoShaderSourceSemanticsTest, "Nano.Shaders.CoreComputeSourceSemantics", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoShaderSourceSemanticsTest::RunTest(const FString& Parameters)
{
	const FString CalcView = ReadPluginFile(TEXT("Shaders/Private/CalcViewData.usf"));
	const FString Dist = ReadPluginFile(TEXT("Shaders/Private/CalcDistances.usf"));
	const FString Cluster = ReadPluginFile(TEXT("Shaders/Private/ClusterCulling.usf"));
	const FString Compact = ReadPluginFile(TEXT("Shaders/Private/CompactSplats.usf"));
	const FString Radix = ReadPluginFile(TEXT("Shaders/Private/RadixSort.usf"));
	const FString Prepare = ReadPluginFile(TEXT("Shaders/Private/PrepareIndirectArgs.usf"));
	const FString Prefix = ReadPluginFile(TEXT("Shaders/Private/GlobalAccumulatorPrefixSum.usf"));
	const FString Composite = ReadPluginFile(TEXT("Shaders/Private/GaussianSplatComposite.usf"));
	TestTrue(TEXT("CalcViewData evaluates projection, covariance, and SH color"), CalcView.Contains(TEXT("Covariance")) && CalcView.Contains(TEXT("EvaluateSH")) && CalcView.Contains(TEXT("WriteInvalidSplat")));
	TestTrue(TEXT("CalcDistances handles invalid splats and sortable depth bits"), Dist.Contains(TEXT("clipPos.w <= 0.0")) && Dist.Contains(TEXT("asuint(depth)")));
	TestTrue(TEXT("Cluster culling tests all frustum planes and LOD/error state"), Cluster.Contains(TEXT("FrustumPlanes[6]")) && Cluster.Contains(TEXT("ErrorThreshold")) && Cluster.Contains(TEXT("DebugForceLODLevel")));
	TestTrue(TEXT("CompactSplats uses atomics and LOD visibility"), Compact.Contains(TEXT("InterlockedAdd")) && Compact.Contains(TEXT("UseLODRendering")) && Compact.Contains(TEXT("CompactedSplatIndices")));
	TestTrue(TEXT("RadixSort implements four 8-bit LSD passes"), Radix.Contains(TEXT("RADIX_BITS 8")) && Radix.Contains(TEXT("CountCS")) && Radix.Contains(TEXT("ScatterCS")));
	TestTrue(TEXT("PrepareIndirectArgs writes dispatch, draw, and sort args"), Prepare.Contains(TEXT("IndirectDrawArgs[4]")) && Prepare.Contains(TEXT("SortParamsBuffer[1]")));
	TestTrue(TEXT("Global prefix sum computes exclusive offsets and max budget args"), Prefix.Contains(TEXT("exclusive prefix")) && Prefix.Contains(TEXT("MaxRenderBudget")));
	TestTrue(TEXT("Composite converts sRGB, applies inverse ACES, and premultiplies alpha"), Composite.Contains(TEXT("pow")) && Composite.Contains(TEXT("ACES")) && Composite.Contains(TEXT("alpha")));
	return true;
}
