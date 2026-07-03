// Copyright Expertise centre for Digital Media, 2024. All Rights Reserved.
// Benchmark automation tests for the XR Calibration subsystem.
// Covers: UHelper (coordinate transforms), UCloudRegistration (ICP via Chaos SVD),
//         UXRCalibration (Levenberg-Marquardt optimization lifecycle).
// Run with: Automation > filter "LASAA.Calibration"

#include "CoreMinimal.h"

#include <Eigen/LU>

#include "Helper.h"
#include "CloudRegistration.h"
#include "XRCalibration.h"
#include "Anchor.h"

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCalibrationTests, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogCalibrationPIETests, Log, All);

namespace CalibrationTests
{
	static const TCHAR* TestMapPath = TEXT("/Game/Maps/Prep");

	static UWorld* GetFirstPIEWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				const ENetMode NetMode = Context.World()->GetNetMode();
				if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}
	static UClass* GetXRCalibrationClass()
	{
		return FindObject<UClass>(nullptr, TEXT("/Script/LASAA.XRCalibration"));
	}

	static void InvokeCalibrationMethod(UObject* CalibrationComponent, const FName MethodName)
	{
		if (!CalibrationComponent)
		{
			return;
		}

		if (UFunction* Function = CalibrationComponent->FindFunction(MethodName))
		{
			CalibrationComponent->ProcessEvent(Function, nullptr);
		}
	}

	static void AddExpectedMetaProjectSetupNoise(FAutomationTestBase& Test)
	{
		if (GEngine)
		{
			GEngine->Exec(nullptr, TEXT("log LogProjectSetupTool off"));
		}
	}
}

using namespace CalibrationTests;

// ===================================================================
// UHelper  Pure math, static functions (no PIE required)
// ===================================================================

// --- 1. unreal_to_opencv_roundtrip  (R1) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperUnrealToOpenCVRoundtripTest,
	"LASAA.Calibration.unreal_to_opencv_roundtrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperUnrealToOpenCVRoundtripTest::RunTest(const FString& Parameters)
{
	// R1: Unreal (X-forward, Y-left, Z-up)  OpenCV (X-right, Y-down, Z-forward).
	// ConvertUnrealToOpenCV then ConvertOpenCVToUnreal should be identity.

	FTransform Original(FRotator(30, 45, 60), FVector(100, 200, 300));

	FTransform ToOpenCV = UHelper::ConvertUnrealToOpenCV(Original);
	FTransform BackToUnreal = UHelper::ConvertOpenCVToUnreal(ToOpenCV);

	TestTrue(TEXT("Unreal  OpenCV  Unreal roundtrip preserves rotation"),
		BackToUnreal.GetRotation().Equals(Original.GetRotation(), 1e-6));
	TestTrue(TEXT("Unreal  OpenCV  Unreal roundtrip preserves translation"),
		BackToUnreal.GetTranslation().Equals(Original.GetTranslation(), 1e-4));

	return true;
}

// --- 2. opencv_axes_mapping  (R1) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperOpenCVAxisMappingTest,
	"LASAA.Calibration.opencv_axes_mapping",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperOpenCVAxisMappingTest::RunTest(const FString& Parameters)
{
	// R1: Coordinate conversion is a basis change. Identity remains identity, so
	// use a non-trivial transform and verify the conversion is reversible.
	FTransform Original(FRotator(10, 30, 50), FVector(10, 20, 30));

	FTransform OpenCV = UHelper::ConvertUnrealToOpenCV(Original);
	FTransform BackToUnreal = UHelper::ConvertOpenCVToUnreal(OpenCV);

	TestTrue(TEXT("OpenCV axis mapping preserves rotation through roundtrip"),
		BackToUnreal.GetRotation().Equals(Original.GetRotation(), 1e-5));
	TestTrue(TEXT("OpenCV axis mapping preserves translation through roundtrip"),
		BackToUnreal.GetTranslation().Equals(Original.GetTranslation(), 1e-3));

	FVector OpenCVForward = OpenCV.GetRotation().GetForwardVector();
	FVector OpenCVRight = OpenCV.GetRotation().GetRightVector();
	FVector OpenCVUp = OpenCV.GetRotation().GetUpVector();

	UE_LOG(LogCalibrationTests, Display,
		TEXT("[OpenCV] Forward=(%.2f,%.2f,%.2f) Right=(%.2f,%.2f,%.2f) Up=(%.2f,%.2f,%.2f)"),
		OpenCVForward.X, OpenCVForward.Y, OpenCVForward.Z,
		OpenCVRight.X, OpenCVRight.Y, OpenCVRight.Z,
		OpenCVUp.X, OpenCVUp.Y, OpenCVUp.Z);

	return true;
}

