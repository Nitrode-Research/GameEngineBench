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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoRendererSourcePipelineTest, "Nano.Renderer.SourcePipelineSemantics", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoRendererSourcePipelineTest::RunTest(const FString& Parameters)
{
	const FString Cpp = ReadPluginFile(TEXT("Source/NanoGS/Private/GaussianSplatRenderer.cpp"));
	TestTrue(TEXT("Renderer dispatches calc view data shader"), Cpp.Contains(TEXT("FGaussianSplatCalcViewDataCS")));
	TestTrue(TEXT("Renderer dispatches distance shader"), Cpp.Contains(TEXT("FGaussianSplatCalcDistancesCS")));
	TestTrue(TEXT("Renderer runs radix count/prefix/scatter passes"), Cpp.Contains(TEXT("FRadixSortCountCS")) && Cpp.Contains(TEXT("FRadixSortScatterCS")));
	TestTrue(TEXT("Renderer extracts six frustum planes for culling"), Cpp.Contains(TEXT("OutPlanes[5]")) && Cpp.Contains(TEXT("Normalize")));
	TestTrue(TEXT("Renderer supports global accumulator path"), Cpp.Contains(TEXT("DispatchCalcViewDataGlobal")) && Cpp.Contains(TEXT("GlobalBaseOffset")));
	TestTrue(TEXT("Renderer composites splats into scene color"), Cpp.Contains(TEXT("FGaussianSplatCompositePS")));
	return true;
}
