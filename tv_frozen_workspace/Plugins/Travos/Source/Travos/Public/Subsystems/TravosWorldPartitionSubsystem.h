// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Components/TravosGameStateComponent.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionRuntimeCellData.h"
#include "UObject/Object.h"
#include "Engine/LevelStreaming.h"
#include "TravosWorldPartitionSubsystem.generated.h"

UCLASS()
class TRAVOS_API UTravosWorldPartitionSubsystem : public UWorldPartitionSubsystem
{
	GENERATED_BODY()

public:
	// Override the core streaming state update function.
	// virtual void UpdateStreamingState(const FVector& InLocation);

	virtual void OnUpdateStreamingState() override;

	// Delegates for World Partition events
	void OnCellLoaded(ULevelStreaming* InWorldPartition);
	void OnCellUnloaded(ULevelStreaming* InWorldPartition);

	static FGuid GetAssetGuid(const FAssetData& InAssetData);

private:
	UTravosGameStateComponent* TravosGameStateComponent;
};
