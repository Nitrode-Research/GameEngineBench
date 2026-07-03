// Copyleft: All rights reversed


#include "Customization/CustomizationMaterialNameSpace.h"

#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "Components/MeshComponent.h"
#include "Customization/CustomizationElementaryModule.h"
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values for this component's properties
UCustomizationMaterialNameSpace::UCustomizationMaterialNameSpace()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



FCustomizationMaterialNamespaceData UCustomizationMaterialNameSpace::GetMaterialCustomizationData(
	const FName NamespaceOverride) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UCustomizationMaterialNameSpace::OnChildAttached(USceneComponent* ChildComponent)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UCustomizationMaterialNameSpace::CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance,
                                                                       const FName ParameterName,
                                                                       const EMaterialParameterType ParameterType)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}



void UCustomizationMaterialNameSpace::ApplyMaterialChanges(USceneComponent* ChildComponent,
                                                           const TMap<FName, float>& GivenScalarParameters,
                                                           const TMap<FName, FLinearColor>& GivenVectorParameters,
                                                           const TMap<FName, UTexture*>& GivenTextureParameters,
                                                           const TArray<FName> SlotNames)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCustomizationMaterialNameSpace::SaveCMA()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

