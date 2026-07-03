// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Actors/ClimaMicroclimateActor.h"
#include "Data/ClimaMicroClimateStructs.h"

AClimaMicroClimateActor::AClimaMicroClimateActor()
{
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
}

void inline AClimaMicroClimateActor::UpdateMeshFromData(const FCompressedMicroclimateData& Data)
{
	// 1. Decompress the vertex and index data
	// TArray<FVector> Vertices = DecompressData(Data.MicroclimateShapeData.CompressedVertexData);
	// TArray<int32> Indices = DecompressData(Data.MicroclimateShapeData.CompressedIndexData);
    
	// 2. Generate the mesh section
	// MicroclimateMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, TArray<FVector>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AClimaMicroClimateActor::BeginPlay()
{
    Super::BeginPlay();
    LifeSpan = 0.0f;
    GenerateAndMutateMesh();
    ApplyDynamicMaterial();
    CurrentVelocity = FVector(FMath::RandRange(-100.f, 100.f), FMath::RandRange(-100.f, 100.f), 0.f);
}

void AClimaMicroClimateActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateMovement(DeltaTime);
    UpdateLifecycle(DeltaTime);
    // Mutate shape occasionally (e.g., every few seconds) or continuously as needed
    if (FMath::RandRange(0.f, 100.f) < 0.5f) // Small chance per frame to mutate
    {
        GenerateAndMutateMesh();
    }
}

void AClimaMicroClimateActor::GenerateAndMutateMesh()
{
    Vertices.Empty();
    Triangles.Empty();
    UV0.Empty();
    VertexColors.Empty();

    // Generate a random number of polygon points (e.g., 5 to 10 sides)
    int32 NumSides = FMath::RandRange(5, 10);
    // Add central vertex
    Vertices.Add(FVector(0.f, 0.f, 0.f)); 
    UV0.Add(FVector2D(0.5f, 0.5f)); // Center UV

    for (int32 i = 0; i < NumSides; ++i)
    {
        float Angle = (float)i / NumSides * 360.f;
        float AngleRad = FMath::DegreesToRadians(Angle);
        // Vary radius slightly to 'mutate' shape
        float Radius = CurrentRadius * FMath::RandRange(0.8f, 1.2f); 

        float X = FMath::Cos(AngleRad) * Radius;
        float Y = FMath::Sin(AngleRad) * Radius;

        Vertices.Add(FVector(X, Y, 0.f));
        // Calculate UVs based on normalized position
        UV0.Add(FVector2D(X / MaxRadius * 0.5f + 0.5f, Y / MaxRadius * 0.5f + 0.5f));

        // Create triangles (connect center to the edge vertices)
        if (i < NumSides - 1)
        {
            Triangles.Add(0); // Center point
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }
        else // Last triangle connects back to the first edge point
        {
            Triangles.Add(0); // Center point
            Triangles.Add(i + 1);
            Triangles.Add(1);
        }
    }
    
    // Create the mesh section
    // We pass empty arrays for Normals, Tangents, VertexColors as we will use material for visual effects
    ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
    // Enable collision
    // ProceduralMesh->bUseAsyncCollision = true; 
}

void AClimaMicroClimateActor::ApplyDynamicMaterial()
{
    if (BaseMaterial)
    {
        DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicMaterialInstance)
        {
            ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
            // Set initial alpha falloff (e.g., 1.0 for fully opaque center)
            DynamicMaterialInstance->SetScalarParameterValue(AlphaFalloffParamName, 1.0f);
        }
    }
}

void AClimaMicroClimateActor::UpdateMovement(float DeltaTime)
{
    // Move on XY axis
    AddActorWorldOffset(CurrentVelocity * DeltaTime, true);
}

void AClimaMicroClimateActor::UpdateLifecycle(float DeltaTime)
{
    LifeSpan += DeltaTime;

    // Grow/Shrink logic
    // Example: Grow for half life, then shrink
    if (LifeSpan < MaxLifeSpan / 2.0f)
    {
        CurrentRadius += GrowthRate * DeltaTime;
        CurrentRadius = FMath::Min(CurrentRadius, MaxRadius);
    }
    else
    {
        CurrentRadius -= GrowthRate * DeltaTime * 2.0f; // Shrink faster
        if (CurrentRadius <= 0.0f)
        {
            Destroy(); // Die when size is zero
            return;
        }
    }
    
    // Update material alpha based on lifespan
    if (DynamicMaterialInstance)
    {
        // Fade out as lifespan progresses, more aggressively near the end
        float Alpha = 1.0f - (LifeSpan / MaxLifeSpan);
        DynamicMaterialInstance->SetScalarParameterValue(AlphaFalloffParamName, Alpha);
    }

    // Re-generate mesh if size changed significantly to update vertices
    // Note: this is expensive, an alternative is WPO in material, but this meets the 'mutate' criteria with size
    GenerateAndMutateMesh();
}
