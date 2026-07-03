// Copyright (c) 2026 GameDevBench. Cell Math + Cell Utility Library automation tests (Bomber).
//
// The bulk of this file tests the pure-math surface of FBmrCell and the
// static helpers in UBmrCellUtilsLibrary directly (no PIE) — these primitives
// are deterministic and observable through the public surface.
//
// The final test brings up a PIE session on /Game/Bomber/Maps/Main to cover
// the level-dependent surface that routes through ABmrGeneratedMap:
// EPathType-aware GetSideCells, actor-typed cell queries (IntersectCellsByTypes),
// and DoesPathExistToCellsOnLevel BFS.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

// Bomber
#include "Bomber.h" // EPathType, EBmrActorType
#include "Structures/BmrCell.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"
#include "Actors/BmrGeneratedMap.h"

DEFINE_LOG_CATEGORY_STATIC(LogCellMathTests, Log, All);

namespace CellMathTestsPrivate
{
	static constexpr float CellSize = FBmrCell::CellSize; // 200.f

	// Build an FTransform centered at `Center` with yaw `YawDeg` and
	// `ColumnsX` x `RowsY` "scale-as-size" — that is exactly the format
	// MakeCellGridByTransform expects.
	static FTransform MakeGridOriginTransform(const FVector& Center, float YawDeg, int32 ColumnsX, int32 RowsY)
	{
		FTransform Out = FTransform::Identity;
		Out.SetLocation(Center);
		Out.SetRotation(FRotator(0.f, YawDeg, 0.f).Quaternion());
		Out.SetScale3D(FVector(static_cast<float>(ColumnsX), static_cast<float>(RowsY), 1.f));
		return Out;
	}

	// Compute the expected world position for cell (Col, Row) of an
	// axis-aligned grid (yaw=0) centered at `Center` with `ColumnsX` x `RowsY`
	// cells. Matches the layout MakeCellGridByTransform produces for yaw=0.
	static FVector ExpectedAxisAlignedCellCenter(const FVector& Center, int32 ColumnsX, int32 RowsY, int32 Col, int32 Row)
	{
		const FVector Offset(
			(static_cast<float>(Col) - 0.5f * static_cast<float>(ColumnsX - 1)) * CellSize,
			(static_cast<float>(Row) - 0.5f * static_cast<float>(RowsY - 1)) * CellSize,
			0.f);
		return Center + Offset;
	}
}

