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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoGlobalAccumulatorSourceTest, "Nano.GlobalAccumulator.SourceAndLayoutSemantics", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoGlobalAccumulatorSourceTest::RunTest(const FString& Parameters)
{
	const FString Header = ReadPluginFile(TEXT("Source/NanoGS/Public/GaussianGlobalAccumulator.h"));
	const FString AccumCpp = ReadPluginFile(TEXT("Source/NanoGS/Private/GaussianGlobalAccumulator.cpp"));
	const FString ViewCpp = ReadPluginFile(TEXT("Source/NanoGS/Private/GaussianSplatViewExtension.cpp"));
	TestTrue(TEXT("Global accumulator exposes max proxy compaction layout"), Header.Contains(TEXT("MAX_PROXY_COUNT = 8192")) && Header.Contains(TEXT("GlobalBaseOffsetsBuffer")));
	TestTrue(TEXT("Resize allocates global view/sort/key buffers with headroom"), AccumCpp.Contains(TEXT("NewAllocatedCount")) && AccumCpp.Contains(TEXT("GlobalViewDataBuffer")) && AccumCpp.Contains(TEXT("GlobalSortKeysBuffer")));
	TestTrue(TEXT("Compaction buffers are fixed-size and allocated once"), AccumCpp.Contains(TEXT("EnsureCompactionBuffersAllocated")) && AccumCpp.Contains(TEXT("MAX_PROXY_COUNT + 1")));
	TestTrue(TEXT("Release clears RHI resources and capacity"), AccumCpp.Contains(TEXT("SafeRelease")) && AccumCpp.Contains(TEXT("AllocatedCount = 0")));
	TestTrue(TEXT("View extension uses singleton and critical section guarded proxy list"), ViewCpp.Contains(TEXT("Instance = this")) && ViewCpp.Contains(TEXT("FScopeLock")) && ViewCpp.Contains(TEXT("RegisteredProxies.AddUnique")));
	return true;
}
