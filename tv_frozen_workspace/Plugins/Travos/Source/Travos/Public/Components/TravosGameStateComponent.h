// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Landscape.h"
#include "Components/ActorComponent.h"
#include "Engine/LevelStreaming.h"
#include "TravosGameStateComponent.generated.h"

class ARuntimeVirtualTextureVolume;

/**
 * @file TravosGameStateComponent.h
 * @brief Actor Component that manages the spawning and destroying Scene Capture components and Merging all the RVTs together for the Landscape Material.
 * Subsystem 
 */

USTRUCT()
struct FCaptureComponentInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY()
	TObjectPtr<ARuntimeVirtualTextureVolume> RVTVolume;
};

// DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelStateUpdated, ULevelStreaming*, Level, ELevelStreamingState, State);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRAVOS_API UTravosGameStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void OnLevelStreamingStateChanged(UWorld* World, const ULevelStreaming* LevelStreaming, ULevel* Level, ELevelStreamingState PreviousState, ELevelStreamingState NewState);
	// Sets default values for this component's properties
	UTravosGameStateComponent();

	// The delegate to which other classes can bind.
	// UPROPERTY(BlueprintAssignable)
	// FOnLevelStateUpdated OnLeveStateUpdated;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Mapped by World Partition cell coordinates or a unique identifier
	TMap<FString, FCaptureComponentInfo> ActiveCaptureComponents;
	
	void OnCellLoaded(const ULevelStreaming* LevelStreaming);
	void OnCellUnloaded(const ULevelStreaming* LevelStreaming);

	// Creates and configures a SceneCaptureComponent for a specific cell
	void CreateCaptureForCell(const ULevelStreaming* LevelStreaming);

	// Destroys the SceneCaptureComponent for an unloaded cell
	void DestroyCaptureForCell(const ULevelStreaming* LevelStreaming);

	// Combines all RVTs into one.
	void CombineRuntimeVirtualTextures();

public:
	// Required components and properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TObjectPtr<UTextureRenderTarget2D> MasterRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TObjectPtr<UMaterialInterface> LandscapeMaterial;

	UPROPERTY()
	ALandscape* WorldLandscape;

	UPROPERTY()
	TObjectPtr<UWorldPartitionSubsystem> WorldPartitionSubsystem;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
};