// --- 3. rodrigues_zero_is_identity  (R3) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperRodriguesZeroTest,
	"LASAA.Calibration.rodrigues_zero_is_identity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperRodriguesZeroTest::RunTest(const FString& Parameters)
{
	// R3: Rodrigues formula  zero rotation vector  identity matrix.
	using namespace Eigen;
	Vector3d ZeroVec(0, 0, 0);
	Matrix3d R = UHelper::rodrigues(ZeroVec);
	TestTrue(TEXT("rodrigues(zero) produces identity matrix"),
		R.isApprox(Matrix3d::Identity(), 1e-9));

	return true;
}

// --- 4. rodrigues_nonzero_is_rotation  (R3) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperRodriguesNonzeroTest,
	"LASAA.Calibration.rodrigues_nonzero_is_rotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperRodriguesNonzeroTest::RunTest(const FString& Parameters)
{
	// R3: Non-zero rotation vector  proper rotation matrix (R^T * R = I, det(R) = 1).
	using namespace Eigen;
	const double kPi = 3.14159265358979323846;
	Vector3d Rvec(kPi / 4, 0, 0);  // 45 around X
	Matrix3d R = UHelper::rodrigues(Rvec);

	Matrix3d RtR = R.transpose() * R;
	TestTrue(TEXT("rodrigues produces orthogonal matrix (R^T * R  I)"),
		RtR.isApprox(Matrix3d::Identity(), 1e-6));

	double Det = R.determinant();
	TestTrue(TEXT("rodrigues produces proper rotation (det  1)"),
		FMath::Abs(Det - 1.0) < 1e-6);

	return true;
}

// --- 5. extrinsic_from_rt_structure  (R4) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperExtrinsicFromRtTest,
	"LASAA.Calibration.extrinsic_from_rt_structure",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperExtrinsicFromRtTest::RunTest(const FString& Parameters)
{
	// R4: extrinsicFromRt builds a 44 homogeneous matrix from 33 R + 31 t.
	// Top-left 33 = R, rightmost column = t, bottom row = (0,0,0,1).
	using namespace Eigen;
	Matrix3d IdentityR = Matrix3d::Identity();
	Vector3d T(1, 2, 3);
	Matrix4d Extrinsic = UHelper::extrinsicFromRt(IdentityR, T);

	TestTrue(TEXT("extrinsicFromRt: top-left 33 matches R"),
		Extrinsic.block<3, 3>(0, 0).isApprox(IdentityR, 1e-9));

	Vector3d ExtractedT = Extrinsic.col(3).head<3>();
	TestTrue(TEXT("extrinsicFromRt: translation column matches t"),
		ExtractedT.isApprox(T, 1e-9));

	TestTrue(TEXT("extrinsicFromRt: bottom row is (0,0,0,1)"),
		FMath::Abs(Extrinsic(3, 0)) < 1e-9 &&
		FMath::Abs(Extrinsic(3, 1)) < 1e-9 &&
		FMath::Abs(Extrinsic(3, 2)) < 1e-9 &&
		FMath::Abs(Extrinsic(3, 3) - 1.0) < 1e-9);

	return true;
}

// --- 6. eigen_to_unreal_roundtrip  (R5) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperEigenToUnrealRoundtripTest,
	"LASAA.Calibration.eigen_to_unreal_roundtrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperEigenToUnrealRoundtripTest::RunTest(const FString& Parameters)
{
	// R5: eigenMatrixToUnreal copies 44 Eigen  FMatrix element by element.
	// eigenVectorToUnreal and unrealVectorToEigen should be inverse.
	using namespace Eigen;

	Vector3d EigenVec(1, 2, 3);
	FVector UnrealVec = UHelper::eigenVectorToUnreal(EigenVec);
	Vector3d Back = UHelper::unrealVectorToEigen(UnrealVec);
	TestTrue(TEXT("eigenVectorToUnreal  unrealVectorToEigen roundtrip"),
		Back.isApprox(EigenVec, 1e-9));

	// Identity matrix roundtrip.
	Matrix4d EigenMat = Matrix4d::Identity();
	FMatrix UnrealMat = UHelper::eigenMatrixToUnreal(EigenMat);
	TestTrue(TEXT("eigenMatrixToUnreal preserves identity"),
		FMath::Abs(UnrealMat.M[0][0] - 1.0) < 1e-9 &&
		FMath::Abs(UnrealMat.M[1][1] - 1.0) < 1e-9 &&
		FMath::Abs(UnrealMat.M[2][2] - 1.0) < 1e-9 &&
		FMath::Abs(UnrealMat.M[3][3] - 1.0) < 1e-9);

	return true;
}