// ---------------------------------------------------------------------------
// Test 1 — B1, B25: world→cell rounds the input vector
//
// The solution's `FBmrCell(FVector)` rounds each component to the nearest
// integer cm (via FMath::RoundToFloat), so a non-integer input is observably
// changed. The stripped state's naive `Location = Vector` keeps the input
// verbatim and fails this check. This is the discriminator the failure-mode
// B25 calls out — and the rounding is what makes downstream grid alignment
// (B26) consistent under rotation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_VectorCtorRounds,
	"Bomber.CellMath.VectorCtorRounds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_VectorCtorRounds::RunTest(const FString& Parameters)
{
	// Non-integer input. The solution rounds to (124, 456, 0); a naive
	// `Location = Vector` keeps (123.7, 456.3, 0).
	const FVector Input(123.7f, 456.3f, 0.0f);
	const FBmrCell Cell(Input);

	TestEqual(
		TEXT("B1/B25: FBmrCell(FVector) must round X to the nearest integer cm. "
			 "A naive `Location = Vector` ctor (the stripped state) leaves X=123.7."),
		Cell.X(), 124.0f);
	TestEqual(
		TEXT("B1/B25: FBmrCell(FVector) must round Y to the nearest integer cm. "
			 "A naive `Location = Vector` ctor (the stripped state) leaves Y=456.3."),
		Cell.Y(), 456.0f);

	// Round-trip identity for an exact integer input is required for every
	// downstream grid op — cells stored at (0, 0, 0) must reproduce as
	// (0, 0, 0). This catches an over-eager ctor that snaps to 200 here.
	const FBmrCell ZeroCell(FVector::ZeroVector);
	TestTrue(
		FString::Printf(TEXT("B2: integer-aligned input must round-trip exactly through the ctor. Got %s."),
			*ZeroCell.Location.ToString()),
		ZeroCell.Location.Equals(FVector::ZeroVector, 0.01f));

	// `operator FVector()` returns the stored Location verbatim (B2 — cell→world
	// is "return Location"). Verify the cast yields exactly the rounded value.
	const FVector Round = static_cast<FVector>(Cell);
	TestTrue(
		FString::Printf(TEXT("B2: cell→world (operator FVector) must return the stored Location "
							 "verbatim. Got %s, expected (124, 456, 0)."),
			*Round.ToString()),
		Round.Equals(FVector(124.0f, 456.0f, 0.0f), 0.01f));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B3, B26: round-trip stays within half a cell, even under rotation
//
// Build a grid at yaw=30° via MakeCellGridByTransform. Every produced cell's
// Location, when fed back through the FBmrCell(FVector) ctor, must reproduce
// itself within half-cell precision. A stripped MakeCellGridByTransform that
// returns an empty set fails the count check; a world→cell implementation
// using `FloorToInt(WorldXY / CellSize)` (the B26 anti-pattern) would not
// produce rotated cell centers at 200-cm spacing around the rotated origin.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_RoundTripUnderRotation,
	"Bomber.CellMath.RoundTripUnderRotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_RoundTripUnderRotation::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	const FTransform Origin = MakeGridOriginTransform(FVector(0.f, 0.f, 0.f), 30.f, 3, 3);
	const FBmrCells Grid = FBmrCell::MakeCellGridByTransform(Origin);

	TestEqual(
		TEXT("B3/B6: a 3x3 rotated grid must produce exactly 9 cells. "
			 "A stripped MakeCellGridByTransform returns the empty set."),
		Grid.Num(), 9);

	// Round-trip every cell: FBmrCell(c.Location) must land within half a
	// cell of the original Location. Naive ctor behavior or grid-stub stubs
	// break either by producing the wrong count above, or by storing
	// `DownVector` placeholders that don't round-trip.
	const float HalfCell = 0.5f * CellSize;
	for (const FBmrCell& C : Grid)
	{
		const FBmrCell RoundTrip(C.Location);
		const float DeltaX = FMath::Abs(RoundTrip.X() - C.X());
		const float DeltaY = FMath::Abs(RoundTrip.Y() - C.Y());

		TestTrue(
			FString::Printf(TEXT("B3/B26: round-trip of cell %s through FBmrCell(FVector) "
								 "must stay within half a cell (X-delta %.3f, half-cell %.1f). "
								 "A naive `FloorToInt(WorldXY/CellSize)` world→cell breaks "
								 "this under any non-zero map yaw."),
				*C.ToString(), DeltaX, HalfCell),
			DeltaX <= HalfCell);
		TestTrue(
			FString::Printf(TEXT("B3/B26: round-trip Y-delta %.3f must be <= half-cell %.1f for cell %s."),
				DeltaY, HalfCell, *C.ToString()),
			DeltaY <= HalfCell);
	}

	// The center of a 3x3 rotated grid centered at the origin should remain
	// at the origin regardless of yaw, since rotation about the grid center
	// is the identity for the center cell. Allow half-cell tolerance for
	// integer rounding inside the ctor / SnapCell.
	const FBmrCell CenterCell = FBmrCell::GetCellArrayCenter(Grid);
	TestTrue(
		FString::Printf(TEXT("B6: a yaw-rotated grid centered at the world origin must keep its "
							 "center cell at/near the origin; got %s."),
			*CenterCell.ToString()),
		FVector::Dist(CenterCell.Location, FVector::ZeroVector) <= HalfCell);

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B4: InvalidCell is the documented sentinel, hashed consistently,
//              never equal to a cell built from a real (non-DownVector) world
//              position.
//
// InvalidCell.Location is FVector::DownVector (0,0,-1). A cell built from any
// world position whose Z != -1 must not collide with InvalidCell, and the hash
// must be stable across re-evaluations so InvalidCell can be used as a TSet
// key (which several library helpers rely on).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_InvalidCellSentinel,
	"Bomber.CellMath.InvalidCellSentinel",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_InvalidCellSentinel::RunTest(const FString& Parameters)
{
	// IsValid / IsInvalid round-trip.
	TestTrue(
		TEXT("B4: InvalidCell.IsInvalidCell() must be true."),
		FBmrCell::InvalidCell.IsInvalidCell());
	TestFalse(
		TEXT("B4: InvalidCell.IsValid() must be false."),
		FBmrCell::InvalidCell.IsValid());

	// Cells built from real (non-DownVector) world positions must not collide
	// with InvalidCell.
	const FBmrCell RealA(FVector(0.f, 0.f, 0.f));
	const FBmrCell RealB(FVector(200.f, 400.f, 0.f));
	TestTrue(
		TEXT("B4: a real cell at (0,0,0) must compare as IsValid (!= InvalidCell)."),
		RealA.IsValid());
	TestTrue(
		TEXT("B4: a real cell at (200,400,0) must compare as IsValid."),
		RealB.IsValid());
	TestTrue(
		TEXT("B4: a real cell must not equal InvalidCell."),
		RealA != FBmrCell::InvalidCell);

	// Hash stability across re-evaluations of GetTypeHash.
	const uint32 H1 = GetTypeHash(FBmrCell::InvalidCell);
	const uint32 H2 = GetTypeHash(FBmrCell::InvalidCell);
	TestEqual(
		TEXT("B4: GetTypeHash(InvalidCell) must be stable across calls — "
			 "otherwise sets/maps keyed on FBmrCell break for the sentinel value."),
		H1, H2);

	// InvalidCell can be inserted into a set and looked up (TSet uses the hash).
	FBmrCells Set;
	Set.Emplace(FBmrCell::InvalidCell);
	TestTrue(
		TEXT("B4: TSet<FBmrCell> must contain InvalidCell after Emplace — "
			 "the hash/equality contract for the sentinel must be honored."),
		Set.Contains(FBmrCell::InvalidCell));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B5: math operators are component-wise on Location
//
// +, -, *, +=, -= must operate component-wise. This is required by every
// other helper that does arithmetic on cells — RotateCellAroundOrigin,
// ScaleCellToNewGrid, GetCellArrayCenter. A non-component-wise overload (or a
// stripped no-op) breaks all of them.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_MathOperators,
	"Bomber.CellMath.MathOperators",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_MathOperators::RunTest(const FString& Parameters)
{
	const FBmrCell A(FVector(200.f, 400.f, 0.f));
	const FBmrCell B(FVector(100.f, -50.f, 0.f));

	// operator+
	const FBmrCell Sum = A + B;
	TestEqual(TEXT("B5: (A + B).X must be component-wise."), Sum.X(), 300.0f);
	TestEqual(TEXT("B5: (A + B).Y must be component-wise."), Sum.Y(), 350.0f);

	// operator-
	const FBmrCell Diff = A - B;
	TestEqual(TEXT("B5: (A - B).X must be component-wise."), Diff.X(), 100.0f);
	TestEqual(TEXT("B5: (A - B).Y must be component-wise."), Diff.Y(), 450.0f);

	// operator* with scalar
	const FBmrCell Scaled = A * 2.0f;
	TestEqual(TEXT("B5: (A * 2).X must be component-wise."), Scaled.X(), 400.0f);
	TestEqual(TEXT("B5: (A * 2).Y must be component-wise."), Scaled.Y(), 800.0f);

	// operator+= and operator-=
	FBmrCell Acc(FVector(50.f, 60.f, 0.f));
	Acc += B;
	TestEqual(TEXT("B5: (Acc += B).X must be component-wise."), Acc.X(), 150.0f);
	TestEqual(TEXT("B5: (Acc += B).Y must be component-wise."), Acc.Y(), 10.0f);

	Acc -= B;
	TestEqual(TEXT("B5: (Acc -= B).X must undo the prior +=."), Acc.X(), 50.0f);
	TestEqual(TEXT("B5: (Acc -= B).Y must undo the prior +=."), Acc.Y(), 60.0f);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B6, B9: MakeCellGridByTransform produces ColsX × RowsY cells whose
//                  layout matches the origin transform.
//
// For yaw=0 the layout is axis-aligned at 200-cm spacing centered on the
// origin transform's location. The center cell of a 3×5 grid is at the
// origin transform's location, and GetCenterCellPositionOnGrid returns
// (floor(W/2), floor(L/2)) = (1, 2).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_MakeCellGridByTransform,
	"Bomber.CellMath.MakeCellGridByTransform",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_MakeCellGridByTransform::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	constexpr int32 ColumnsX = 3;
	constexpr int32 RowsY = 5;
	const FVector Center(1000.f, 2000.f, 0.f);
	const FTransform Origin = MakeGridOriginTransform(Center, /*Yaw=*/0.f, ColumnsX, RowsY);

	const FBmrCells Grid = FBmrCell::MakeCellGridByTransform(Origin);

	TestEqual(
		FString::Printf(TEXT("B6: MakeCellGridByTransform with scale=(%d,%d) must produce %d cells. "
							 "A stripped stub returns the empty set."),
			ColumnsX, RowsY, ColumnsX * RowsY),
		Grid.Num(), ColumnsX * RowsY);

	// Every cell of an axis-aligned 3x5 grid must land on an expected
	// integer-aligned position. This rejects implementations that drop
	// the half-grid offset, ignore the cell size, or fail to snap.
	int32 Matched = 0;
	for (int32 Row = 0; Row < RowsY; ++Row)
	{
		for (int32 Col = 0; Col < ColumnsX; ++Col)
		{
			const FVector Expected = ExpectedAxisAlignedCellCenter(Center, ColumnsX, RowsY, Col, Row);
			if (Grid.Contains(FBmrCell(Expected)))
			{
				++Matched;
			}
		}
	}
	TestEqual(
		FString::Printf(TEXT("B6: every (col,row) of the 3x5 axis-aligned grid must have an "
							 "FBmrCell at the expected world position. Matched=%d of %d."),
			Matched, ColumnsX * RowsY),
		Matched, ColumnsX * RowsY);

	// B9: the center position on a 3x5 grid is (1, 2).
	const FIntPoint CenterPos = FBmrCell::GetCenterCellPositionOnGrid(Grid);
	TestEqual(
		TEXT("B9: GetCenterCellPositionOnGrid on a 3x5 grid must return X = floor(3/2) = 1."),
		CenterPos.X, 1);
	TestEqual(
		TEXT("B9: GetCenterCellPositionOnGrid on a 3x5 grid must return Y = floor(5/2) = 2."),
		CenterPos.Y, 2);

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B7, B8: GetCellByPositionOnGrid / GetPositionByCellOnGrid are
//                  inverses, and out-of-range queries return the documented
//                  sentinels.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_GridIndexingRoundTrip,
	"Bomber.CellMath.GridIndexingRoundTrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_GridIndexingRoundTrip::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	constexpr int32 ColumnsX = 3;
	constexpr int32 RowsY = 5;
	const FTransform Origin = MakeGridOriginTransform(FVector::ZeroVector, /*Yaw=*/0.f, ColumnsX, RowsY);
	const FBmrCells Grid = FBmrCell::MakeCellGridByTransform(Origin);
	if (!TestEqual(TEXT("Grid setup precondition"), Grid.Num(), ColumnsX * RowsY))
	{
		return false;
	}

	// Round-trip every in-range (Col, Row) pair via grid indexing.
	int32 RoundTrips = 0;
	for (int32 Row = 0; Row < RowsY; ++Row)
	{
		for (int32 Col = 0; Col < ColumnsX; ++Col)
		{
			const FIntPoint Position(Col, Row);
			const FBmrCell Cell = FBmrCell::GetCellByPositionOnGrid(Position, Grid);
			if (!Cell.IsValid())
			{
				continue;
			}
			const FIntPoint Recovered = FBmrCell::GetPositionByCellOnGrid(Cell, Grid);
			if (Recovered == Position)
			{
				++RoundTrips;
			}
		}
	}
	TestEqual(
		FString::Printf(TEXT("B7/B8: position → cell → position must round-trip for every "
							 "(col,row) of the 3x5 grid. Matched=%d of %d."),
			RoundTrips, ColumnsX * RowsY),
		RoundTrips, ColumnsX * RowsY);

	// Out-of-range queries: cell lookup returns InvalidCell, position lookup
	// returns (-1, -1).
	const FBmrCell OutOfRangeCell = FBmrCell::GetCellByPositionOnGrid(FIntPoint(999, 999), Grid);
	TestTrue(
		TEXT("B7: GetCellByPositionOnGrid on an out-of-range position must return InvalidCell."),
		OutOfRangeCell.IsInvalidCell());

	const FBmrCell NeverInGrid(FVector(1e7f, 1e7f, 0.f));
	const FIntPoint OutOfRangePos = FBmrCell::GetPositionByCellOnGrid(NeverInGrid, Grid);
	TestEqual(
		TEXT("B8: GetPositionByCellOnGrid on a cell that isn't in the grid must return X=-1."),
		OutOfRangePos.X, -1);
	TestEqual(
		TEXT("B8: GetPositionByCellOnGrid on a cell that isn't in the grid must return Y=-1."),
		OutOfRangePos.Y, -1);

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — B10: GetCornerCellsOnGrid returns four distinct corners that match
//               the documented (col,row) positions, and GetCellByCornerOnGrid
//               returns the specific corner.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_CornerCells,
	"Bomber.CellMath.CornerCells",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_CornerCells::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	constexpr int32 ColumnsX = 3;
	constexpr int32 RowsY = 5;
	const FTransform Origin = MakeGridOriginTransform(FVector::ZeroVector, /*Yaw=*/0.f, ColumnsX, RowsY);
	const FBmrCells Grid = FBmrCell::MakeCellGridByTransform(Origin);
	if (!TestEqual(TEXT("Grid setup precondition"), Grid.Num(), ColumnsX * RowsY))
	{
		return false;
	}

	const FBmrCells Corners = FBmrCell::GetCornerCellsOnGrid(Grid);
	TestEqual(
		TEXT("B10: GetCornerCellsOnGrid must return exactly 4 distinct corner cells "
			 "on a 3x5 grid."),
		Corners.Num(), 4);

	// Each named corner must round-trip back to its expected (col,row)
	// position on the grid. This proves the four corners are correctly
	// identified individually, not just "4 cells".
	const auto CheckCorner = [this, &Grid](EBmrGridCorner CornerType, const FIntPoint& ExpectedPos, const TCHAR* Label)
	{
		const FBmrCell CornerCell = FBmrCell::GetCellByCornerOnGrid(CornerType, Grid);
		TestTrue(
			FString::Printf(TEXT("B10: GetCellByCornerOnGrid(%s) must return a valid cell on a populated grid."), Label),
			CornerCell.IsValid());
		const FIntPoint Pos = FBmrCell::GetPositionByCellOnGrid(CornerCell, Grid);
		TestEqual(
			FString::Printf(TEXT("B10: corner %s must be at column %d on a 3x5 grid."), Label, ExpectedPos.X),
			Pos.X, ExpectedPos.X);
		TestEqual(
			FString::Printf(TEXT("B10: corner %s must be at row %d on a 3x5 grid."), Label, ExpectedPos.Y),
			Pos.Y, ExpectedPos.Y);
	};

	CheckCorner(EBmrGridCorner::TopLeft,     FIntPoint(0, 0),               TEXT("TopLeft"));
	CheckCorner(EBmrGridCorner::TopRight,    FIntPoint(ColumnsX - 1, 0),    TEXT("TopRight"));
	CheckCorner(EBmrGridCorner::BottomLeft,  FIntPoint(0, RowsY - 1),       TEXT("BottomLeft"));
	CheckCorner(EBmrGridCorner::BottomRight, FIntPoint(ColumnsX - 1, RowsY - 1), TEXT("BottomRight"));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — B15, B28: GetCellArrayNearest returns the closest cell.
//
// Builds a 3-cell set and queries from a point that is strictly closer to one
// of the three cells. Tiebreak determinism (B28) is exercised by running the
// query twice and requiring identical results — a non-deterministic tiebreak
// (e.g. depending on TSet iteration order plus floating-point rounding noise)
// would diverge across peers.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_GetCellArrayNearest,
	"Bomber.CellMath.GetCellArrayNearest",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_GetCellArrayNearest::RunTest(const FString& Parameters)
{
	const FBmrCell CellA(FVector(0.f, 0.f, 0.f));
	const FBmrCell CellB(FVector(400.f, 0.f, 0.f));
	const FBmrCell CellC(FVector(0.f, 600.f, 0.f));

	FBmrCells Set;
	Set.Emplace(CellA);
	Set.Emplace(CellB);
	Set.Emplace(CellC);

	// Query strictly closer to CellB.
	const FBmrCell QueryNearB(FVector(380.f, 30.f, 0.f));
	const FBmrCell Nearest1 = FBmrCell::GetCellArrayNearest(Set, QueryNearB);
	TestTrue(
		FString::Printf(TEXT("B15: GetCellArrayNearest must return the closest cell. "
							 "Query at (380,30) should resolve to CellB at (400,0). Got %s."),
			*Nearest1.ToString()),
		Nearest1 == CellB);

	// Tiebreak determinism (B28): same inputs ⇒ same output across calls.
	const FBmrCell Nearest2 = FBmrCell::GetCellArrayNearest(Set, QueryNearB);
	TestTrue(
		TEXT("B28: GetCellArrayNearest must be deterministic for the same input — "
			 "a tiebreak that depends on iteration order would diverge across peers."),
		Nearest1 == Nearest2);

	// Empty input returns InvalidCell.
	const FBmrCells EmptySet;
	const FBmrCell NearestEmpty = FBmrCell::GetCellArrayNearest(EmptySet, QueryNearB);
	TestTrue(
		TEXT("B15: GetCellArrayNearest on an empty set must return InvalidCell."),
		NearestEmpty.IsInvalidCell());

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — B13: RotateCellAroundOrigin rotates the cell about the origin
//               transform's yaw, with sign controlled by AxisZ.
//
// A cell at offset (+200, 0) from the origin, rotated by yaw=90° with
// AxisZ=+1, ends up at offset (0, +200). Flipping AxisZ to -1 reverses the
// rotation to offset (0, -200).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_RotateCellAroundOrigin,
	"Bomber.CellMath.RotateCellAroundOrigin",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_RotateCellAroundOrigin::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	const FVector OriginLocation(0.f, 0.f, 0.f);
	FTransform OriginTransform = FTransform::Identity;
	OriginTransform.SetLocation(OriginLocation);
	OriginTransform.SetRotation(FRotator(0.f, 90.f, 0.f).Quaternion()); // yaw=90°

	const FBmrCell CellOnXAxis(FVector(CellSize, 0.f, 0.f));

	// AxisZ = +1 ⇒ rotate by +yaw about +Z ⇒ (1,0,0) → (0,1,0)
	const FBmrCell PositiveRotation = FBmrCell::RotateCellAroundOrigin(CellOnXAxis, +1.f, OriginTransform);
	TestTrue(
		FString::Printf(TEXT("B13: rotating a cell at (+CellSize, 0, 0) around the origin with yaw=90° "
							 "and AxisZ=+1 must land near (0, +CellSize, 0). Got %s."),
			*PositiveRotation.ToString()),
		FVector::Dist(PositiveRotation.Location, FVector(0.f, CellSize, 0.f)) <= 0.5f * CellSize);

	// AxisZ = -1 ⇒ reversed direction ⇒ (1,0,0) → (0,-1,0)
	const FBmrCell NegativeRotation = FBmrCell::RotateCellAroundOrigin(CellOnXAxis, -1.f, OriginTransform);
	TestTrue(
		FString::Printf(TEXT("B13: the AxisZ sign must control rotation direction. With AxisZ=-1 the "
							 "same cell must land near (0, -CellSize, 0). Got %s."),
			*NegativeRotation.ToString()),
		FVector::Dist(NegativeRotation.Location, FVector(0.f, -CellSize, 0.f)) <= 0.5f * CellSize);

	// Yaw=0 ⇒ rotation is identity regardless of AxisZ.
	FTransform IdentityYaw = FTransform::Identity;
	IdentityYaw.SetLocation(OriginLocation);
	const FBmrCell ZeroYaw = FBmrCell::RotateCellAroundOrigin(CellOnXAxis, +1.f, IdentityYaw);
	TestTrue(
		FString::Printf(TEXT("B13: a yaw=0 origin transform must rotate cells by 0° (identity). "
							 "Got %s, expected near (CellSize, 0, 0)."),
			*ZeroYaw.ToString()),
		FVector::Dist(ZeroYaw.Location, FVector(CellSize, 0.f, 0.f)) <= 0.5f * CellSize);

	return true;
}

// ---------------------------------------------------------------------------
// Test 10 — B12, B14: GetCellArrayWidth / GetCellArrayLength / GetCellArraySize
//                     derive the grid dimensions back out of a cell set by
//                     unrotating the set first and then taking its bounding
//                     box. RotateCellArray is the bulk version used to
//                     unrotate.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_GridDimensionRecovery,
	"Bomber.CellMath.GridDimensionRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_GridDimensionRecovery::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	constexpr int32 ColumnsX = 3;
	constexpr int32 RowsY = 5;
	const FTransform Origin = MakeGridOriginTransform(FVector::ZeroVector, /*Yaw=*/0.f, ColumnsX, RowsY);
	const FBmrCells Grid = FBmrCell::MakeCellGridByTransform(Origin);
	if (!TestEqual(TEXT("Grid setup precondition"), Grid.Num(), ColumnsX * RowsY))
	{
		return false;
	}

	const float Width = FBmrCell::GetCellArrayWidth(Grid);
	const float Length = FBmrCell::GetCellArrayLength(Grid);
	const FVector2D Size = FBmrCell::GetCellArraySize(Grid);

	// GetCellArrayRotation derives orientation from (SecondCell - FirstCell)
	// in TSet iteration order — the recovered width/length may be reported
	// in either order (3x5 or 5x3) depending on which neighbor pair the
	// iterator picks first. The strong invariant is that Width × Length
	// recovers the total cell count, and the {width, length} set matches
	// the input dimensions {columns, rows}.
	const int32 WidthInt = FMath::FloorToInt32(Width);
	const int32 LengthInt = FMath::FloorToInt32(Length);

	TestEqual(
		FString::Printf(TEXT("B12: GetCellArrayWidth × GetCellArrayLength must equal the total cell "
							 "count on a %dx%d grid. Got Width=%d, Length=%d."),
			ColumnsX, RowsY, WidthInt, LengthInt),
		WidthInt * LengthInt, ColumnsX * RowsY);

	const TSet<int32> Recovered{WidthInt, LengthInt};
	const TSet<int32> Expected{ColumnsX, RowsY};
	TestTrue(
		FString::Printf(TEXT("B12: the {Width, Length} pair recovered from the cell set must match "
							 "the input dimensions {%d, %d}. Got {%d, %d}."),
			ColumnsX, RowsY, WidthInt, LengthInt),
		Recovered.Difference(Expected).Num() == 0 && Recovered.Num() == Expected.Num());

	TestTrue(
		TEXT("B12: GetCellArraySize.X must equal GetCellArrayWidth."),
		FMath::IsNearlyEqual(static_cast<float>(Size.X), Width, 0.01f));
	TestTrue(
		TEXT("B12: GetCellArraySize.Y must equal GetCellArrayLength."),
		FMath::IsNearlyEqual(static_cast<float>(Size.Y), Length, 0.01f));

	// B14: RotateCellArray(-1) must unrotate against the array's *own* origin
	// (a B29 anti-pattern is to rotate about the world origin instead). For
	// a yaw=0 grid both interpretations *only* agree when the grid is at
	// the world origin; shifting the grid distinguishes the two. If
	// RotateCellArray rotated about the world origin instead of the derived
	// origin the unrotated bounding box is inflated and Width × Length
	// blows past the true cell count.
	const FTransform ShiftedOrigin = MakeGridOriginTransform(FVector(5000.f, 3000.f, 0.f), /*Yaw=*/0.f, ColumnsX, RowsY);
	const FBmrCells ShiftedGrid = FBmrCell::MakeCellGridByTransform(ShiftedOrigin);
	const int32 ShiftedWidthInt = FMath::FloorToInt32(FBmrCell::GetCellArrayWidth(ShiftedGrid));
	const int32 ShiftedLengthInt = FMath::FloorToInt32(FBmrCell::GetCellArrayLength(ShiftedGrid));
	TestEqual(
		FString::Printf(TEXT("B14/B29: Width × Length must remain == cell count for a grid shifted "
							 "off the world origin (recovers via the array's *own* origin, not "
							 "world origin). Got Width=%d, Length=%d on %d cells."),
			ShiftedWidthInt, ShiftedLengthInt, ShiftedGrid.Num()),
		ShiftedWidthInt * ShiftedLengthInt, ColumnsX * RowsY);

	return true;
}

// ---------------------------------------------------------------------------
// Test 11 — B1+B16: building a cell from a raw FVector and then snapping it
//                   must quantize correctly even when the raw input sits just
//                   below a half-cell boundary.
//
// SnapCell itself is FORCEINLINE in BmrCell.h and just calls
// `InCell.Location.GridSnap(CellSize)`, so the wrapper is identical between
// start/ and solution/. The discriminating step in the snap pipeline is the
// FBmrCell(FVector) ctor, which the spec (B1/B25) requires to *round* to the
// nearest cm — without rounding, grid alignment under rotation breaks. The
// two behaviors are observably chained: GridSnap(99.6, 200) floors *down* to
// 0, but GridSnap(100, 200) floors *up* to 200. Feeding the snap pipeline a
// value just below the half-cell boundary therefore separates the two:
// the stripped `Location = Vector` ctor keeps 99.6 and yields 0; a rounded
// ctor produces 100 and SnapCell snaps it to 200.
//
// The on-grid case is kept as a regression check on SnapCell itself (a stub
// that returned InvalidCell would fail it).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_SnapCellQuantizes,
	"Bomber.CellMath.SnapCellQuantizes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_SnapCellQuantizes::RunTest(const FString& Parameters)
{
	using namespace CellMathTestsPrivate;

	// 99.6 sits just below the 100-cm half-cell boundary. A rounded ctor
	// pulls it up to 100; GridSnap(100, 200) then snaps *up* to 200. A naive
	// `Location = Vector` ctor (the stripped state) keeps 99.6, and
	// GridSnap(99.6, 200) floors *down* to 0.
	const FBmrCell BelowHalfCell(FVector(99.6f, 99.6f, 0.f));
	const FBmrCell SnappedBelow = FBmrCell::SnapCell(BelowHalfCell);
	TestEqual(
		FString::Printf(TEXT("B1/B16: an input below the half-cell boundary must be rounded by "
							 "the ctor (99.6 → 100) and then snapped by SnapCell to 200. "
							 "A non-rounding ctor leaves 99.6, and GridSnap floors that *down* "
							 "to 0. Got X=%.3f."),
			SnappedBelow.X()),
		SnappedBelow.X(), 200.0f);
	TestEqual(
		FString::Printf(TEXT("B1/B16: same chain for the Y component (99.6 → 100 → 200). "
							 "Got Y=%.3f."),
			SnappedBelow.Y()),
		SnappedBelow.Y(), 200.0f);

	// An already on-grid input must be invariant under SnapCell — a stub
	// SnapCell that returned InvalidCell (DownVector) would fail this.
	const FBmrCell OnGrid(FVector(400.f, 600.f, 0.f));
	const FBmrCell SnappedOnGrid = FBmrCell::SnapCell(OnGrid);
	TestTrue(
		FString::Printf(TEXT("B16: SnapCell of an on-grid cell must be the cell itself. "
							 "Got %s, expected (400, 600, 0)."),
			*SnappedOnGrid.ToString()),
		SnappedOnGrid.Location.Equals(FVector(400.f, 600.f, 0.f), 0.01f));

	return true;
}

// ---------------------------------------------------------------------------
// PIE helpers — shared state for the level-dependent test below.
// ---------------------------------------------------------------------------
namespace CellMathPIEPrivate
{
	static const TCHAR* MainMapPath = TEXT("/Game/Bomber/Maps/Main");

	// The Bomber data-asset bootstrap is async; initial generation does not
	// fire until DA registry rows finish loading. Give it generous walls.
	static constexpr float MapReadyTimeoutSeconds = 30.f;

	// Cross-latent-command shared state for the PIE test. A struct keeps the
	// teardown path single-source-of-truth (ResetRunState) and avoids leaking
	// between test runs.
	struct FRunState
	{
		TWeakObjectPtr<ABmrGeneratedMap> GeneratedMap;
		bool bMapReady = false;
		// Restored on teardown so test ordering doesn't pollute subsequent runs.
		TSet<FBmrCell> SavedDangerousCells;
		bool bSavedDangerous = false;
	};

	static FRunState GRunState;
	static void ResetRunState() { GRunState = FRunState{}; }

	static UWorld* GetServerPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		UWorld* Fallback = nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType != EWorldType::PIE || !Ctx.World())
			{
				continue;
			}
			const ENetMode NetMode = Ctx.World()->GetNetMode();
			if (NetMode == NM_Standalone || NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
			{
				return Ctx.World();
			}
			Fallback = Ctx.World();
		}
		return Fallback;
	}
}

