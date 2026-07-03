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

#include "IKRig_ConstraintBones.h"
#include "Rig/IKRigSkeleton.h" // UE5.7: FIKRigSkeleton full definition (was transitively available via the old IKRigSolver.h)

#define LOCTEXT_NAMESPACE "UIKRig_BoneConstrainer"

UIKRig_ConstraintBones::UIKRig_ConstraintBones() {}
UIKRig_ConstraintBones::~UIKRig_ConstraintBones() {}

void UIKRig_ConstraintBones::Initialize(const FIKRigSkeleton& IKRigSkeleton)
{
	m_constraintBones.Reset();
}

void UIKRig_ConstraintBones::Solve(FIKRigSkeleton& IKRigSkeleton, const FIKRigGoalContainer& Goals)
{
}

#if WITH_EDITOR

FText UIKRig_ConstraintBones::GetNiceName() const
{
	return FText(LOCTEXT("SolverName", "Constraint Bones"));
}

bool UIKRig_ConstraintBones::IsBoneAffectedBySolver(const FName& BoneName, const FIKRigSkeleton& IKRigSkeleton) const
{
	return false;
}

bool UIKRig_ConstraintBones::GetWarningMessage(FText& OutWarningMessage) const
{
	OutWarningMessage = LOCTEXT("ConstraintBonesDisabled", "Constraint bones are unavailable in this build.");
	return true;
}

bool UIKRig_ConstraintBones::IsGoalConnected(const FName& GoalName) const
{
	return false;
}

#endif

#undef LOCTEXT_NAMESPACE