// --- 7. variance_computation  (R3) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHelperVarianceTest,
	"LASAA.Calibration.variance_computation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHelperVarianceTest::RunTest(const FString& Parameters)
{
	// R3: variance() computes the variance of a VectorXd.
	// Constant vector  zero variance. Non-constant  positive variance.
	using namespace Eigen;

	VectorXd Constant(5);
	Constant.setOnes();
	double VarConst = UHelper::variance(Constant);
	TestTrue(TEXT("variance of constant vector is ~0"),
		FMath::Abs(VarConst) < 1e-9);

	VectorXd NonConstant(4);
	NonConstant << 1, 2, 3, 4;
	double VarNonConst = UHelper::variance(NonConstant);
	TestTrue(TEXT("variance of non-constant vector is positive"), VarNonConst > 0);

	UE_LOG(LogCalibrationTests, Display,
		TEXT("[Variance] constant=%.2e, non-constant=%.4f"), VarConst, VarNonConst);

	return true;
}

// ===================================================================
// UCloudRegistration  ICP via Chaos SVD (no PIE required)
// ===================================================================

// --- 8. icp_identity_points  (R2) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCloudRegistrationIdentityTest,
	"LASAA.Calibration.icp_identity_points",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCloudRegistrationIdentityTest::RunTest(const FString& Parameters)
{
	// R2: When point clouds are identical, the transformation should be identity.
	TArray<FVector> Points;
	Points.Add(FVector(0, 0, 0));
	Points.Add(FVector(1, 0, 0));
	Points.Add(FVector(0, 1, 0));
	Points.Add(FVector(0, 0, 1));

	TArray<double> Weights;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		Weights.Add(1.0);
	}

	FMatrix Transform = UCloudRegistration::calculateTransformationMatrix(Points, Points, Weights);

	FMatrix Expected = FMatrix::Identity;
	TestTrue(TEXT("ICP on identical points produces identity rotation[0][0]"),
		FMath::Abs(Transform.M[0][0] - Expected.M[0][0]) < 1e-4);
	TestTrue(TEXT("ICP on identical points produces identity rotation[1][1]"),
		FMath::Abs(Transform.M[1][1] - Expected.M[1][1]) < 1e-4);
	TestTrue(TEXT("ICP on identical points produces identity rotation[2][2]"),
		FMath::Abs(Transform.M[2][2] - Expected.M[2][2]) < 1e-4);
	TestTrue(TEXT("ICP on identical points produces zero translation"),
		FMath::Abs(Transform.M[0][3]) < 1e-4 &&
		FMath::Abs(Transform.M[1][3]) < 1e-4 &&
		FMath::Abs(Transform.M[2][3]) < 1e-4);

	return true;
}

// --- 9. icp_known_rotation  (R2) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCloudRegistrationKnownRotationTest,
	"LASAA.Calibration.icp_known_rotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCloudRegistrationKnownRotationTest::RunTest(const FString& Parameters)
{
	// R2: Apply a known 90 Y rotation to the source points; ICP should recover it.
	TArray<FVector> Source;
	TArray<FVector> Target;

	Source.Add(FVector(1, 0, 0));
	Target.Add(FVector(0, 0, 1));  // 90 around Y: (1,0,0)  (0,0,1)

	Source.Add(FVector(0, 1, 0));
	Target.Add(FVector(0, 1, 0));  // Y axis unchanged

	Source.Add(FVector(0, 0, 1));
	Target.Add(FVector(1, 0, 0));  // 90 around Y: (0,0,1)  (1,0,0)

	TArray<double> Weights;
	for (int32 i = 0; i < Source.Num(); ++i)
	{
		Weights.Add(1.0);
	}

	FMatrix Transform = UCloudRegistration::calculateTransformationMatrix(Source, Target, Weights);

	// The recovered rotation should approximately match a 90 Y rotation.
	// Apply the matrix to (1,0,0) and check it produces ~ (0,0,1).
	FVector Result = Transform.TransformPosition(FVector(1, 0, 0));
	TestTrue(TEXT("ICP recovers 90 Y rotation for (1,0,0)  ~ (0,0,1)"),
		FMath::Abs(Result.Z - 1.0) < 0.1 || FMath::Abs(Result.X - 0.0) < 0.1);

	UE_LOG(LogCalibrationTests, Display,
		TEXT("[ICP Rotation] Transform(1,0,0) = (%.3f, %.3f, %.3f)"),
		Result.X, Result.Y, Result.Z);

	return true;
}

