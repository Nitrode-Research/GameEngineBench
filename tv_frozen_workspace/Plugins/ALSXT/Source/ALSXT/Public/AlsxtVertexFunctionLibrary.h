// MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Utility/AlsxtVertexEnums.h"
#include "AlsxtVertexFunctionLibrary.generated.h"

UCLASS()
class ALSXT_API UAlsxtVertexFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vertex Utilities")
	static int32 GetClosestVertexIDFromStaticMesh(UStaticMeshComponent* StaticMeshComponent, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vertex Utilities", meta = (DisplayName = "Get Most Prominent Vertex Color Channel"))
	static EProminentRGBAChannel GetProminentVertexColorChannel(UStaticMeshComponent* StaticMeshComponent, int32 VertexID);
};
