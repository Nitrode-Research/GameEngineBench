#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/PBPlayerMovement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPBSlopeBoostingTest, "PB.Slope.HandleSlopeBoostingDeflectsDelta", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPBSlopeBoostingTest::RunTest(const FString& Parameters)
{
	UPBPlayerMovement* Move = NewObject<UPBPlayerMovement>();
	FHitResult Hit;
	Hit.ImpactNormal = FVector(0.0, 0.70710678, 0.70710678);
	const FVector SlideResult = FVector::ZeroVector;
	const FVector Delta(100.0, 50.0, -100.0);
	const FVector Normal(0.0, 1.0, 0.0);
	const FVector Result = Move->HandleSlopeBoosting(SlideResult, Delta, 0.5f, Normal, Hit);
	TestTrue(TEXT("HandleSlopeBoosting returns a deflected movement vector"), !Result.IsNearlyZero());
	TestNotEqual(TEXT("HandleSlopeBoosting does not just return the incoming slide result"), Result, SlideResult);
	return true;
}