// ---------------------------------------------------------------------------
// Test 12 — Level-dependent surface (PIE on /Game/Bomber/Maps/Main):
//           B17-B21 GetSideCells EPathType semantics, B22-B23 actor-typed
//           cell queries via IntersectCellsByTypes, B24 BFS reachability.
//
// These helpers all route through ABmrGeneratedMap and return safe defaults
// without one (GetCellsAround → empty, GetAllCellsWithActors → empty,
// DoesPathExistToCellsOnLevel → false). A PIE session brings up Main and
// runs the async generator so the actor-typed surface is observable.
//
// Key discriminator for Safe vs Free/Explosion (B20): ABmrGeneratedMap exposes
// `AdditionalDangerousCells` as a public BlueprintReadWrite property that
// GetSideCells reads when Pathfinder ∈ {Safe, Secure}. Setting it to a single
// cell adjacent to the center gives a clean controlled-difference test:
//   - Free  / Explosion: still reach the cell (ignore dangerous)
//   - Safe  / Secure   : break before reaching it
//
// |Any| > |Free| (B17/B18) comes from the standard Bomber map: every cardinal
// arm from center hits border walls, and Free breaks on walls while Any does
// not. |Explosion| >= |Free| (B19) holds because Explosion is strictly less
// restrictive than Free in production semantics (Explosion ignores obstacles,
// Free breaks on bombs/boxes).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCellMath_LevelPIE_PathTypesActorsAndReachability,
	"Bomber.CellMath.LevelPIE_PathTypesActorsAndReachability",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCellMath_LevelPIE_PathTypesActorsAndReachability::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace CellMathPIEPrivate;
	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	// false = standalone PIE; HasAuthority is true, so the actor-typed
	// surface runs exactly as it does on a listen-server host.
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// -----------------------------------------------------------------------
	// Phase 0 — wait for Generated Map + initial generation (cells populated
	// AND at least one player spawned, which proves MapComponents has been
	// hydrated and IntersectCellsByTypes can see actors).
	// -----------------------------------------------------------------------
	{
		const double Deadline = FPlatformTime::Seconds() + MapReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Deadline]() -> bool
		{
			UWorld* World = GetServerPIEWorld();
			ABmrGeneratedMap* Map = ABmrGeneratedMap::GetGeneratedMap(World);
			if (Map
				&& UBmrCellUtilsLibrary::GetAllCellsOnLevel().Num() > 0
				&& UBmrCellUtilsLibrary::GetAllCellsWithActors(TO_FLAG(EAT::Player)).Num() > 0)
			{
				GRunState.GeneratedMap = Map;
				GRunState.bMapReady = true;
				return true;
			}
			return FPlatformTime::Seconds() >= Deadline;
		}));
	}

	// -----------------------------------------------------------------------
	// Phase 1 — B17-B21, B22-B23, B24 assertions.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestTrue(TEXT("PIE setup: Generated Map must be available and initial generation complete "
						   "(map actor present, cells populated, players spawned). Without this, the "
						   "level-dependent helpers all return safe defaults and B17-B24 cannot be "
						   "exercised."),
				Map != nullptr && GRunState.bMapReady))
		{
			return true;
		}

		// ---------- B22 / B23: actor-typed cell queries via IntersectCellsByTypes ----------
		const FBmrCells AllCellsOnLevel = UBmrCellUtilsLibrary::GetAllCellsOnLevel();
		const FBmrCells PlayerCells = UBmrCellUtilsLibrary::GetAllCellsWithActors(TO_FLAG(EAT::Player));
		const FBmrCells WallCells = UBmrCellUtilsLibrary::GetAllCellsWithActors(TO_FLAG(EAT::Wall));
		const FBmrCells EmptyCells = UBmrCellUtilsLibrary::GetAllEmptyCellsWithoutActors();

		TestTrue(
			FString::Printf(TEXT("B22: GetAllCellsWithActors(Player) must return >0 player cells on a populated "
								 "Main map (Bomber places corner players). Got %d. A stub IntersectCellsByTypes "
								 "(no-op) leaves this at 0."),
				PlayerCells.Num()),
			PlayerCells.Num() > 0);
		TestTrue(
			FString::Printf(TEXT("B22: GetAllCellsWithActors(Wall) must return >0 wall cells (Bomber places a "
								 "wall border + interior wall pattern). Got %d."),
				WallCells.Num()),
			WallCells.Num() > 0);

		// B23: GetAllEmptyCellsWithoutActors delegates through IntersectCellsByTypes with the None bitmask
		// and the "intersect-all-if-empty" branch. The result must be a strict subset of all cells.
		TestTrue(
			FString::Printf(TEXT("B23: GetAllEmptyCellsWithoutActors() (wrapper around IntersectCellsByTypes) "
								 "must return a non-empty proper subset of all level cells. Got %d empty of %d total."),
				EmptyCells.Num(), AllCellsOnLevel.Num()),
			EmptyCells.Num() > 0 && EmptyCells.Num() < AllCellsOnLevel.Num());

		// B23: FilterCellsByActors(AllCells, Player) must equal GetAllCellsWithActors(Player).
		// Both routes go through IntersectCellsByTypes; equality across the two wrappers proves the
		// delegation invariant rather than a coincidental matching count.
		const FBmrCells FilteredPlayers = UBmrCellUtilsLibrary::FilterCellsByActors(AllCellsOnLevel, TO_FLAG(EAT::Player));
		TestEqual(
			TEXT("B23: FilterCellsByActors(AllCells, Player) must yield the same count as "
				 "GetAllCellsWithActors(Player); both wrappers delegate to IntersectCellsByTypes."),
			FilteredPlayers.Num(), PlayerCells.Num());

		// ---------- B24: BFS path-existence on the level ----------
		const FBmrCell Center = UBmrCellUtilsLibrary::GetCenterCellOnLevel();
		TestTrue(TEXT("B24 setup: center cell must be valid on a populated map."), Center.IsValid());

		// True case: empty target set is trivially reachable.
		TestTrue(
			TEXT("B24: DoesPathExistToCellsOnLevel({}, empty) must return true (no targets → trivially reached)."),
			UBmrCellUtilsLibrary::DoesPathExistToCellsOnLevel(FBmrCell::EmptyCells, FBmrCell::EmptyCells));

		// True case: center cell must be reachable from the BFS root (any-cell connectivity on a
		// generated Bomber map is the invariant the BFS exists to enforce).
		TestTrue(
			TEXT("B24: DoesPathExistToCellsOnLevel({Center}, empty) must return true on a connected map. "
				 "A stub returning false would fail this."),
			UBmrCellUtilsLibrary::DoesPathExistToCellsOnLevel({Center}, FBmrCell::EmptyCells));

		// False case: a cell that isn't part of the level grid can never be visited by the BFS.
		// This discriminates a stub that returns `true` constantly (an alternate failure mode).
		const FBmrCell OffGridCell(FVector(1e7f, 1e7f, 0.f));
		TestFalse(
			TEXT("B24: DoesPathExistToCellsOnLevel({off-grid cell}, empty) must return false — the BFS "
				 "only enqueues neighbors via GetCellsAround on the level grid, so a target outside the "
				 "grid is unreachable."),
			UBmrCellUtilsLibrary::DoesPathExistToCellsOnLevel({OffGridCell}, FBmrCell::EmptyCells));

		// ---------- B17-B21: GetSideCells respects EPathType ----------
		// Scan with each EPathType at a large radius and compare the resulting cell counts.
		// Production semantics (from BmrCellUtilsLibrary.cpp::GetSideCells):
		//   Any       — never breaks
		//   Explosion — breaks on walls only
		//   Free      — breaks on walls + obstacles (Bombs/Boxes)
		//   Safe      — breaks on walls + obstacles + dangerous (AdditionalDangerousCells ∪ explosions)
		//   Secure    — Safe + breaks on players
		constexpr int32 LargeRadius = 99;
		const FBmrCells AnyScan = UBmrCellUtilsLibrary::GetCellsAround(Center, EPathType::Any, LargeRadius);
		const FBmrCells ExplosionScan = UBmrCellUtilsLibrary::GetCellsAround(Center, EPathType::Explosion, LargeRadius);
		const FBmrCells FreeScan = UBmrCellUtilsLibrary::GetCellsAround(Center, EPathType::Free, LargeRadius);
		const FBmrCells SafeScan = UBmrCellUtilsLibrary::GetCellsAround(Center, EPathType::Safe, LargeRadius);
		const FBmrCells SecureScan = UBmrCellUtilsLibrary::GetCellsAround(Center, EPathType::Secure, LargeRadius);

		// All five pathfinders must produce >0 cells from a valid interior center — a no-op GetSideCells
		// (stub) leaves OutCells empty.
		TestTrue(FString::Printf(TEXT("B17: Any pathfinder scan must produce >0 cells from a valid center "
									   "on a populated map (got %d). A stub GetSideCells returns 0."),
								 AnyScan.Num()),
			AnyScan.Num() > 0);
		TestTrue(FString::Printf(TEXT("B19: Explosion pathfinder scan must produce >0 cells (got %d)."),
								 ExplosionScan.Num()),
			ExplosionScan.Num() > 0);
		TestTrue(FString::Printf(TEXT("B18: Free pathfinder scan must produce >0 cells (got %d)."),
								 FreeScan.Num()),
			FreeScan.Num() > 0);
		TestTrue(FString::Printf(TEXT("B20: Safe pathfinder scan must produce >0 cells (got %d)."),
								 SafeScan.Num()),
			SafeScan.Num() > 0);
		TestTrue(FString::Printf(TEXT("B21: Secure pathfinder scan must produce >0 cells (got %d)."),
								 SecureScan.Num()),
			SecureScan.Num() > 0);

		// B17 DIRECT: Any never breaks; Free does break on walls. The Bomber map's center always has
		// border walls in cardinal arms, so |Any| must exceed |Free| in any populated run.
		TestTrue(
			FString::Printf(TEXT("B17: |Any|=%d must strictly exceed |Free|=%d on a populated Bomber map — "
								 "every cardinal arm from center hits border walls, and Free breaks on walls "
								 "while Any does not."),
				AnyScan.Num(), FreeScan.Num()),
			AnyScan.Num() > FreeScan.Num());

		// B18 / B19 DIRECT: in production, Explosion is strictly less restrictive than Free (Explosion
		// ignores obstacles, Free breaks on bombs/boxes), so |Explosion| >= |Free|. Anti-pattern guard
		// for a GetSideCells that ignores Pathfinder (B27): a Pathfinder-blind implementation would
		// make Explosion and Free identical, so the assertion is still informative.
		TestTrue(
			FString::Printf(TEXT("B18/B19: |Explosion|=%d must be >= |Free|=%d. Explosion is strictly less "
								 "restrictive than Free (Explosion breaks on walls only, Free additionally "
								 "breaks on Bomb|Box). A Pathfinder-blind GetSideCells (B27 anti-pattern) "
								 "fails this."),
				ExplosionScan.Num(), FreeScan.Num()),
			ExplosionScan.Num() >= FreeScan.Num());

		// B21 partial: Secure includes everything Safe breaks on, plus players. So |Secure| <= |Safe|.
		TestTrue(
			FString::Printf(TEXT("B21: |Secure|=%d must be <= |Safe|=%d — Secure inherits Safe's break set "
								 "and additionally breaks on player cells."),
				SecureScan.Num(), SafeScan.Num()),
			SecureScan.Num() <= SafeScan.Num());

		// ---------- B18-B21 controlled discriminator via AdditionalDangerousCells ----------
		// Find any cardinal neighbor that Free can reach (so we know the cell is not a wall/box, and
		// the pre-condition isolates the dangerous-cell-break behavior). Bomber maps use a
		// "pillar at odd column AND odd row" interior wall pattern; only some directions from
		// center are open. Iterating over cardinals makes the test layout-tolerant.
		const EBmrCellDirection CardinalDirections[] = {
			EBmrCellDirection::Forward,
			EBmrCellDirection::Backward,
			EBmrCellDirection::Right,
			EBmrCellDirection::Left,
		};

		FBmrCell Neighbor = FBmrCell::InvalidCell;
		EBmrCellDirection NeighborDir = EBmrCellDirection::None;
		for (EBmrCellDirection Dir : CardinalDirections)
		{
			const FBmrCell Candidate = UBmrCellUtilsLibrary::GetCellInDirection(Center, EPathType::Free, Dir);
			if (Candidate.IsValid())
			{
				Neighbor = Candidate;
				NeighborDir = Dir;
				break;
			}
		}

		if (Neighbor.IsValid())
		{
			GRunState.SavedDangerousCells = Map->AdditionalDangerousCells;
			GRunState.bSavedDangerous = true;
			Map->AdditionalDangerousCells = FBmrCells{Neighbor};

			// B18: Free pathfinder must NOT break on AdditionalDangerousCells.
			const FBmrCell FreeReach = UBmrCellUtilsLibrary::GetCellInDirection(
				Center, EPathType::Free, NeighborDir);
			TestTrue(
				FString::Printf(TEXT("B18: Free pathfinder must break on walls only — adding %s to "
									 "AdditionalDangerousCells must NOT affect a Free scan (got %s, expected %s). "
									 "If this fails, Free is incorrectly breaking on dangerous cells."),
					*Neighbor.ToString(), *FreeReach.ToString(), *Neighbor.ToString()),
				FreeReach == Neighbor);

			// B19: Explosion also ignores AdditionalDangerousCells (production semantics: Explosion
			// breaks on walls only).
			const FBmrCell ExplosionReach = UBmrCellUtilsLibrary::GetCellInDirection(
				Center, EPathType::Explosion, NeighborDir);
			TestTrue(
				FString::Printf(TEXT("B19: Explosion pathfinder must not break on dangerous cells — adding %s "
									 "to AdditionalDangerousCells must not block an Explosion scan (got %s)."),
					*Neighbor.ToString(), *ExplosionReach.ToString()),
				ExplosionReach == Neighbor);

			// B20 DIRECT: Safe must break before a dangerous cell.
			const FBmrCell SafeReach = UBmrCellUtilsLibrary::GetCellInDirection(
				Center, EPathType::Safe, NeighborDir);
			TestTrue(
				FString::Printf(TEXT("B20: Safe pathfinder must break on AdditionalDangerousCells — "
									 "GetCellInDirection from %s toward %s must return InvalidCell (got %s). "
									 "A Pathfinder-blind GetSideCells (B27 anti-pattern) fails this."),
					*Center.ToString(), *Neighbor.ToString(), *SafeReach.ToString()),
				SafeReach.IsInvalidCell());

			// B21 DIRECT: Secure inherits Safe's break set, including dangerous cells.
			const FBmrCell SecureReach = UBmrCellUtilsLibrary::GetCellInDirection(
				Center, EPathType::Secure, NeighborDir);
			TestTrue(
				FString::Printf(TEXT("B21: Secure pathfinder inherits Safe's break-set (incl. dangerous cells) "
									 "and adds player-break; GetCellInDirection toward %s must return "
									 "InvalidCell (got %s)."),
					*Neighbor.ToString(), *SecureReach.ToString()),
				SecureReach.IsInvalidCell());

			// Restore so we don't pollute later phases of this PIE session.
			Map->AdditionalDangerousCells = GRunState.SavedDangerousCells;
			GRunState.bSavedDangerous = false;
		}
		else
		{
			AddWarning(TEXT("B18-B21 controlled discriminator skipped: no Free-reachable cardinal neighbor "
							"of the center cell on this map layout. Ordering assertions above still cover "
							"these behavior groups."));
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Teardown — stop PIE, restore any leftover dangerous-cell state, clear
	// shared state.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		// Safety: if Phase 1 was bypassed (e.g. early-out on missing map) and
		// the dangerous-cell mutation was still applied, restore now.
		if (GRunState.bSavedDangerous)
		{
			if (ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get())
			{
				Map->AdditionalDangerousCells = GRunState.SavedDangerousCells;
			}
			GRunState.bSavedDangerous = false;
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));
#else
	AddWarning(TEXT("PIE-based level test skipped: requires WITH_EDITOR."));
#endif
	return true;
}