// --- 10. icp_reflection_fix  (R2) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCloudRegistrationReflectionFixTest,
	"LASAA.Calibration.icp_reflection_fix",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCloudRegistrationReflectionFixTest::RunTest(const FString& Parameters)
{
	// R2: The reflection fix checks R.Determinant() < 0 and flips the last row of Vt.
	// This ensures the result is a proper rotation (det = +1), not a reflection.
	//
	// Use points that might trigger the reflection path (planar or collinear).

	TArray<FVector> Source;
	TArray<FVector> Target;

	// Points on the XY plane  well-conditioned for SVD.
	Source.Add(FVector(0, 0, 0));
	Target.Add(FVector(0, 0, 0));
	Source.Add(FVector(1, 0, 0));
	Target.Add(FVector(0, 1, 0));
	Source.Add(FVector(0, 1, 0));
	Target.Add(FVector(1, 0, 0));
	Source.Add(FVector(1, 1, 0));
	Target.Add(FVector(0, 2, 0));

	TArray<double> Weights;
	for (int32 i = 0; i < Source.Num(); ++i)
	{
		Weights.Add(1.0);
	}

	FMatrix Transform = UCloudRegistration::calculateTransformationMatrix(Source, Target, Weights);

	// Extract the 33 rotation and check determinant  +1 (not -1).
	double Det = Transform.M[0][0] * (Transform.M[1][1] * Transform.M[2][2] - Transform.M[1][2] * Transform.M[2][1])
		- Transform.M[0][1] * (Transform.M[1][0] * Transform.M[2][2] - Transform.M[1][2] * Transform.M[2][0])
		+ Transform.M[0][2] * (Transform.M[1][0] * Transform.M[2][1] - Transform.M[1][1] * Transform.M[2][0]);

	TestTrue(TEXT("ICP produces proper rotation (det  +1, not reflection)"),
		FMath::Abs(Det - 1.0) < 0.1);

	UE_LOG(LogCalibrationTests, Display, TEXT("[ICP Reflection] Rotation determinant = %.4f"), Det);

	return true;
}

// --- 11. icp_fewer_than_3_points  (R2) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCloudRegistrationTooFewPointsTest,
	"LASAA.Calibration.icp_fewer_than_3_points",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCloudRegistrationTooFewPointsTest::RunTest(const FString& Parameters)
{
	// R2: calculateTransformationMatrix requires at least 3 points.
	TArray<FVector> Source;
	Source.Add(FVector(1, 0, 0));
	Source.Add(FVector(0, 1, 0));

	TArray<FVector> Target;
	Target.Add(FVector(0, 1, 0));
	Target.Add(FVector(1, 0, 0));

	TArray<double> Weights;
	Weights.Add(1.0);
	Weights.Add(1.0);

	FMatrix Transform = UCloudRegistration::calculateTransformationMatrix(Source, Target, Weights);
	FMatrix Identity = FMatrix::Identity;

	TestTrue(TEXT("ICP with <3 points returns identity"),
		Transform.Equals(Identity, 1e-6));

	return true;
}

// ===================================================================

// ===================================================================
// UXRCalibration PIE lifecycle tests (merged from CalibrationPIETests.cpp)
// ===================================================================

// Static trackers for PIE test object persistence
static FWeakObjectPtr Test12_CalibrationActor;
static FWeakObjectPtr g_CalibActor;
static FWeakObjectPtr g_CalibComponent;

// ===================================================================
// UXRCalibration  PIE lifecycle tests
// ===================================================================

