#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "Rig/IKRigSkeleton.h" // full FIKRigSkeleton definition (constructed below)
#include "IKRig_ConstraintBones.h"

// Test-only access to the protected UIKRigSolver overrides + the protected ConstraintBones
// member (no production code is modified). Standard explicit-instantiation idiom — NOT
// `#define protected public`, which mis-mangles protected method calls and fails to link
// on MSVC (see project-test-authoring-infra).
namespace RigAccess
{
	template <class Tag> struct TBag { static typename Tag::Type Ptr; };
	template <class Tag> typename Tag::Type TBag<Tag>::Ptr{};
	template <class Tag, typename Tag::Type P> struct TPick
	{
		struct FInit { FInit() { TBag<Tag>::Ptr = P; } };
		static FInit Init;
	};
	template <class Tag, typename Tag::Type P> typename TPick<Tag, P>::FInit TPick<Tag, P>::Init;
}
#define RIG_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace RigAccess { struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &UIKRig_ConstraintBones::MEMBER>; }

RIG_STEAL(FConstraintsTag,  TArray<FConstraintBone> UIKRig_ConstraintBones::*,                          ConstraintBones)
RIG_STEAL(FGoalTag,         bool (UIKRig_ConstraintBones::*)(const FName&) const,                        IsGoalConnected)
RIG_STEAL(FAffectedTag,     bool (UIKRig_ConstraintBones::*)(const FName&, const FIKRigSkeleton&) const, IsBoneAffectedBySolver)
RIG_STEAL(FWarningTag,      bool (UIKRig_ConstraintBones::*)(FText&) const,                              GetWarningMessage)

namespace RigTools
{
	static bool IsGoalConnected(const UIKRig_ConstraintBones* S, const FName& Goal)
	{
		return (S->*RigAccess::TBag<RigAccess::FGoalTag>::Ptr)(Goal);
	}
	static bool IsBoneAffected(const UIKRig_ConstraintBones* S, const FName& Bone, const FIKRigSkeleton& Sk)
	{
		return (S->*RigAccess::TBag<RigAccess::FAffectedTag>::Ptr)(Bone, Sk);
	}
	static bool GetWarning(const UIKRig_ConstraintBones* S, FText& Out)
	{
		return (S->*RigAccess::TBag<RigAccess::FWarningTag>::Ptr)(Out);
	}
	static TArray<FConstraintBone>& Constraints(UIKRig_ConstraintBones* S)
	{
		return S->*RigAccess::TBag<RigAccess::FConstraintsTag>::Ptr;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0069 (TTToolbox Rig Tools — IKRig constraint solver).
//
// EDITABLE FILES UNDER TEST: TTToolboxBlueprintLibrary.cpp/.h and IKRig_ConstraintBones
// .cpp/.h. In start/ the solver methods are stubs:
//   IsGoalConnected()          → return false
//   IsBoneAffectedBySolver()   → return false
//   GetWarningMessage()        → always returns true with "unavailable in this build"
// so the solver claims no goals, affects no bones, and is permanently in a warning state.
// The solution implements the real solver model: goals are reported connected, a bone is
// affected iff it is a configured constraint's ModifiedBone, and the warning is raised
// only when there are no configured constraints.
//
// OBSERVABLE CONTRACT (pure, CPU-only — the IKRig constraint solver operates on its own
// ConstraintBones data and does not require a real skeleton for these predicates):
//   • bool IsGoalConnected(FName)
//   • bool IsBoneAffectedBySolver(FName, FIKRigSkeleton)
//   • bool GetWarningMessage(FText&)
// These are protected UIKRigSolver overrides, reached via the access idiom; ConstraintBones
// is a protected UPROPERTY populated the same way the editor inspector would.
//
// The earlier suite asserted member-function-pointer existence and the constructor's empty
// ConstraintBones (structural; the start stubs also pass) and is dropped.
//
// NOT COVERED, and why: the USkeleton/anim-asset editor helpers (AddSocket,
// AddSkeletonCurve, DumpVirtualBones, …) mutate real skeleton assets through the editor
// and need authored skeleton content that is not available headlessly; the solver
// predicates are the deterministic, content-free discriminators for this task.
// ─────────────────────────────────────────────────────────────────────────────

// ── REQUIRED: the solver reports its goal connection (start stub always denies it) ──
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTToolboxSolverGoalConnectedTest,
	"TTToolbox.RigTools.solver_reports_goal_connected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTTToolboxSolverGoalConnectedTest::RunTest(const FString&)
{
	UIKRig_ConstraintBones* Solver = NewObject<UIKRig_ConstraintBones>(GetTransientPackage());
	if (!TestNotNull(TEXT("IKRig constraint solver must be constructible"), Solver)) return true;

	TestTrue(TEXT("The constraint solver must report its goal as connected (start stub denies it)"),
		RigTools::IsGoalConnected(Solver, FName(TEXT("AnyGoal"))));
	return true;
}

// ── REQUIRED: a configured constraint's ModifiedBone is affected by the solver ──────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTToolboxSolverBoneAffectedTest,
	"TTToolbox.RigTools.solver_affects_configured_constraint_bone",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTTToolboxSolverBoneAffectedTest::RunTest(const FString&)
{
	UIKRig_ConstraintBones* Solver = NewObject<UIKRig_ConstraintBones>(GetTransientPackage());
	if (!TestNotNull(TEXT("IKRig constraint solver must be constructible"), Solver)) return true;

	FConstraintBone Entry;
	Entry.ConstraintBone = FName(TEXT("root"));
	Entry.ModifiedBone = FName(TEXT("pelvis"));
	RigTools::Constraints(Solver).Add(Entry);

	const FIKRigSkeleton EmptySkeleton; // the predicate inspects ConstraintBones, not the skeleton

	TestTrue(TEXT("A bone configured as a constraint's ModifiedBone must be affected by the solver"),
		RigTools::IsBoneAffected(Solver, FName(TEXT("pelvis")), EmptySkeleton));
	// Negative control: a bone not in the constraint list must NOT be reported as affected.
	TestFalse(TEXT("A bone not in the constraint list must not be affected"),
		RigTools::IsBoneAffected(Solver, FName(TEXT("hand_l")), EmptySkeleton));
	return true;
}

// ── REQUIRED: the warning reflects the real constraint state (fail-safe reporting) ──
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTToolboxSolverWarningStateTest,
	"TTToolbox.RigTools.solver_warning_reflects_constraint_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTTToolboxSolverWarningStateTest::RunTest(const FString&)
{
	UIKRig_ConstraintBones* Solver = NewObject<UIKRig_ConstraintBones>(GetTransientPackage());
	if (!TestNotNull(TEXT("IKRig constraint solver must be constructible"), Solver)) return true;

	// With at least one constraint configured, the solver must NOT raise a warning.
	FConstraintBone Entry;
	Entry.ConstraintBone = FName(TEXT("root"));
	Entry.ModifiedBone = FName(TEXT("pelvis"));
	RigTools::Constraints(Solver).Add(Entry);

	FText Warning;
	const bool bHasWarning = RigTools::GetWarning(Solver, Warning);

	TestFalse(TEXT("A solver with configured constraints must not report a warning (start stub always warns)"),
		bHasWarning);
	return true;
}
