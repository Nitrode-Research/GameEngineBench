// The MIT License (MIT)
// ---------------------
//
// Copyright 2022 Achim Turan (https://www.instagram.com/tuatec/)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "TTToolboxBlueprintLibrary.h"

bool UTTToolboxBlueprintLibrary::DumpVirtualBones(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddVirtualBone(
	const FName& VirtualBoneName,
	const FName& SourceBoneName,
	const FName& TargetBoneName,
	USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::DumpSockets(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddSocket(
	const FName& BoneName,
	const FName& SocketName,
	const FTransform& RelativeTransform,
	USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::HasSocket(const FName& SocketName, USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::DumpSkeletonCurveNames(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::CheckForMissingCurveNames(const TArray<FName>& CurveNamesToCheck, USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::HasSkeletonCurve(USkeleton* Skeleton, const FName& SkeletonCurveName)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::DumpSkeletonBlendProfile(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddSkeletonBlendProfile(
	USkeleton* Skeleton,
	const FName& BlendProfileName,
	const FTTBlendProfile_BP& BlendProfile,
	bool Overwrite)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddSkeletonCurve(USkeleton* Skeleton, const FName& SkeletonCurveName)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddSkeletonSlotGroup(USkeleton* Skeleton, const FTTMontageSlotGroup& SlotGroup)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::DumpGroupsAndSlots(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddUnweightedBone(const TArray<FTTNewBone_BP>& NewBones, USkeleton* Skeleton)
{
	return false;
}

void UTTToolboxBlueprintLibrary::RequestAnimationRecompress(USkeleton* Skeleton)
{
}

void UTTToolboxBlueprintLibrary::RequestAnimSequencesRecompression(TArray<UAnimSequence*> AnimSequences)
{
}

bool UTTToolboxBlueprintLibrary::SetAnimSequenceInterpolation(UAnimSequence* AnimSequence, EAnimInterpolationType AnimInterpolationType)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::ConstraintBonesForSkeletonPose(
	const TArray<FTTConstraintBone_BP>& ConstraintBones,
	USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddRootBone(USkeleton* Skeleton)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::UpdateControlRigBlueprintPreviewMesh(
	UControlRigBlueprint* ControlRigBlueprint,
	USkeletalMesh* SkeletalMesh)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::CopyAnimMontageCurves(UAnimMontage* SourceAnimMontage, UAnimMontage* TargetAnimMontage)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::DumpIKChains(const UIKRigDefinition* IKRigDefinition)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::AddIKBoneChains(UIKRigDefinition* IKRigDefinition, const TArray<FBoneChain_BP>& BoneChains)
{
	return false;
}

bool UTTToolboxBlueprintLibrary::SetIKBoneChainGoal(
	UIKRigDefinition* IKRigDefinition,
	const FName& ChainName,
	const FName& GoalName)
{
	return false;
}