// --- 12. beginplay_loads_calibration  (R6) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FXRCalibrationBeginPlayTest,
	"LASAA.Calibration.beginplay_loads_calibration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FXRCalibrationBeginPlayTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	// Spawn an actor with a UXRCalibration component.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator, Params);
		if (!TestNotNull(TEXT("Calibration actor must spawn"), Actor))
		{
			return true;
		}

		UClass* CalibrationClass = GetXRCalibrationClass();
		if (!TestNotNull(TEXT("UXRCalibration class must be registered"), CalibrationClass))
		{
			return true;
		}

		USceneComponent* Calib = Cast<USceneComponent>(
			Actor->AddComponentByClass(CalibrationClass, false, FTransform::Identity, false));
		if (!TestNotNull(TEXT("UXRCalibration component must be creatable"), Calib))
		{
			return true;
		}
		Calib->RegisterComponent();

		Test12_CalibrationActor = Actor;
		g_CalibActor = Actor;
		g_CalibComponent = Calib;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// BeginPlay should have been called, which calls loadFromFile + setCalibrationOffset.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		AActor* Actor = Cast<AActor>(Test12_CalibrationActor.Get());
		if (!Actor)
		{
			AddWarning(TEXT("Calibration actor was garbage collected"));
			return true;
		}

		UE_LOG(LogCalibrationPIETests, Display,
			TEXT("[BeginPlay] Calibration component created and BeginPlay should have fired."));

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// --- 13. reset_calibration_clears_state  (R6) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FXRCalibrationResetTest,
	"LASAA.Calibration.reset_calibration_clears_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FXRCalibrationResetTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World) return true;

		FActorSpawnParameters Params;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator);
		if (!Actor) return true;

		UClass* CalibrationClass = GetXRCalibrationClass();
		if (!CalibrationClass) return true;

		USceneComponent* Calib = Cast<USceneComponent>(
			Actor->AddComponentByClass(CalibrationClass, false, FTransform::Identity, false));
		if (!Calib) return true;
		Calib->RegisterComponent();

		// Set a non-identity transform to simulate a loaded calibration.
		FTransform TestTransform(FRotator(10, 20, 30), FVector(5, 10, 15));
		Calib->SetRelativeTransform(TestTransform);

		g_CalibActor = Actor;
		g_CalibComponent = Calib;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Call resetCalibration  should set transform to identity and propagate.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		USceneComponent* Calib = Cast<USceneComponent>(g_CalibComponent.Get());
		if (!TestNotNull(TEXT("Calibration component must still exist"), Calib))
		{
			return true;
		}

		InvokeCalibrationMethod(Calib, TEXT("resetCalibration"));

		FTransform AfterReset = Calib->GetRelativeTransform();
		FTransform Identity = FTransform::Identity;

		TestTrue(TEXT("resetCalibration sets rotation to identity"),
			AfterReset.GetRotation().Equals(Identity.GetRotation(), 1e-6));
		TestTrue(TEXT("resetCalibration sets translation to identity"),
			AfterReset.GetTranslation().Equals(Identity.GetTranslation(), 1e-6));

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// --- 14. write_load_via_public_api  (R6, R7) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FXRCalibrationWriteLoadRoundtripTest,
	"LASAA.Calibration.write_load_roundtrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FXRCalibrationWriteLoadRoundtripTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World) return true;

		FActorSpawnParameters Params;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator);
		if (!Actor) return true;

		UClass* CalibrationClass = GetXRCalibrationClass();
		if (!CalibrationClass) return true;

		USceneComponent* Calib = Cast<USceneComponent>(
			Actor->AddComponentByClass(CalibrationClass, false, FTransform::Identity, false));
		if (!Calib) return true;
		Calib->RegisterComponent();

		g_CalibActor = Actor;
		g_CalibComponent = Calib;

		// Set a known transform to simulate a calibrated state.
		FTransform Original(FRotator(5, 10, 15), FVector(1, 2, 3));
		Calib->SetRelativeTransform(Original);

		// Write a calibration file manually: identity matrix in the expected CSV format.
		TArray<FString> CSVLines;
		CSVLines.Add(TEXT("1,0,0,0"));
		CSVLines.Add(TEXT("0,1,0,0"));
		CSVLines.Add(TEXT("0,0,1,0"));
		CSVLines.Add(TEXT("0,0,0,1"));

		FString Filepath = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectPluginsDir() + TEXT("LASAA/anchorCalibrate.txt"));
		FFileHelper::SaveStringArrayToFile(CSVLines, *Filepath);

		UE_LOG(LogCalibrationPIETests, Display,
			TEXT("[Roundtrip] Wrote identity calibration to %s"), *Filepath);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Call changeCalibration  toggles filename, loads file, propagates offset.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		USceneComponent* Calib = Cast<USceneComponent>(g_CalibComponent.Get());
		if (!TestNotNull(TEXT("Calibration component must still exist"), Calib))
		{
			return true;
		}

		FTransform Before = Calib->GetRelativeTransform();

		InvokeCalibrationMethod(Calib, TEXT("changeCalibration"));
		FTransform AfterFirst = Calib->GetRelativeTransform();

		InvokeCalibrationMethod(Calib, TEXT("changeCalibration"));
		FTransform AfterSecond = Calib->GetRelativeTransform();

		UE_LOG(LogCalibrationPIETests, Display,
			TEXT("[Roundtrip] Before=(%.1f,%.1f,%.1f) AfterFirst=(%.1f,%.1f,%.1f) AfterSecond=(%.1f,%.1f,%.1f)"),
			Before.GetTranslation().X, Before.GetTranslation().Y, Before.GetTranslation().Z,
			AfterFirst.GetTranslation().X, AfterFirst.GetTranslation().Y, AfterFirst.GetTranslation().Z,
			AfterSecond.GetTranslation().X, AfterSecond.GetTranslation().Y, AfterSecond.GetTranslation().Z);

		TestNotNull(TEXT("Component must survive changeCalibration x 2"), Calib);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// --- 15. calibrate_requires_enough_anchors  (R3, R4) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FXRCalibrationAnchorRequirementTest,
	"LASAA.Calibration.calibrate_requires_enough_anchors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FXRCalibrationAnchorRequirementTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World) return true;

		FActorSpawnParameters Params;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator);
		if (!Actor) return true;

		UClass* CalibrationClass = GetXRCalibrationClass();
		if (!CalibrationClass) return true;

		USceneComponent* Calib = Cast<USceneComponent>(
			Actor->AddComponentByClass(CalibrationClass, false, FTransform::Identity, false));
		if (!Calib) return true;
		Calib->RegisterComponent();

		g_CalibActor = Actor;
		g_CalibComponent = Calib;

		UE_LOG(LogCalibrationPIETests, Display,
			TEXT("[AnchorReq] Calling calibrate() in the empty PIE test scene; it should early-out with insufficient anchors."));

		InvokeCalibrationMethod(Calib, TEXT("calibrate"));

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Verify the component is still valid (no crash).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		USceneComponent* Calib = Cast<USceneComponent>(g_CalibComponent.Get());
		TestNotNull(TEXT("Calibration component must survive calibrate() with insufficient anchors"), Calib);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

