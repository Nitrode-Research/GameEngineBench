// Copyright (c) Yevhenii Selivanov

#include "GameFramework/BmrCheatManager.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrGeneratedMap.h"
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "Components/BmrCameraComponent.h"
#include "Components/BmrMapComponent.h"
#include "Controllers/BmrAIController.h"
#include "Controllers/BmrPlayerController.h"
#include "DataAssets/BmrGeneratedMapDataAsset.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "Engine/DebugCameraController.h"
#include "GameFramework/BmrPlayerState.h"
#include "MyUtilsLibraries/MultiplayerUtilsLibrary.h"
#include "Structures/BmrGameplayTags.h"
#include "Structures/BmrPlayerTag.h"
#include "Structures/BmrPowerupTag.h"
#include "Subsystems/BmrWidgetsSubsystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrActorUtilsLibrary.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrCheatManager)

UBmrCheatManager::UBmrCheatManager()
{
}

/*********************************************************************************************
 * Utils
 ********************************************************************************************* */

int32 UBmrCheatManager::GetBitmaskFromReverseString(const FString& ReverseBitmaskStr)
{
	return 0;
}

int32 UBmrCheatManager::GetBitmaskFromActorTypesString(const FString& ActorTypesBitmaskStr)
{
	return 0;
}

/*********************************************************************************************
 * Destroy
 ********************************************************************************************* */

void UBmrCheatManager::DestroyAllByType(EBmrActorType ActorType)
{
}

void UBmrCheatManager::DestroyPlayersBySlots(const FString& Slot)
{
}

/*********************************************************************************************
 * Box
 ********************************************************************************************* */

TAutoConsoleVariable<int32> UBmrCheatManager::CVarPowerupsChance(
    TEXT("Bomber.Box.SetPowerupsChance"),
    0,
    TEXT(""),
    ECVF_Cheat);

/*********************************************************************************************
 * Player
 ********************************************************************************************* */

void UBmrCheatManager::God()
{
}

void UBmrCheatManager::Suicide()
{
}

void UBmrCheatManager::SetPlayerPowerups(int32 NewLevel)
{
}

void UBmrCheatManager::SetAutoCopilot()
{
}

/*********************************************************************************************
 * AI
 ********************************************************************************************* */

TAutoConsoleVariable<bool> UBmrCheatManager::CVarAISetEnabled(
    TEXT("Bomber.AI.SetEnabled"),
    true,
    TEXT(""),
    ECVF_Cheat);

void UBmrCheatManager::SetAIPowerups(int32 NewLevel)
{
}

void UBmrCheatManager::ApplyPlayersSkinOnAI()
{
}

void UBmrCheatManager::AddBot()
{
}

/*********************************************************************************************
 * Debug
 ********************************************************************************************* */

TAutoConsoleVariable<FString> UBmrCheatManager::CVarDisplayCells(
    TEXT("Bomber.Debug.DisplayCells"),
    TEXT(""),
    TEXT(""),
    ECVF_Cheat);

/*********************************************************************************************
 * Level
 ********************************************************************************************* */

void UBmrCheatManager::SetLevelSize(const FString& LevelSize)
{
}

void UBmrCheatManager::SpawnActorByType(EBmrActorType ActorType, int32 ColumnX, int32 RowY, int32 SkinIndex)
{
}

void UBmrCheatManager::SetWallsChance(int32 WallsChance)
{
}

void UBmrCheatManager::SetBoxesChance(int32 BoxesChance)
{
}

/*********************************************************************************************
 * Camera
 ********************************************************************************************* */

void UBmrCheatManager::EnableDebugCamera()
{
	Super::EnableDebugCamera();
}

void UBmrCheatManager::DisableDebugCamera()
{
	Super::DisableDebugCamera();
}

void UBmrCheatManager::FitViewAdditiveAngle(float InFitViewAdditiveAngle)
{
}

void UBmrCheatManager::MinDistance(float InMinDistance)
{
}

/*********************************************************************************************
 * UI
 ********************************************************************************************* */

void UBmrCheatManager::SetUIHideAllWidgets()
{
}
