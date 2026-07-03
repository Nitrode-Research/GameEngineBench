#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GaussianDataTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoCompressionPackingTest, "Nano.Compression.BitPackingRoundTrips", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoCompressionPackingTest::RunTest(const FString& Parameters)
{
	uint32 X = 0, Y = 0;
	const uint32 Morton = GaussianSplattingUtils::EncodeMorton2D_16x16(9, 6);
	GaussianSplattingUtils::DecodeMorton2D_16x16(Morton, X, Y);
	TestEqual(TEXT("Morton 2D X round-trips"), X, 9u);
	TestEqual(TEXT("Morton 2D Y round-trips"), Y, 6u);
	float A = 0.0f, B = 0.0f;
	GaussianSplattingUtils::UnpackHalf2x16(GaussianSplattingUtils::PackHalf2x16(1.5f, -2.0f), A, B);
	TestEqual(TEXT("Half pack first lane"), A, 1.5f);
	TestEqual(TEXT("Half pack second lane"), B, -2.0f);
	TestEqual(TEXT("Zero scale uses sentinel"), GaussianSplattingUtils::EncodeScaleUint8(0.0f), uint8(0));
	uint32 Words[4] = {0,0,0,0};
	GaussianSplattingUtils::PackSplatToUint4(FVector3f(1,2,3), FQuat4f::Identity, FVector3f(1,2,3), 0.25f, 0.5f, 0.75f, 1.0f, Words);
	TestTrue(TEXT("Packed splat writes non-zero payload"), Words[0] != 0 || Words[1] != 0 || Words[2] != 0 || Words[3] != 0);
	return true;
}
