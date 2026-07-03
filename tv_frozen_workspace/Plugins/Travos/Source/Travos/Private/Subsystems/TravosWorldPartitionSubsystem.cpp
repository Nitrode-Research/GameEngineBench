// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Subsystems/TravosWorldPartitionSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "Engine/LevelStreaming.h"
#include "UObject/SoftObjectPath.h"
#include "WorldPartition/WorldPartitionRuntimeCellData.h"

// FGuid UTravosWorldPartitionSubsystem::GetLevelStreamingGuid(const UWorld* World, FName LevelPackageName)
// {
// 	if (!World)
// 	{
// 		return FGuid();
// 	}
// 	
// 	// Get the array of streaming levels from the world
// 	TArray<TObjectPtr<ULevelStreaming>> StreamingLevels;
// 	StreamingLevels = World->GetStreamingLevels(); 
// 
// 	// Iterate through the streaming levels to find the one we need
// 	for (const TObjectPtr<ULevelStreaming>& StreamingLevel : StreamingLevels)
// 	{
// 		if (StreamingLevel && StreamingLevel->GetWorldAssetPackageFName() == LevelPackageName)
// 		{
// 			// The level package name matches, so we found our level
// 			// Check if the level is loaded before trying to get the handle
// 			if (StreamingLevel->IsLevelLoaded())
// 			{
// 				// The world asset's path is the most reliable identifier
// 				FSoftObjectPtr<UWorld> LevelPath;
// 				LevelPath = StreamingLevel->GetWorldAsset(); 
// 
// 				// Generate a GUID from the path string
// 				FGuid LevelGuid = FGuid::NewGuid();
// 				return LevelGuid;
// 			}
// 		}
// 	}
// 
// 	// Return an invalid GUID if the level is not found or not loaded
// 	return FGuid();
// }

FGuid UTravosWorldPartitionSubsystem::GetAssetGuid(const FAssetData& InAssetData)
{
	FGuid OutGuid;
    
	// The tag containing the asset's GUID is named "PackageGuid"
	static const FName PackageGuidTagName(TEXT("PackageGuid"));

	// Attempt to get the string value of the "PackageGuid" tag
	FString PackageGuidString;
	if (InAssetData.GetTagValue(PackageGuidTagName, PackageGuidString))
	{
		// Convert the string to an FGuid
		if (FGuid::Parse(PackageGuidString, OutGuid))
		{
			// Now you have the valid GUID
			UE_LOG(LogTemp, Log, TEXT("Found GUID: %s"), *OutGuid.ToString());
			return OutGuid;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to parse GUID from string."));
			return OutGuid;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Tag 'PackageGuid' not found in FAssetData."));
		return OutGuid;
	}
}

 void UTravosWorldPartitionSubsystem::OnUpdateStreamingState()
 {
 	// Call the parent implementation first to ensure default logic is executed.
 	Super::OnUpdateStreamingState();
 
 	// Get the World Partition.
 	UWorld* World = GetWorld();
 	if (!World)
 	{
 		return;
 	}
 
 	UWorldPartition* WorldPartition = World->GetWorldPartition();
 	if (!WorldPartition)
 	{
 		return;
 	}
 
 	// Example of getting cell information. You will need to query for the specific cell
 	// or iterate through relevant cells to find their state. The exact method will depend
 	// on your specific logic for which cells to check.
 	TArray<UWorldPartitionRuntimeCell> AllCells;
 	WorldPartition->GetWorldAssetStreamingCells(World->PersistentLevel->GetPackage()->GetPersistentGuid());
 
 	// Iterate through all the streaming levels in the world
 	for (ULevelStreaming* LevelStreaming : World->GetStreamingLevels())
 	{
 		// Get the package name (path) of the level asset
 		FName PackageName = LevelStreaming->GetWorldAssetPackageFName();
 		
 
 		ELevelStreamingState LevelStreamingState = LevelStreaming->GetLevelStreamingState();
 
 		if (PackageName != NAME_None)
 		{
 			// Access the Asset Registry module
 			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
 			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
 
 			// Find the asset data for the level using its package name
 			const FAssetData& AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(LevelStreaming->GetWorldAssetPackageName()));
 
 			if (AssetData.IsValid())
 			{
 				FGuid LevelGuid = FGuid::NewGuid();
 				// We found the correct ULevelStreaming object
 				LevelGuid = GetAssetGuid(AssetData);
 
 				// TravosGameStateComponent->
 				
 				// OnUpdate.Broadcast(Cell, CellState);
 				// OnCellStateUpdatedNew.Broadcast(LevelStreaming, LevelStreamingState);
 
 				if (LevelStreamingState == ELevelStreamingState::LoadedVisible || LevelStreamingState == ELevelStreamingState::MakingVisible)
 				{
 					
 				}
 				else
 				{
 					
 				}
 			}
 		}
 
 
 	}
 
 	// for (const FWorldPartitionRuntimeCell& Cell : AllCells)
 	// {
 	// 	// EWorldPartitionRuntimeCellState CellState = Cell.GetStreamingState();
 	// 	// You can add logic here to only broadcast if the state has changed.
 	// 	OnCellStateUpdated.Broadcast(Cell, CellState);
 	// }
 }
