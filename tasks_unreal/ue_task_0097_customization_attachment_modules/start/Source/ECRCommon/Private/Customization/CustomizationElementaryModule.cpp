// Copyleft: All rights reversed


#include "Customization/CustomizationElementaryModule.h"

#include "Customization/CustomizationMaterialNameSpace.h"
#include "Customization/CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "Customization/CustomizationElementaryAsset.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/SavePackage.h"


UCustomizationElementaryModule::UCustomizationElementaryModule()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UCustomizationElementaryModule::OnRegister()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationElementaryModule::OnChildAttached(USceneComponent* ChildComponent)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



FCustomizationMaterialNamespaceData UCustomizationElementaryModule::GetMaterialCustomizationData(
	const FName MaterialNamespace) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UCustomizationElementaryModule::InheritAnimationsIfNeeded()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



FString UCustomizationElementaryModule::GetFirstMaterialNameSpaceRaw(const USceneComponent* GivenComponent) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



UCustomizationElementaryAsset* UCustomizationElementaryModule::SaveToDataAsset(bool bDoOverwrite) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCustomizationAttachmentAsset* UCustomizationElementaryModule::SaveAttachmentsToDataAsset() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UCustomizationElementaryModule::SaveToDataAsset() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationElementaryModule::SaveToAttachmentAsset() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

