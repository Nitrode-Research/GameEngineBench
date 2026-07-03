// MIT


#include "AlsxtVertexFunctionLibrary.h"
#include "CoreMinimal.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h" // For collision queries if needed
#include "Math/Vector.h"
#include "Rendering/StaticMeshVertexBuffer.h"

#include "RawMesh.h" // For accessing FRawMeshVertexData

int32 UAlsxtVertexFunctionLibrary::GetClosestVertexIDFromStaticMesh(UStaticMeshComponent* StaticMeshComponent, const FVector& WorldLocation)
{
	if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
	{
		return INDEX_NONE; // Invalid component or mesh
	}

	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return INDEX_NONE; // No render data or LODs
	}

	// Get the LOD 0 resources (or chosen LOD)
	const FStaticMeshLODResources& LODResources = StaticMesh->GetRenderData()->LODResources[0];

	// Get the vertex buffer
	// const FStaticMeshVertexBuffer& VertexBuffer = LODResources.VertexBuffers.PositionVertexBuffer;
	// const FStaticMeshVertexBuffer& VertexBuffer = LODResources.VertexBuffers.StaticMeshVertexBuffer;
	const FPositionVertexBuffer& VertexBuffer = LODResources.VertexBuffers.PositionVertexBuffer;

	if (VertexBuffer.GetNumVertices() == 0)
	{
		return INDEX_NONE; // No vertices
	}

	const FTransform ComponentTransform = StaticMeshComponent->GetComponentTransform();
	int32 ClosestVertexId = INDEX_NONE;
	float MinSquaredDistance = FLT_MAX;

	for (uint32 i = 0; i < VertexBuffer.GetNumVertices(); ++i)
	{
		// Get vertex position in mesh local space
		const FVector3f LocalVertexPosition = VertexBuffer.VertexPosition(i);
		
		UE::Math::TVector<double> ConvertedLocalVertexPosition(
		static_cast<double>(LocalVertexPosition.X),
		static_cast<double>(LocalVertexPosition.Y),
		static_cast<double>(LocalVertexPosition.Z)
		);

		// Transform to world space
		const UE::Math::TVector<double> WorldVertexPosition = ComponentTransform.TransformPosition(ConvertedLocalVertexPosition);
		FVector MyFloatVector = FVector(WorldVertexPosition); 
		//TransformPosition(LocalVertexPosition);

		// Calculate squared distance to target location
		const float CurrentSquaredDistance = FVector::DistSquared(MyFloatVector, WorldLocation);

		if (CurrentSquaredDistance < MinSquaredDistance)
		{
			MinSquaredDistance = CurrentSquaredDistance;
			ClosestVertexId = i;
		}
	}

	return ClosestVertexId;
}

EProminentRGBAChannel UAlsxtVertexFunctionLibrary::GetProminentVertexColorChannel(UStaticMeshComponent* StaticMeshComponent, int32 VertexID)
{
    if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh() || VertexID < 0)
    {
        // Handle invalid inputs
        return EProminentRGBAChannel::None;
    }

    const FStaticMeshRenderData* RenderData = StaticMeshComponent->GetStaticMesh()->GetRenderData();
    if (!RenderData || RenderData->LODResources.Num() == 0)
    {
        return EProminentRGBAChannel::None;
    }

    // Access the LOD (Level of Detail) you want to sample from
    // (LOD 0 is typically the highest detail)
    const FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
		
    // Access the vertex color buffer if available
    if (!LODResources.VertexBuffers.ColorVertexBuffer.IsInitialized())
    {
        return EProminentRGBAChannel::None; // No vertex color data found
    }

	unsigned int VID = VertexID;
	
    // Get the vertex color at the specified VertexID
    if (VID >= LODResources.VertexBuffers.ColorVertexBuffer.GetNumVertices())
    {
        return EProminentRGBAChannel::None; // VertexID out of bounds
    }
    const FColor VertexColor = LODResources.VertexBuffers.ColorVertexBuffer.VertexColor(VertexID);

    // Convert FColor (0-255) to FLinearColor (0.0-1.0) for easier comparison
    const FLinearColor LinearColor = FLinearColor::FromSRGBColor(VertexColor);

    // Determine the most prominent channel
    EProminentRGBAChannel ProminentChannel = EProminentRGBAChannel::None;
    float MaxValue = 0.0f;

    if (LinearColor.R > MaxValue)
    {
        MaxValue = LinearColor.R;
        ProminentChannel = EProminentRGBAChannel::Red;
    }
    if (LinearColor.G > MaxValue)
    {
        MaxValue = LinearColor.G;
        ProminentChannel = EProminentRGBAChannel::Green;
    }
    if (LinearColor.B > MaxValue)
    {
        MaxValue = LinearColor.B;
        ProminentChannel = EProminentRGBAChannel::Blue;
    }
    // Consider Alpha as well if relevant
    if (LinearColor.A > MaxValue)
    {
        MaxValue = LinearColor.A;
        ProminentChannel = EProminentRGBAChannel::Alpha;
    }
    else
    {
    	// Check if the color is "empty" (all channels are approximately zero)
	    if (FMath::IsNearlyZero(LinearColor.R) && 
	        FMath::IsNearlyZero(LinearColor.G) &&
	        FMath::IsNearlyZero(LinearColor.B) &&
	        FMath::IsNearlyZero(LinearColor.A))
	    {
	        // Return an None if color is empty
    		ProminentChannel = EProminentRGBAChannel::None;
    		return ProminentChannel;
	    }

    	// Check if the color is "White" (all channels at LinearColor.GetMax())
    	if ((LinearColor.R == LinearColor.GetMax()) && 
			(LinearColor.G == LinearColor.GetMax()) &&
			(LinearColor.B == LinearColor.GetMax()) &&
			(LinearColor.A == LinearColor.GetMax()))
    	{
    		// Return an None if color is empty
    		ProminentChannel = EProminentRGBAChannel::White;
    		return ProminentChannel;
    	}

	    MaxValue = 0.0f;
	    TArray<EProminentRGBAChannel> ProminentChannels;

	    // Find the maximum value
	    MaxValue = FMath::Max(LinearColor.R, FMath::Max(LinearColor.G, FMath::Max(LinearColor.B, LinearColor.A)));

	    // Identify channels with the maximum value
	    if (FMath::IsNearlyEqual(LinearColor.R, MaxValue))
	    {
	        ProminentChannels.Add(EProminentRGBAChannel::Red);
	    }
	    if (FMath::IsNearlyEqual(LinearColor.G, MaxValue))
	    {
	        ProminentChannels.Add(EProminentRGBAChannel::Green);
	    }
	    if (FMath::IsNearlyEqual(LinearColor.B, MaxValue))
	    {
	        ProminentChannels.Add(EProminentRGBAChannel::Blue);
	    }
	    if (FMath::IsNearlyEqual(LinearColor.A, MaxValue))
	    {
	        ProminentChannels.Add(EProminentRGBAChannel::Alpha);
	    }

	    // Randomly select one from the prominent channels
	    if (ProminentChannels.Num() > 0)
	    {
	        int32 RandomIndex = FMath::RandRange(0, ProminentChannels.Num() - 1);
    		ProminentChannel = ProminentChannels[RandomIndex];
	        return ProminentChannel;
	    }
	    else
	    {
	    	// Return an None if color is empty
	    	ProminentChannel = EProminentRGBAChannel::None;
	    	return ProminentChannel;
	    }
    }

    return ProminentChannel;
}