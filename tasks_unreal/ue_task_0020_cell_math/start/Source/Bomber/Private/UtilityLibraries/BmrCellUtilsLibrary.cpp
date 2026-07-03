// Copyright (c) Yevhenii Selivanov

#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// Bomber
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrGeneratedMap.h"
#include "Bomber.h"
#include "Components/BmrMapComponent.h"
#include "GameFramework/BmrCheatManager.h"
#include "UtilityLibraries/BmrActorUtilsLibrary.h"

// UE
#include "Components/TextRenderComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrCellUtilsLibrary)

const FBmrDisplayCellsParams FBmrDisplayCellsParams::EmptyParams = FBmrDisplayCellsParams();

void UBmrCellUtilsLibrary::BreakCell(const FBmrCell& InCell, double& X, double& Y, double& Z)
{
	X = InCell.X();
	Y = InCell.Y();
	Z = InCell.Z();
}

/*********************************************************************************************
 * Transform (Location, Rotation, Scale) on the level
 ********************************************************************************************* */

FTransform UBmrCellUtilsLibrary::GetLevelGridTransform()
{
	return FTransform::Identity;
}

FVector UBmrCellUtilsLibrary::GetLevelGridLocation()
{
	return FVector::ZeroVector;
}

float UBmrCellUtilsLibrary::GetCellHeightLocation()
{
	return 0.f;
}

FRotator UBmrCellUtilsLibrary::GetLevelGridRotation()
{
	return FRotator::ZeroRotator;
}

float UBmrCellUtilsLibrary::GetCellYawDegree()
{
	return 0.f;
}

FIntPoint UBmrCellUtilsLibrary::GetLevelGridScale()
{
	return FIntPoint::ZeroValue;
}

int32 UBmrCellUtilsLibrary::GetCellColumnsNumOnLevel()
{
	return 0;
}

int32 UBmrCellUtilsLibrary::GetCellRowsNumOnLevel()
{
	return 0;
}

int32 UBmrCellUtilsLibrary::GetLastColumnIndexOnLevel()
{
	return -1;
}

int32 UBmrCellUtilsLibrary::GetLastRowIndexOnLevel()
{
	return -1;
}

/*********************************************************************************************
 * Generated Map related cell functions
 ********************************************************************************************* */

void UBmrCellUtilsLibrary::GetPositionByCellOnLevel(const FBmrCell& InCell, int32& OutColumnX, int32& OutRowY)
{
	OutColumnX = -1;
	OutRowY = -1;
}

int32 UBmrCellUtilsLibrary::GetIndexByCellOnLevel(const FBmrCell& InCell)
{
	return INDEX_NONE;
}

FBmrCell UBmrCellUtilsLibrary::GetCellByIndexOnLevel(int32 CellIndex)
{
	return FBmrCell::InvalidCell;
}

const FBmrCellsArr& UBmrCellUtilsLibrary::GetAllCellsOnLevelAsArray()
{
	return FBmrCell::EmptyCellsArr;
}

FBmrCell UBmrCellUtilsLibrary::GetCenterCellOnLevel()
{
	return FBmrCell::InvalidCell;
}

void UBmrCellUtilsLibrary::GetCenterCellPositionOnLevel(int32& OutColumnX, int32& OutRowY)
{
	OutColumnX = -1;
	OutRowY = -1;
}

FIntPoint UBmrCellUtilsLibrary::GetCenterCellPositionOnLevel()
{
	return FIntPoint(-1, -1);
}

FBmrCells UBmrCellUtilsLibrary::GetAllEmptyCellsWithoutActors()
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::GetAllCellsWithActors(int32 ActorsTypesBitmask)
{
	return FBmrCell::EmptyCells;
}

void UBmrCellUtilsLibrary::IntersectCellsByTypes(FBmrCells& InOutCells, int32 ActorsTypesBitmask, bool bIntersectAllIfEmpty)
{
}

FBmrCells UBmrCellUtilsLibrary::FilterEmptyCellsWithoutActors(const FBmrCells& InCells)
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::FilterCellsByActors(const FBmrCells& InCells, int32 ActorsTypesBitmask)
{
	return FBmrCell::EmptyCells;
}

bool UBmrCellUtilsLibrary::IsEmptyCellWithoutActor(const FBmrCell& Cell)
{
	return false;
}

