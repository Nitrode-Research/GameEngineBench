// Copyright 2026 GameDevBench. Instanced static mesh converter source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInstancedStaticMeshConverter_SourceContract,
	"Bomber.InstancedStaticMeshConverter.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInstancedStaticMeshConverter_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates the converter's component lifecycle, actor extraction, material handling, transform semantics, and cleanup.
	const FString SourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("Plugins/InstancedStaticMeshConverter/Source/InstancedStaticMeshConverter/Private/InstancedStaticMeshActor.cpp"));

	FString Source;
	if (!TestTrue(TEXT("InstancedStaticMeshActor source readable"), FFileHelper::LoadFileToString(Source, *SourcePath)))
	{
		return true;
	}

	TestTrue(TEXT("Direct mesh spawning must create and configure instanced static mesh components"),
		Source.Contains(TEXT("NewObject<UInstancedStaticMeshComponent>")) && Source.Contains(TEXT("RegisterComponent"))
		&& Source.Contains(TEXT("AttachToComponent")) && Source.Contains(TEXT("SetStaticMesh"))
		&& Source.Contains(TEXT("SetCanEverAffectNavigation(false)")));
	TestTrue(TEXT("Direct mesh spawning must cache and reuse components by mesh"),
		Source.Contains(TEXT("CachedBlueprintMeshes")) && Source.Contains(TEXT("InstancedStaticMeshData.StaticMesh == Mesh"))
		&& Source.Contains(TEXT("Emplace_GetRef")) && Source.Contains(TEXT("AddInstance(Transform)")));
	TestTrue(TEXT("Actor conversion must find or create cached mesh data"),
		Source.Contains(TEXT("FindCachedActorMeshInstances")) && Source.Contains(TEXT("FindOrCreateInstancedMeshes"))
		&& Source.Contains(TEXT("NewActorMeshInstance.ActorBlueprint = ActorClass")));
	TestTrue(TEXT("Actor conversion must spawn an actor and inspect visible static mesh components"),
		Source.Contains(TEXT("SpawnActor<AActor>")) && Source.Contains(TEXT("GetComponents<UStaticMeshComponent>"))
		&& Source.Contains(TEXT("GetStaticMesh")) && Source.Contains(TEXT("IsVisible")) && Source.Contains(TEXT("bHiddenInGame")));
	TestTrue(TEXT("Actor conversion must copy materials and mark parent materials for instanced static meshes"),
		Source.Contains(TEXT("GetNumMaterials")) && Source.Contains(TEXT("GetMaterial")) && Source.Contains(TEXT("SetMaterial"))
		&& Source.Contains(TEXT("Cast<UMaterialInstanceDynamic>")) && Source.Contains(TEXT("MaterialInstance->Parent"))
		&& Source.Contains(TEXT("bUsedWithInstancedStaticMeshes = true")));
	TestTrue(TEXT("Actor conversion must preserve relative component transforms and add world-space instances"),
		Source.Contains(TEXT("GetRelativeTransform(SpawnedActor->GetActorTransform())"))
		&& Source.Contains(TEXT("It.RelativeTransform * Transform")) && Source.Contains(TEXT("AddInstance(CombinedTransform, bWorldSpace)")));
	TestTrue(TEXT("Reset and destruction must clear instances and cached blueprint data"),
		Source.Contains(TEXT("ClearInstances")) && Source.Contains(TEXT("ResetAllInstances")) && Source.Contains(TEXT("CachedBlueprintMeshes.Empty")));
	TestTrue(TEXT("Temporary actors used for conversion must be destroyed"),
		Source.Contains(TEXT("SpawnedActor->Destroy()")));

	return true;
}
