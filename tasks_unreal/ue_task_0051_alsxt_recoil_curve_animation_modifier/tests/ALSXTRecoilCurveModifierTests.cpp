// Copyright 2026 GameDevBench. ALSXT recoil curve animation modifier source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ALSXTRecoilCurveModifierTests
{
	static bool LoadModifierSource(FString& OutSource)
	{
		const FString SourcePath = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("Plugins/ALSXT/Source/ALSXTEditor/Private/Modifiers/AlsxtAnimationModifier_ExtractRecoilCurves.cpp"));
		return FFileHelper::LoadFileToString(OutSource, *SourcePath);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTRecoilCurveModifier_ValidationAndSamplingSource,
	"TargetVector.ALSXTRecoilCurveModifier.validation_and_sampling_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTRecoilCurveModifier_ValidationAndSamplingSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates input checks and the Unreal animation pose sampling path.
	FString Source;
	if (!TestTrue(TEXT("ALSXT recoil modifier source must be readable"), ALSXTRecoilCurveModifierTests::LoadModifierSource(Source)))
	{
		return true;
	}

	TestTrue(TEXT("Modifier must reject invalid animation sequences"),
		Source.Contains(TEXT("Invalid Animation")));
	TestTrue(TEXT("Modifier must reject invalid skeletons"),
		Source.Contains(TEXT("GetSkeleton()")) && Source.Contains(TEXT("invalid Skeleton")));
	TestTrue(TEXT("Modifier must validate the configured bone name"),
		Source.Contains(TEXT("FindBoneIndex(BoneName)")) && Source.Contains(TEXT("Invalid Bone Index")));
	TestTrue(TEXT("Modifier must build required bones and compact pose indices"),
		Source.Contains(TEXT("RequiredBones")) && Source.Contains(TEXT("FBoneContainer")) && Source.Contains(TEXT("MakeCompactPoseIndex")));
	TestTrue(TEXT("Modifier must sample over animation length using AnimationSampleRate"),
		Source.Contains(TEXT("GetPlayLength()")) && Source.Contains(TEXT("1.f / AnimationSampleRate")) && Source.Contains(TEXT("while (Time < AnimLength)")));
	TestTrue(TEXT("Modifier must extract Unreal animation pose data"),
		Source.Contains(TEXT("FAnimationPoseData")) && Source.Contains(TEXT("GetBonePose")) && Source.Contains(TEXT("FCSPose<FCompactPose>")));
	TestTrue(TEXT("Modifier must compute deltas from current pose and reference pose"),
		Source.Contains(TEXT("CurrentPose")) && Source.Contains(TEXT("RefPose")) && Source.Contains(TEXT("DeltaPose")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTRecoilCurveModifier_NormalizationAndAnimCurveSource,
	"TargetVector.ALSXTRecoilCurveModifier.normalization_and_anim_curve_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTRecoilCurveModifier_NormalizationAndAnimCurveSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates normalization, animation-curve creation/update, naming, and revert cleanup.
	FString Source;
	if (!TestTrue(TEXT("ALSXT recoil modifier source must be readable"), ALSXTRecoilCurveModifierTests::LoadModifierSource(Source)))
	{
		return true;
	}

	TestTrue(TEXT("NormalizeValue must guard near-zero maxima"),
		Source.Contains(TEXT("FMath::IsNearlyZero(MaxValue)")) && Source.Contains(TEXT("return 0.f")));
	TestTrue(TEXT("Normalization must track max translation and rotation magnitudes"),
		Source.Contains(TEXT("MaxLoc")) && Source.Contains(TEXT("MaxRot")) && Source.Contains(TEXT("FMath::Abs")));
	TestTrue(TEXT("Normalization must store sampled poses and rewrite curve keys"),
		Source.Contains(TEXT("BonePoses.Add")) && Source.Contains(TEXT("NormalizeValue")) && Source.Contains(TEXT("CurveTX[i].Value")));
	TestTrue(TEXT("Animation curve mode must use the animation data controller"),
		Source.Contains(TEXT("GetController()")) && Source.Contains(TEXT("SetCurveKeys")) && Source.Contains(TEXT("AddCurve(CurveId)")));
	TestTrue(TEXT("Curve names must combine CurveName, type, and axis tokens"),
		Source.Contains(TEXT("GetCurveName")) && Source.Contains(TEXT("%s_%s_%s")));
	TestTrue(TEXT("Animation curve mode must write all translation and rotation axis curves"),
		Source.Contains(TEXT("GetCurveName(TEXT(\"T\"), TEXT(\"X\"))"))
		&& Source.Contains(TEXT("GetCurveName(TEXT(\"T\"), TEXT(\"Y\"))"))
		&& Source.Contains(TEXT("GetCurveName(TEXT(\"T\"), TEXT(\"Z\"))"))
		&& Source.Contains(TEXT("GetCurveName(TEXT(\"R\"), TEXT(\"R\"))"))
		&& Source.Contains(TEXT("GetCurveName(TEXT(\"R\"), TEXT(\"P\"))"))
		&& Source.Contains(TEXT("GetCurveName(TEXT(\"R\"), TEXT(\"Y\"))")));
	TestTrue(TEXT("Revert must remove the generated translation and rotation curves"),
		Source.Contains(TEXT("OnRevert_Implementation")) && Source.Contains(TEXT("RemoveCurve(AnimationSequence")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTRecoilCurveModifier_AssetExportSource,
	"TargetVector.ALSXTRecoilCurveModifier.asset_export_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTRecoilCurveModifier_AssetExportSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates standalone UCurveVector asset export and package saving behavior.
	FString Source;
	if (!TestTrue(TEXT("ALSXT recoil modifier source must be readable"), ALSXTRecoilCurveModifierTests::LoadModifierSource(Source)))
	{
		return true;
	}

	TestTrue(TEXT("Standalone export must use plugin settings for the curve save path"),
		Source.Contains(TEXT("GetMutableDefault<UAlsxtAnimationModifierExtractCurvesSettings>")) && Source.Contains(TEXT("CurvesSavePath")));
	TestTrue(TEXT("Standalone export must create separate Loc and Rot packages"),
		Source.Contains(TEXT("CreatePackage")) && Source.Contains(TEXT("Loc")) && Source.Contains(TEXT("Rot")));
	TestTrue(TEXT("Standalone export must create UCurveVector assets"),
		Source.Contains(TEXT("NewObject<UCurveVector>")));
	TestTrue(TEXT("Standalone export must write translation and rotation float curves"),
		Source.Contains(TEXT("TranslationCurve->FloatCurves")) && Source.Contains(TEXT("RotationCurve->FloatCurves")));
	TestTrue(TEXT("SaveCurve must register the asset with the asset registry"),
		Source.Contains(TEXT("FAssetRegistryModule::AssetCreated")));
	TestTrue(TEXT("SaveCurve must convert the long package name and save the package"),
		Source.Contains(TEXT("LongPackageNameToFilename")) && Source.Contains(TEXT("UPackage::SavePackage")));
	TestTrue(TEXT("SaveCurve must mark the package dirty after saving"),
		Source.Contains(TEXT("SetDirtyFlag(true)")));
	return true;
}
