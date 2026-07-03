// Copyleft: All rights reversed


#include "Customization/CustomizationLoaderComponent.h"

#include "Customization/CustomizationElementaryAsset.h"
#include "Customization/CustomizationLoaderAsset.h"
#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationMaterialNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "MeshMergeFunctionLibrary.h"
#include "Customization/CustomizationAttachmentAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"


UCustomizationLoaderComponent::UCustomizationLoaderComponent()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationLoaderComponent::BeginPlay()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



template <class SceneComponentClass>
SceneComponentClass* UCustomizationLoaderComponent::SpawnChildComponent(USkeletalMeshComponent* Component,
                                                                        const FString Name, const FName SocketName,
                                                                        const FTransform RelativeTransform)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}



void UCustomizationLoaderComponent::LoadFromAsset(TArray<UCustomizationElementaryAsset*> NewElementaryAssets,
                                                  TArray<UCustomizationMaterialAsset*> NewMaterialConfigs,
                                                  const TMap<UCustomizationElementaryAsset*,
                                                             FCustomizationMaterialAssetMap>&
                                                  NewMaterialConfigsOverrides,
                                                  const TMap<UCustomizationElementaryAsset*,
                                                             FCustomizationAttachmentAssetArray>&
                                                  NewExternalAttachments)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationLoaderComponent::UnloadPreviousCustomization()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



template <class ComponentClass>
FName UCustomizationLoaderComponent::GetExistingSocketNameOrNameNone(const ComponentClass* Component,
                                                                     FName SocketName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCustomizationLoaderComponent::ProcessSkeletalAttachesForComponent(USkeletalMeshComponent* Component,
                                                                        const TArray<
	                                                                        FCustomizationElementarySubmoduleSkeletal>&
                                                                        MeshesForAttach,
                                                                        const FString NameEnding,
                                                                        TMap<FName, UCustomizationMaterialAsset*>&
                                                                        MaterialNamespacesToData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationLoaderComponent::ProcessStaticAttachesForComponent(USkeletalMeshComponent* Component,
                                                                      const TArray<
	                                                                      FCustomizationElementarySubmoduleStatic>&
                                                                      MeshesForAttach,
                                                                      const FString NameEnding,
                                                                      TMap<FName, UCustomizationMaterialAsset*>&
                                                                      MaterialNamespacesToData)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationLoaderComponent::ProcessParticleAttachesForComponent(USkeletalMeshComponent* Component,
                                                                        const TArray<
	                                                                        FCustomizationElementarySubmoduleParticle>&
                                                                        ParticlesForAttach, const FString NameEnding)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UCustomizationLoaderComponent::ProcessMeshMergeModule(const FName Namespace,
                                                           TArray<UCustomizationElementaryAsset*>&
                                                           NamespaceAssets,
                                                           USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                           TMap<FName, UCustomizationMaterialAsset*>&
                                                           MaterialNamespacesToData,
                                                           const TMap<UCustomizationElementaryAsset*,
                                                                      FCustomizationAttachmentAssetArray>&
                                                           NewExternalAttachments)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UCustomizationLoaderComponent::ProcessAttachmentModule(FName SocketName,
                                                            TArray<UCustomizationElementaryAsset*>& SocketNameAssets,
                                                            USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                            TMap<FName, UCustomizationMaterialAsset*>&
                                                            MaterialNamespacesToData,
                                                            const TMap<UCustomizationElementaryAsset*,
                                                                       FCustomizationMaterialAssetMap>&
                                                            NewMaterialConfigsOverrides,
                                                            const TMap<UCustomizationElementaryAsset*,
                                                                       FCustomizationAttachmentAssetArray>&
                                                            NewExternalAttachments)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