bool UBmrCellUtilsLibrary::IsCellHasAnyMatchingActor(const FBmrCell& Cell, int32 ActorsTypesBitmask)
{
	return false;
}

bool UBmrCellUtilsLibrary::IsAnyCellEmptyWithoutActor(const FBmrCells& Cells)
{
	return false;
}

bool UBmrCellUtilsLibrary::AreCellsHaveAnyMatchingActors(const FBmrCells& Cells, int32 ActorsTypesBitmask)
{
	return false;
}

bool UBmrCellUtilsLibrary::AreAllCellsEmptyWithoutActors(const FBmrCells& Cells)
{
	return false;
}

bool UBmrCellUtilsLibrary::AreCellsHaveAllMatchingActors(const FBmrCells& Cells, int32 ActorsTypesBitmask)
{
	return false;
}

void UBmrCellUtilsLibrary::GetSideCells(FBmrCells& OutCells, const FBmrCell& Cell, EPathType Pathfinder, int32 SideLength, int32 DirectionsBitmask, bool bBreakInputCells)
{
}

FBmrCells UBmrCellUtilsLibrary::GetCellsAround(const FBmrCell& CenterCell, EPathType Pathfinder, int32 Radius)
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::GetCellsAroundWithActors(const FBmrCell& CenterCell, EPathType Pathfinder, int32 Radius, int32 ActorsTypesBitmask)
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::GetEmptyCellsAroundWithoutActors(const FBmrCell& CenterCell, EPathType Pathfinder, int32 Radius)
{
	return FBmrCell::EmptyCells;
}

FBmrCell UBmrCellUtilsLibrary::GetCellInDirection(const FBmrCell& CenterCell, EPathType Pathfinder, EBmrCellDirection Direction)
{
	return FBmrCell::InvalidCell;
}

bool UBmrCellUtilsLibrary::CanGetCellInDirection(const FBmrCell& CenterCell, EPathType Pathfinder, EBmrCellDirection Direction)
{
	return false;
}

FBmrCells UBmrCellUtilsLibrary::GetCellsInDirections(const FBmrCell& CenterCell, EPathType Pathfinder, int32 SideLength, int32 DirectionsBitmask)
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::GetCellsInDirectionsWithActors(const FBmrCell& CenterCell, EPathType Pathfinder, int32 SideLength, int32 DirectionsBitmask, int32 ActorsTypesBitmask)
{
	return FBmrCell::EmptyCells;
}

FBmrCells UBmrCellUtilsLibrary::GetEmptyCellsInDirectionsWithoutActors(const FBmrCell& CenterCell, EPathType Pathfinder, int32 SideLength, int32 DirectionsBitmask)
{
	return FBmrCell::EmptyCells;
}

bool UBmrCellUtilsLibrary::IsIslandCell(const FBmrCell& Cell)
{
	return false;
}

FBmrCell UBmrCellUtilsLibrary::RotateCellAroundLevelOrigin(const FBmrCell& Cell, float AxisZ)
{
	return FBmrCell::InvalidCell;
}

FBmrCell UBmrCellUtilsLibrary::SnapActorOnLevel(const AActor* Actor)
{
	return FBmrCell::InvalidCell;
}

FBmrCell UBmrCellUtilsLibrary::GetNearestFreeCell(const FBmrCell& Cell)
{
	return FBmrCell::InvalidCell;
}

TSet<FBmrCell> UBmrCellUtilsLibrary::GetAllExplosionCells()
{
	return FBmrCell::EmptyCells;
}

bool UBmrCellUtilsLibrary::DoesPathExistToCellsOnLevel(const TSet<FBmrCell>& CellsToFind, const TSet<FBmrCell>& OptionalPathBreakers)
{
	return false;
}

FBmrCell UBmrCellUtilsLibrary::GetNearestCornerCellOnLevel(const FBmrCell& CellToCheck)
{
	return FBmrCell::InvalidCell;
}

/*********************************************************************************************
 * Debug cells utilities
 ********************************************************************************************* */

void UBmrCellUtilsLibrary::ClearDisplayedCells(const UObject* Owner /* = nullptr*/)
{
}

void UBmrCellUtilsLibrary::DisplayCells(UObject* Owner, const FBmrCells& Cells, const FBmrDisplayCellsParams& Params)
{
}

bool UBmrCellUtilsLibrary::CanDisplayCells(const UObject* Owner)
{
	return false;
}