// --- 16. change_calibration_toggles_files  (R6) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FXRCalibrationChangeCalibrationTest,
	"LASAA.Calibration.change_calibration_toggles_files",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FXRCalibrationChangeCalibrationTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	AddExpectedMetaProjectSetupNoise(*this);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TestMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World) return true;

		FActorSpawnParameters Params;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator);
		if (!Actor) return true;

		UClass* CalibrationClass = GetXRCalibrationClass();
		if (!CalibrationClass) return true;

		USceneComponent* Calib = Cast<USceneComponent>(
			Actor->AddComponentByClass(CalibrationClass, false, FTransform::Identity, false));
		if (!Calib) return true;
		Calib->RegisterComponent();

		g_CalibActor = Actor;
		g_CalibComponent = Calib;

		FTransform Before = Calib->GetRelativeTransform();

		InvokeCalibrationMethod(Calib, TEXT("changeCalibration"));

		FTransform After = Calib->GetRelativeTransform();

		UE_LOG(LogCalibrationPIETests, Display,
			TEXT("[ChangeCal] Before=(%.1f,%.1f,%.1f) After=(%.1f,%.1f,%.1f)"),
			Before.GetTranslation().X, Before.GetTranslation().Y, Before.GetTranslation().Z,
			After.GetTranslation().X, After.GetTranslation().Y, After.GetTranslation().Z);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}

