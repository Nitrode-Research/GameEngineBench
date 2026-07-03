#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GaussianClusterBuilder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoClusterBuilderHierarchyTest, "Nano.ClusterBuilder.HierarchyBuildsLeafAndParentClusters", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoClusterBuilderHierarchyTest::RunTest(const FString& Parameters)
{
	TArray<FGaussianSplatData> Splats;
	for (int32 Index = 0; Index < 300; ++Index)
	{
		FGaussianSplatData Splat;
		Splat.Position = FVector3f(float(299 - Index), float(Index % 17), float(Index / 17));
		Splat.Scale = FVector3f(1.0f);
		Splat.Rotation = FQuat4f::Identity;
		Splat.Opacity = 1.0f;
		Splats.Add(Splat);
	}
	FGaussianClusterHierarchy Hierarchy;
	FGaussianClusterBuilder::FBuildSettings Settings;
	Settings.SplatsPerCluster = 128;
	Settings.MaxChildrenPerCluster = 8;
	Settings.bGenerateLOD = true;
	TestTrue(TEXT("BuildClusterHierarchy succeeds for non-empty splat data"), FGaussianClusterBuilder::BuildClusterHierarchy(Splats, Hierarchy, Settings));
	TestEqual(TEXT("Three leaf clusters are produced for 300 splats at 128 splats per leaf"), int32(Hierarchy.NumLeafClusters), 3);
	TestTrue(TEXT("Hierarchy contains parent/root clusters beyond leaves"), Hierarchy.Clusters.Num() > int32(Hierarchy.NumLeafClusters));
	TestTrue(TEXT("Root cluster index is valid"), int32(Hierarchy.RootClusterIndex) < Hierarchy.Clusters.Num());
	TestTrue(TEXT("LOD splats are generated for parent clusters"), Hierarchy.TotalLODSplatCount > 0);
	return true;
}
