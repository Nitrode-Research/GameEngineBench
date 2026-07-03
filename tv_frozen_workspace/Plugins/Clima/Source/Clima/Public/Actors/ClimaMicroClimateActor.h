// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ClimaMicroClimateActor.generated.h"

struct FCompressedMicroclimateData;

UCLASS()
class AClimaMicroClimateActor : public AActor
{
	GENERATED_BODY()

public:
	AClimaMicroClimateActor();

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* MicroclimateMesh = nullptr;

	// Called by the GameState's OnRep function
	void UpdateMeshFromData(const FCompressedMicroclimateData& Data);

	///
	///
	///

protected:
	virtual void BeginPlay() override;

public:    
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* BaseMaterial;

private:
	void GenerateAndMutateMesh();
	void UpdateMesh();
	void ApplyDynamicMaterial();
	void UpdateMovement(float DeltaTime);
	void UpdateLifecycle(float DeltaTime);

	// Mesh Data
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Dynamics
	FVector CurrentVelocity;
	float LifeSpan;
	float MaxLifeSpan = 20.0f; // Actor dies after 20 seconds
	float GrowthRate = 50.0f; // units per second
	float MaxRadius = 500.0f;
	float CurrentRadius = 100.0f;

	// Material
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	// Falloff/Alpha parameter name
	FName AlphaFalloffParamName = TEXT("AlphaFalloff"); 
};


