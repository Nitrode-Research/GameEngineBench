// Copyright (C) 2026 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Modifiers/AlsxtAnimationModifier_ExtractRecoilCurves.h"

float UAlsxtAnimationModifier_ExtractRecoilCurves::NormalizeValue(float Value, float MaxValue) const
{
	return 0.f;
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::AddCurveKey(TArray<FRichCurveKey>& Keys, float Time, float Value)
{
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::AddCurve(UAnimSequence* Animation, FName Name, const TArray<FRichCurveKey>& Keys)
{
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::RemoveCurve(UAnimSequence* Animation, FName Name)
{
}

FName UAlsxtAnimationModifier_ExtractRecoilCurves::GetCurveName(FString Type, FString Axis) const
{
	return NAME_None;
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::ExtractAnimCurves(UAnimSequence* AnimationSequence)
{
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::ExtractCurveAssets(UAnimSequence* AnimationSequence)
{
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnApply_Implementation(AnimationSequence);
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnRevert_Implementation(AnimationSequence);
}

FTransform UAlsxtAnimationModifier_ExtractRecoilCurves::ExtractPose(UAnimSequence* Animation, const FBoneContainer& BoneContainer, FCompactPoseBoneIndex CompactPoseBoneIndex, double Time)
{
	return FTransform::Identity;
}

void UAlsxtAnimationModifier_ExtractRecoilCurves::SaveCurve(UPackage* Package, UCurveVector* Curve, const FString& PackagePath)
{
}
