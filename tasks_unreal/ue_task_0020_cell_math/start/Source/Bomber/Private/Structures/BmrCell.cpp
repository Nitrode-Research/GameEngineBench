// Copyright (c) Yevhenii Selivanov.

#include "Structures/BmrCell.h"

// Bomber
#include "Engine/NetSerialization.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrCell)

const FBmrCell FBmrCell::InvalidCell = FBmrCell(0.f, 0.f, -1.f);
const FBmrCell FBmrCell::ForwardCell = FBmrCell(1.f, 0.f, 0.f);
const FBmrCell FBmrCell::BackwardCell = FBmrCell(-1.f, 0.f, 0.f);
const FBmrCell FBmrCell::RightCell = FBmrCell(0.f, 1.f, 0.f);
const FBmrCell FBmrCell::LeftCell = FBmrCell(0.f, -1.f, 0.f);
const FBmrCells FBmrCell::EmptyCells = FBmrCells{};
const FBmrCellsArr FBmrCell::EmptyCellsArr = FBmrCellsArr{};

FBmrCell::FBmrCell(const FVector& Vector)
{
	Location = Vector;
}

FBmrCell::FBmrCell(const FVector_NetQuantize& Vector)
    : FBmrCell(FVector(Vector))
{
}

FBmrCell::FBmrCell(float X, float Y, float Z)
{
	Location = FVector(X, Y, Z);
}

FBmrCell::FBmrCell(double X, double Y, double Z)
{
	Location = FVector(X, Y, Z);
}

FBmrCell& FBmrCell::operator=(const FVector& Vector)
{
	Location = Vector;
	return *this;
}

FBmrCell& FBmrCell::operator=(const FVector_NetQuantize& Vector)
{
	Location = FVector(Vector);
	return *this;
}

/*********************************************************************************************
 * Rotation
 ********************************************************************************************* */

FBmrCell FBmrCell::RotateCellAroundOrigin(const FBmrCell& InCell, float AxisZ, const FTransform& OriginTransformNoScale)
{
	return InvalidCell;
}

FBmrCells FBmrCell::RotateCellArray(float AxisZ, const FBmrCells& InCells)
{
	return EmptyCells;
}

/*********************************************************************************************
 * Grid
 ********************************************************************************************* */

FBmrCells FBmrCell::MakeCellGridByTransform(const FTransform& OriginTransform)
{
	return EmptyCells;
}

FBmrCell FBmrCell::GetCellByPositionOnGrid(const FIntPoint& CellPosition, const FBmrCells& InGrid)
{
	return InvalidCell;
}

FBmrCell FBmrCell::GetCellByPositionOnGrid(const FIntPoint& CellPosition, const FBmrCellsArr& InGrid, int32 GridWidth)
{
	return InvalidCell;
}

FIntPoint FBmrCell::GetPositionByCellOnGrid(const FBmrCell& InCell, const FBmrCells& InGrid)
{
	return FIntPoint(-1, -1);
}

FIntPoint FBmrCell::GetPositionByCellOnGrid(const FBmrCell& InCell, const FBmrCellsArr& InGrid, int32 GridWidth)
{
	return FIntPoint(-1, -1);
}

TMap<FBmrCell, FIntPoint> FBmrCell::GetPositionsByCellsOnGrid(const FBmrCellsArr& InGrid, int32 GridWidth)
{
	return TMap<FBmrCell, FIntPoint>{};
}

FIntPoint FBmrCell::GetCenterCellPositionOnGrid(const FBmrCells& InGrid)
{
	return FIntPoint(-1, -1);
}

FBmrCells FBmrCell::GetCornerCellsOnGrid(const FBmrCells& InGrid)
{
	return EmptyCells;
}

FBmrCell FBmrCell::GetCellByCornerOnGrid(EBmrGridCorner CornerType, const FBmrCells& InGrid)
{
	return InvalidCell;
}

FBmrCell FBmrCell::ScaleCellToNewGrid(const FBmrCell& OriginalCell, const FBmrCells& NewCornerCells)
{
	return InvalidCell;
}

FTransform FBmrCell::GetCellArrayTransform(const FBmrCells& InCells)
{
	return FTransform::Identity;
}

FTransform FBmrCell::GetCellArrayTransformNoScale(const FBmrCells& InCells)
{
	return FTransform::Identity;
}

FBmrCell FBmrCell::GetCellArrayCenter(const FBmrCells& Cells)
{
	return InvalidCell;
}

FRotator FBmrCell::GetCellArrayRotation(const FBmrCells& InCells)
{
	return FRotator::ZeroRotator;
}

FVector2D FBmrCell::GetCellArraySize(const FBmrCells& InCells)
{
	return FVector2D::ZeroVector;
}

float FBmrCell::GetCellArrayWidth(const FBmrCells& InCells)
{
	return 0.f;
}

float FBmrCell::GetCellArrayLength(const FBmrCells& InCells)
{
	return 0.f;
}

FBmrCell FBmrCell::GetCellArrayNearest(const FBmrCells& InCells, const FBmrCell& CellToCheck)
{
	return InvalidCell;
}

FBmrCells FBmrCell::FilterCellsByBounds(const FBmrCells& ActiveCells, const FBmrCells& BoundaryCells, const FBmrCell& StartingCell)
{
	return EmptyCells;
}

bool FBmrCell::CanCellSeeTarget(const FBmrCell& StartingCell, const FBmrCell& TargetCell, const FBmrCells& AllVisibleCells, float MaxAngleDegrees /* = 40.f*/)
{
	return false;
}

void FBmrCell::RandShuffle(FBmrCellsArr& InOutArray)
{
}

FBmrCellsArr FBmrCell::GetTopLeftQuarterOnGrid(const FBmrCellsArr& InGrid, const FIntPoint& MapScale)
{
	return EmptyCellsArr;
}

/*********************************************************************************************
 * Math operators
 ********************************************************************************************* */

FBmrCell& FBmrCell::operator+=(const FBmrCell& Other)
{
	Location += Other.Location;
	return *this;
}

FBmrCell& FBmrCell::operator-=(const FBmrCell& Other)
{
	Location -= Other.Location;
	return *this;
}

/*********************************************************************************************
 * Conversion
 ********************************************************************************************* */

FBmrCell::operator FVector_NetQuantize() const
{
	return FVector_NetQuantize(Location);
}

const FBmrCell& FBmrCell::GetCellDirection(EBmrCellDirection CellDirection)
{
	return InvalidCell;
}

EBmrCellDirection FBmrCell::GetCellDirection(const FBmrCell& CellDirection)
{
	return EBmrCellDirection::None;
}

TArray<FVector> FBmrCell::CellsToVectors(const FBmrCells& Cells)
{
	return TArray<FVector>{};
}

FBmrCells FBmrCell::VectorsToCells(const TArray<FVector>& Vectors)
{
	return EmptyCells;
}
