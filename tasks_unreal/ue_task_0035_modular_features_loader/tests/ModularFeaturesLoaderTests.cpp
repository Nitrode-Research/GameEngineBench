// Copyright (c) 2026 GameDevBench. Modular Game Features Loader Subsystem automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/Subsystems/BmrModularGameFeaturesLoaderSubsystem.cpp
//
// The subsystem is the central authority for activating / deactivating
// Modular Game Feature plugins based on gameplay tags on each world's Ability
// System Component. It is a UEngineSubsystem — one instance per engine,
// surviving every map switch and every PIE session. The flow is:
//
//   1. Each world's ASC, when ready, broadcasts the global message
//      WorldASC_Ready with itself as OptionalObject.
//   2. The subsystem snapshots that ASC's owned tags into AscOwnedTags and
//      registers a per-tag listener for every tag any modular feature names.
//   3. When a tag count changes, the listener updates the snapshot and
//      queues a deferred recompute (next-tick ticker).
//   4. The recompute aggregates tags across authoritative ASCs and diffs
//      against ActiveFeatures to produce activate/deactivate batches.
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface would require booting at least
//   two ASC-bearing worlds (editor + PIE) inside an automation test, wiring
//   up UBmrModularGameFeatureSettings with real tag-feature rows, and
//   driving FEditorDelegates::PreBeginPIE/EndPIE/ShutdownPIE — every one of
//   which fires only inside a real editor process and not in a headless
//   automation harness. The runtime side of the subsystem reaches
//   UModularGameFeaturePluginUtils::SetModularGameFeaturesActive /
//   RestoreGameFeatureTargetState, which checkf-assert against the
//   GameFeaturesSubsystem state and crash a headless test process when
//   the plugins aren't registered. The dispatch this task tests is the
//   choice between several engine API calls (RestoreGameFeatureTargetState
//   vs SetModularGameFeaturesActive(false), QueueDeferredRecompute vs
//   synchronous apply, IsValid-guarded ticker vs unguarded) — and those
//   choices are exactly what the source-inspection assertions below pin
//   down. The required-behavior checklist (B1..B7 in the task brief) maps
//   to a small set of code-path discriminators; the tests gate on the
//   CODE shape of those discriminators so a re-impl that emits the wrong
//   destruction path, drops the no-authority guard, or forgets one of the
//   three editor PIE delegates fails outright.

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogModularFeaturesLoaderTests, Log, All);

namespace ModularFeaturesLoaderTests
{
	// The stripped file lives in the Bomber game module (not a plugin), so
	// it is always available on disk during a test run. The header is
	// unchanged in this task; the .cpp is what was gutted.
	static const TCHAR* LoaderSubsystemRelPath =
		TEXT("Source/Bomber/Private/Subsystems/BmrModularGameFeaturesLoaderSubsystem.cpp");

	static FString LoadProjectFile(FAutomationTestBase* Test, const TCHAR* RelativePath)
	{
		const FString AbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelativePath);
		FString Source;
		if (!Test->TestTrue(
				FString::Printf(TEXT("Source file must be readable at %s"), *AbsPath),
				FFileHelper::LoadFileToString(Source, *AbsPath)))
		{
			return FString();
		}
		return Source;
	}

	// Strips C++ line and block comments from source text so a substring
	// scan does not match anti-pattern descriptions written inside
	// comments. Both the canonical solution and many plausible re-impls
	// reference "don't apply synchronously" / "no SetModularGameFeaturesActive
	// on map switch" in comments; without comment-stripping a re-impl could
	// satisfy a positive assertion just by quoting the spec. String literals
	// are preserved so any TEXT("…") payload in real code still appears.
	static FString StripComments(const FString& Source)
	{
		FString Out;
		Out.Reserve(Source.Len());
		bool bInLineComment = false;
		bool bInBlockComment = false;
		bool bInString = false;
		bool bInChar = false;
		for (int32 i = 0; i < Source.Len(); ++i)
		{
			const TCHAR C = Source[i];
			const TCHAR N = (i + 1 < Source.Len()) ? Source[i + 1] : TCHAR('\0');
			if (bInLineComment)
			{
				if (C == TCHAR('\n'))
				{
					bInLineComment = false;
					Out.AppendChar(C);
				}
				continue;
			}
			if (bInBlockComment)
			{
				if (C == TCHAR('*') && N == TCHAR('/'))
				{
					bInBlockComment = false;
					++i;
				}
				continue;
			}
			if (bInString)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++i;
				}
				else if (C == TCHAR('"'))
				{
					bInString = false;
				}
				continue;
			}
			if (bInChar)
			{
				Out.AppendChar(C);
				if (C == TCHAR('\\') && N != TCHAR('\0'))
				{
					Out.AppendChar(N);
					++i;
				}
				else if (C == TCHAR('\''))
				{
					bInChar = false;
				}
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('/'))
			{
				bInLineComment = true;
				++i;
				continue;
			}
			if (C == TCHAR('/') && N == TCHAR('*'))
			{
				bInBlockComment = true;
				++i;
				continue;
			}
			if (C == TCHAR('"'))
			{
				bInString = true;
				Out.AppendChar(C);
				continue;
			}
			if (C == TCHAR('\''))
			{
				bInChar = true;
				Out.AppendChar(C);
				continue;
			}
			Out.AppendChar(C);
		}
		return Out;
	}

	// Brace-depth walker to extract the body of a member-function
	// definition. Counts braces only, not parens, so nested lambdas /
	// initialiser lists (e.g. AddTicker's [this](float){ ... } captures)
	// don't confuse the scan.
	static FString ExtractMemberBody(const FString& Source, const FString& Signature)
	{
		const int32 DefIdx = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (DefIdx == INDEX_NONE)
		{
			return FString();
		}
		const int32 BodyStart = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DefIdx);
		if (BodyStart == INDEX_NONE)
		{
			return FString();
		}
		int32 Depth = 0;
		for (int32 i = BodyStart; i < Source.Len(); ++i)
		{
			const TCHAR C = Source[i];
			if (C == TCHAR('{'))
			{
				++Depth;
			}
			else if (C == TCHAR('}'))
			{
				--Depth;
				if (Depth == 0)
				{
					return Source.Mid(BodyStart, i - BodyStart + 1);
				}
			}
		}
		return FString();
	}
} // namespace ModularFeaturesLoaderTests

// ---------------------------------------------------------------------------
// Test 1 — B1: OnPreBeginPIE and OnEndPIE set bIsAuthorityTransitioning = true.
//
// The spec: "PreBeginPIE and EndPIE set the flag (PIE world is taking or
// releasing authority)." The flag freezes the diff-apply path while the
// editor world and the PIE world are both alive — the editor world's ASC
// must not drive the diff while the PIE world is taking over, and vice
// versa on exit. A re-impl that clears the flag (or only sets it on one
// of the two delegates) lets the wrong-authority ASC drive a recompute
// during the transition window and tears down PIE-active features.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_PreBeginAndEndPIESetAuthorityFlag,
	"Bomber.ModularFeaturesLoader.PreBeginAndEndPIESetAuthorityFlag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_PreBeginAndEndPIESetAuthorityFlag::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString PreBeginBody =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::OnPreBeginPIE"));
	if (!TestTrue(
			TEXT("B1: BmrModularGameFeaturesLoaderSubsystem.cpp must define OnPreBeginPIE with a "
				 "body — the start/ stub is empty; the authority-transitioning hold is what was "
				 "stripped."),
			!PreBeginBody.IsEmpty()))
	{
		return false;
	}

	const FString PreBeginCode = StripComments(PreBeginBody);

	TestTrue(
		TEXT("B1: OnPreBeginPIE must set bIsAuthorityTransitioning = true. The spec is explicit: "
			 "'PreBeginPIE sets the flag (PIE world is taking authority)'. Without the set, the "
			 "editor-world ASC keeps driving the diff while PIE is spinning up and tears down "
			 "features the PIE world expects to find already-active."),
		PreBeginCode.Contains(TEXT("bIsAuthorityTransitioning = true"), ESearchCase::CaseSensitive)
			|| PreBeginCode.Contains(TEXT("bIsAuthorityTransitioning=true"), ESearchCase::CaseSensitive));

	const FString EndBody =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::OnEndPIE"));
	if (!TestTrue(
			TEXT("B1: BmrModularGameFeaturesLoaderSubsystem.cpp must define OnEndPIE with a body."),
			!EndBody.IsEmpty()))
	{
		return false;
	}

	const FString EndCode = StripComments(EndBody);

	TestTrue(
		TEXT("B1: OnEndPIE must also set bIsAuthorityTransitioning = true. The spec: 'EndPIE sets "
			 "the flag (PIE world is releasing authority)'. The symmetric set freezes the diff "
			 "while the PIE world is tearing down so the editor-world ASC does not snap back in "
			 "and aggressively re-diff against a half-destroyed PIE state. The flag is released "
			 "by the first authoritative tag event after the transition, not by EndPIE."),
		EndCode.Contains(TEXT("bIsAuthorityTransitioning = true"), ESearchCase::CaseSensitive)
			|| EndCode.Contains(TEXT("bIsAuthorityTransitioning=true"), ESearchCase::CaseSensitive));

	// The Initialize body must bind BOTH delegates. Setting the flag in the
	// handler is useless if Initialize never wires the handler up to the
	// engine's editor delegate.
	const FString InitBody =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::Initialize"));
	if (!TestTrue(
			TEXT("B1: BmrModularGameFeaturesLoaderSubsystem.cpp must define Initialize with a body."),
			!InitBody.IsEmpty()))
	{
		return false;
	}

	const FString InitCode = StripComments(InitBody);

	TestTrue(
		TEXT("B1: Initialize must bind FEditorDelegates::PreBeginPIE — without the binding the "
			 "handler never fires and the authority-transitioning hold never engages on PIE entry."),
		InitCode.Contains(TEXT("FEditorDelegates::PreBeginPIE"), ESearchCase::CaseSensitive)
			&& InitCode.Contains(TEXT("OnPreBeginPIE"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B1: Initialize must bind FEditorDelegates::EndPIE — without the binding the "
			 "authority-transitioning hold never engages on PIE exit and the editor world's ASC "
			 "drives a recompute against a half-destroyed PIE world."),
		InitCode.Contains(TEXT("FEditorDelegates::EndPIE"), ESearchCase::CaseSensitive)
			&& InitCode.Contains(TEXT("OnEndPIE"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B2: OnEditorShutdownPIE is an emergency restore. It must clear
// bIsAuthorityTransitioning, restore the tag-driven features to their
// .uplugin baseline (RestoreGameFeatureTargetState), reset ActiveFeatures,
// and queue a recompute.
//
// The spec: "ShutdownPIE is an emergency restore: clear the flag, restore
// baseline, queue a recompute." The api_surface pins the first lines of
// this method exactly. A re-impl that only clears the flag (without
// restoring baseline) leaves PIE-state features ACTIVE in the editor
// world; a re-impl that uses SetModularGameFeaturesActive(false) here
// conflates the two destruction paths (B6) and tears down features the
// engine activated at boot via .uplugin BuiltInInitialFeatureState.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_ShutdownPIEEmergencyRestore,
	"Bomber.ModularFeaturesLoader.ShutdownPIEEmergencyRestore",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_ShutdownPIEEmergencyRestore::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::OnEditorShutdownPIE"));
	if (!TestTrue(
			TEXT("B2: BmrModularGameFeaturesLoaderSubsystem.cpp must define OnEditorShutdownPIE "
				 "with a body — the start/ stub is empty; the emergency-restore sequence is what "
				 "was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B2: OnEditorShutdownPIE must clear bIsAuthorityTransitioning. The handler is the "
			 "emergency restore path — it fires when the editor is forcibly tearing down PIE "
			 "(crash, hard stop) and the EndPIE handler may or may not have run. Leaving the "
			 "flag set freezes the diff-apply path forever after a forced PIE stop, which is "
			 "the named failure mode (B7): 'leaves bIsAuthorityTransitioning stuck after closing "
			 "PIE'."),
		CodeOnly.Contains(TEXT("bIsAuthorityTransitioning = false"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("bIsAuthorityTransitioning=false"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: OnEditorShutdownPIE must call RestoreGameFeatureTargetState on the tag-driven "
			 "features. The spec is explicit: 'ShutdownPIE is an emergency restore — restore "
			 "baseline'. The restore returns each tag-driven feature to its .uplugin "
			 "BuiltInInitialFeatureState; calling SetModularGameFeaturesActive(false) instead "
			 "tears down boot-activated features and never reactivates them (B6)."),
		CodeOnly.Contains(TEXT("RestoreGameFeatureTargetState"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: OnEditorShutdownPIE must source its feature list from "
			 "UBmrModularGameFeatureSettings::Get().GetModularGameFeaturesByTags(). Restoring an "
			 "arbitrary list (or an empty list) leaves PIE-state features active in the editor "
			 "world. The api_surface pins the exact retrieval: "
			 "GetModularGameFeaturesByTags().GetKeys(TagDrivenFeatures)."),
		CodeOnly.Contains(TEXT("GetModularGameFeaturesByTags"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: OnEditorShutdownPIE must reset ActiveFeatures (TSet<FName>::Reset / Empty). "
			 "After the restore, the subsystem's notion of 'what is active' is no longer "
			 "synchronised with the engine — keeping the stale set produces phantom-deactivate "
			 "actions on the next recompute."),
		CodeOnly.Contains(TEXT("ActiveFeatures.Reset"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("ActiveFeatures.Empty"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: OnEditorShutdownPIE must finish by calling QueueDeferredRecompute. The restore "
			 "puts features back to their baseline; the deferred recompute then applies the "
			 "current editor-world authority over that baseline. Skipping the recompute leaves "
			 "the editor showing its baseline indefinitely until the next ASC tag event fires."),
		CodeOnly.Contains(TEXT("QueueDeferredRecompute"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B3: No-authority short-circuit in ApplyAuthoritativeFeatureSet.
//
// The spec's load-bearing point: "Without an authoritative ASC, the
// recompute must short-circuit — skipping this guard tears down features
// the engine activated at boot via .uplugin BuiltInInitialFeatureState."
// This is the most common failure mode the evaluator notes call out
// (common model error #1). The canonical encoding aggregates only
// authoritative ASCs into bHasAuthoritativeAsc + Aggregate, then bails
// when bHasAuthoritativeAsc is false — BEFORE any feature is deactivated.
// A re-impl that always applies the diff (even with an empty aggregate)
// produces a wave of deactivations on every cold start, then re-activates
// nothing, leaving the engine without its boot features.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_NoAuthorityShortCircuit,
	"Bomber.ModularFeaturesLoader.NoAuthorityShortCircuit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_NoAuthorityShortCircuit::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::ApplyAuthoritativeFeatureSet"));
	if (!TestTrue(
			TEXT("B3: BmrModularGameFeaturesLoaderSubsystem.cpp must define "
				 "ApplyAuthoritativeFeatureSet with a body — the start/ stub is empty; the "
				 "no-authority short-circuit is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B3: ApplyAuthoritativeFeatureSet must walk AscOwnedTags and gate participation on "
			 "IsAuthoritativeAsc. The spec: 'the recompute aggregates tags only from "
			 "authoritative ASCs'. A body that aggregates over every tracked ASC (without the "
			 "IsAuthoritativeAsc gate) lets the non-authoritative world drive the diff — for "
			 "instance, the editor world's empty-tag ASC would zero out the aggregate during "
			 "PIE and tear down every feature the PIE world activated."),
		CodeOnly.Contains(TEXT("AscOwnedTags"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("IsAuthoritativeAsc"), ESearchCase::CaseSensitive));

	// The short-circuit itself: when no authoritative ASC was found,
	// return BEFORE any feature is deactivated. The canonical encoding
	// flips a `bHasAuthoritativeAsc` flag inside the aggregation loop and
	// returns when it is false. Equivalent shapes — checking
	// `AuthoritativeAscCount == 0`, or stashing the count on a TArray
	// `.Num()` — are all accepted; what is NOT accepted is dropping the
	// guard entirely.
	const bool bHasShortCircuit =
		CodeOnly.Contains(TEXT("bHasAuthoritativeAsc"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("HasAuthoritativeAsc"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("bAnyAuthoritative"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("bAnyAuthoritativeAsc"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B3: ApplyAuthoritativeFeatureSet must track whether ANY authoritative ASC was "
			 "found (e.g. a bHasAuthoritativeAsc flag set inside the aggregation loop). The "
			 "spec calls this guard 'load-bearing': without it, a cold start (no ASC yet "
			 "registered) walks the no-ASC path and tears down boot-activated features. The "
			 "evaluator notes pin this as common error #1: 'Missing no-authority short-circuit "
			 "— most common and load-bearing; features activated at boot get torn down.'"),
		bHasShortCircuit);

	// Negative: the function must NOT call SetModularGameFeaturesActive
	// unconditionally without first checking authority. The canonical
	// shape returns when bHasAuthoritativeAsc is false BEFORE reaching
	// the deactivate / activate batch. A simple proxy: the first
	// occurrence of `return` in the body must precede the first
	// occurrence of `SetModularGameFeaturesActive`. (The function does
	// also early-out for `bIsAuthorityTransitioning`, so there is
	// always at least one return before the calls; we just verify that
	// the SetModularGameFeaturesActive calls are not before any return.)
	const int32 FirstReturnIdx = CodeOnly.Find(TEXT("return"), ESearchCase::CaseSensitive);
	const int32 FirstSetActiveIdx =
		CodeOnly.Find(TEXT("SetModularGameFeaturesActive"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B3: ApplyAuthoritativeFeatureSet must NOT call SetModularGameFeaturesActive before "
			 "checking whether any authoritative ASC is registered. The no-authority short-"
			 "circuit and the bIsAuthorityTransitioning early-out must both precede the "
			 "deactivate batch. A body that orders the deactivate call before any return "
			 "tears down boot-activated features on every cold path."),
		FirstReturnIdx != INDEX_NONE
			&& (FirstSetActiveIdx == INDEX_NONE || FirstReturnIdx < FirstSetActiveIdx));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B4: Feature changes are deferred — OnAscTagCountChanged queues a
// recompute rather than applying SetModularGameFeaturesActive synchronously.
//
// The spec: "Applying feature changes synchronously inside the tag-event
// callback can cause reentry or deadlock." The tag-event listeners fire
// from inside the ASC's gameplay-tag mutation path; reaching
// SetModularGameFeaturesActive from there can re-enter that same path
// (a modular feature's activation can add owned tags) and deadlock on
// the engine's gameplay-tag table mutex. The canonical encoding routes
// every tag-event delta through QueueDeferredRecompute. A re-impl that
// applies the change in the callback compiles fine but hangs the editor
// the moment two features share an overlapping tag.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_TagEventDefersToRecompute,
	"Bomber.ModularFeaturesLoader.TagEventDefersToRecompute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_TagEventDefersToRecompute::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::OnAscTagCountChanged_Implementation"));
	if (!TestTrue(
			TEXT("B4: BmrModularGameFeaturesLoaderSubsystem.cpp must define "
				 "OnAscTagCountChanged_Implementation with a body — the start/ stub is empty; the "
				 "incremental-update path is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The callback must update its per-ASC snapshot incrementally:
	// AddTag for NewCount > 0, RemoveTag for == 0. A re-impl that
	// resnapshots the ASC's full tag set on every event is O(N) where
	// it needs to be O(1) and races against further tag mutations
	// inside the same callback chain.
	TestTrue(
		TEXT("B4: OnAscTagCountChanged must look up the per-ASC snapshot (AscOwnedTags.Find) "
			 "and incrementally mutate it. The api_surface pins the first lines: "
			 "`AscOwnedTags.Find(SourceAsc); if (!OwnedSnapshot) return; if (NewCount > 0) ...`. "
			 "A body that ignores the snapshot lookup and reads the live ASC tags directly "
			 "races against the in-flight tag mutation that just fired this event."),
		CodeOnly.Contains(TEXT("AscOwnedTags.Find"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: OnAscTagCountChanged's NewCount > 0 branch must call AddTag on the snapshot, "
			 "and the NewCount == 0 branch must call RemoveTag. The spec is explicit: 'update "
			 "the snapshot incrementally (add on NewCount > 0, remove on == 0)'. A body that "
			 "calls only one of the two leaks tags one direction."),
		CodeOnly.Contains(TEXT("AddTag"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("RemoveTag"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4: OnAscTagCountChanged's NewCount > 0 test must be in CODE — the snapshot's "
			 "add/remove branch is driven by the changed count, not by re-reading the live tag "
			 "count off the source ASC (which can have raced since the event fired)."),
		CodeOnly.Contains(TEXT("NewCount > 0"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("NewCount >= 1"), ESearchCase::CaseSensitive));

	// The behavior under test: every code path through the callback ends
	// in QueueDeferredRecompute, NOT in a direct SetModularGameFeaturesActive
	// or ApplyAuthoritativeFeatureSet call.
	TestTrue(
		TEXT("B4: OnAscTagCountChanged must call QueueDeferredRecompute to schedule the apply "
			 "for next tick. The spec calls out the failure mode by name: 'Applying feature "
			 "changes synchronously inside the tag-event callback can cause reentry or "
			 "deadlock.' Queueing defers the apply past the current frame, out of the gameplay-"
			 "tag mutation path that triggered the event."),
		CodeOnly.Contains(TEXT("QueueDeferredRecompute"), ESearchCase::CaseSensitive));

	const bool bAppliesSynchronously =
		CodeOnly.Contains(TEXT("SetModularGameFeaturesActive"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("ApplyAuthoritativeFeatureSet"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B4: OnAscTagCountChanged must NOT call SetModularGameFeaturesActive or "
			 "ApplyAuthoritativeFeatureSet directly. The spec's failure mode: 'Applying feature "
			 "changes synchronously inside the tag-event callback can cause reentry or "
			 "deadlock.' Synchronous application reaches into UGameFeaturesSubsystem which can "
			 "mutate the gameplay-tag table while THIS callback is iterating it. The canonical "
			 "path is QueueDeferredRecompute → next-tick ticker → ApplyAuthoritativeFeatureSet."),
		bAppliesSynchronously);

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B5: QueueDeferredRecompute must be guarded by
// DeferredRecomputeHandle.IsValid() — N tag events in the same frame must
// produce exactly ONE ticker registration.
//
// The spec: "QueueDeferredRecompute must suppress duplicate enqueues — if
// a recompute is already pending, return immediately. This collapses
// tag-event bursts in a single frame into one recompute." The evaluator
// notes pin this as common error #3 and "load-bearing": without the
// guard, N tag events queue N tickers, each fires the same recompute,
// and the same deactivate/activate batch is applied N times — visible to
// the player as a feature churn cycle. The api_surface pins the first
// lines: `if (DeferredRecomputeHandle.IsValid()) return;`.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_QueueDeferredRecomputeCoalesces,
	"Bomber.ModularFeaturesLoader.QueueDeferredRecomputeCoalesces",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_QueueDeferredRecomputeCoalesces::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::QueueDeferredRecompute"));
	if (!TestTrue(
			TEXT("B5: BmrModularGameFeaturesLoaderSubsystem.cpp must define "
				 "QueueDeferredRecompute with a body — the start/ stub is empty; the coalescing "
				 "guard is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B5: QueueDeferredRecompute must guard with DeferredRecomputeHandle.IsValid(). The "
			 "api_surface pins the first lines: `if (DeferredRecomputeHandle.IsValid()) "
			 "return;`. Without the guard, N tag events in the same frame schedule N tickers "
			 "— the spec's named failure mode: 'N tag events → N tickers → N churn cycles.' "
			 "The evaluator notes flag this as common error #3."),
		CodeOnly.Contains(TEXT("DeferredRecomputeHandle.IsValid"), ESearchCase::CaseSensitive));

	// The guard must SHORT-CIRCUIT — the IsValid() branch must contain a
	// return statement. A re-impl that checks IsValid() but still adds a
	// second ticker (e.g. as an "if (!Valid) handle = …; else handle =
	// addTicker(…);" pattern) still schedules duplicates.
	const int32 IsValidIdx =
		CodeOnly.Find(TEXT("DeferredRecomputeHandle.IsValid"), ESearchCase::CaseSensitive);
	const int32 AddTickerIdx = CodeOnly.Find(TEXT("AddTicker"), ESearchCase::CaseSensitive);
	const int32 ReturnIdx =
		CodeOnly.Find(TEXT("return"), ESearchCase::CaseSensitive, ESearchDir::FromStart, IsValidIdx);
	TestTrue(
		TEXT("B5: QueueDeferredRecompute's IsValid() branch must short-circuit (return) BEFORE "
			 "reaching FTSTicker::GetCoreTicker().AddTicker(...). Any code path that lets a "
			 "second IsValid()-guarded call still register a ticker defeats the coalescing — "
			 "the spec's failure mode survives."),
		IsValidIdx != INDEX_NONE
			&& ReturnIdx != INDEX_NONE
			&& (AddTickerIdx == INDEX_NONE || ReturnIdx < AddTickerIdx));

	// The ticker payload must reset the handle when it fires so the NEXT
	// QueueDeferredRecompute call after the ticker runs can register a
	// fresh ticker. Without the reset, the first recompute leaves the
	// handle valid forever and every subsequent tag-event burst is
	// silently dropped (no recompute ever fires again).
	TestTrue(
		TEXT("B5: QueueDeferredRecompute's ticker lambda must clear DeferredRecomputeHandle "
			 "(`.Reset()` or equivalent) before invoking the recompute. Without the clear, "
			 "the first recompute leaves the handle valid forever and the IsValid() guard "
			 "silently drops every subsequent tag-event burst — the subsystem becomes a "
			 "no-op after the first frame."),
		CodeOnly.Contains(TEXT("DeferredRecomputeHandle.Reset"), ESearchCase::CaseSensitive));

	// And the ticker must register against FTSTicker::GetCoreTicker(),
	// not a world / actor tick — the subsystem outlives every world.
	TestTrue(
		TEXT("B5: QueueDeferredRecompute must register the deferred apply with "
			 "FTSTicker::GetCoreTicker().AddTicker — the engine subsystem outlives every world, "
			 "so a world-bound tick (FTickerObjectBase, World->Timer) would silently drop the "
			 "callback the first time the world destructs."),
		CodeOnly.Contains(TEXT("FTSTicker::GetCoreTicker"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("AddTicker"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — B6: Editor map switch restores baseline, not deactivates.
//
// The spec's most subtle failure mode: "Using SetModularGameFeaturesActive
// (false) on editor map switch instead of RestoreGameFeatureTargetState
// conflates the two destruction paths." When the editor user changes
// maps, the editor world finishes destroy; the subsystem must put the
// tag-driven features BACK to their .uplugin BuiltInInitialFeatureState
// (so the next loaded level opens with the correct boot-features active).
// Calling SetModularGameFeaturesActive(false) instead deactivates every
// tag-driven feature unconditionally and leaves the next map without
// its boot baseline. The PIE-world destruction path takes the OTHER
// branch — it only queues a recompute, because the editor world's ASC
// is about to take authority back.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_EditorMapSwitchUsesRestore,
	"Bomber.ModularFeaturesLoader.EditorMapSwitchUsesRestore",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_EditorMapSwitchUsesRestore::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::OnPreWorldFinishDestroy_Implementation"));
	if (!TestTrue(
			TEXT("B6: BmrModularGameFeaturesLoaderSubsystem.cpp must define "
				 "OnPreWorldFinishDestroy_Implementation with a body — the start/ stub is empty; "
				 "the editor-map-switch cleanup is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The body must drain stale ASC entries first — leaving them in
	// AscOwnedTags after the world is gone keeps TWeakObjectPtrs that
	// resolve to nullptr inside ApplyAuthoritativeFeatureSet on the
	// next recompute. The canonical encoding iterates AscOwnedTags
	// and removes entries whose ASC is stale or whose world matches
	// the destroying world.
	TestTrue(
		TEXT("B6: OnPreWorldFinishDestroy must drain AscOwnedTags entries belonging to the "
			 "destroying world. Leaking those entries leaves the next recompute aggregating "
			 "tags from a TWeakObjectPtr that resolves to nullptr — the IsAuthoritativeAsc "
			 "guard then returns false for that entry on every recompute, but the map slot "
			 "is never reclaimed."),
		CodeOnly.Contains(TEXT("AscOwnedTags"), ESearchCase::CaseSensitive)
			&& (CodeOnly.Contains(TEXT("RemoveCurrent"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("CreateIterator"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("Remove("), ESearchCase::CaseSensitive)));

	// The editor branch must call RestoreGameFeatureTargetState (NOT
	// SetModularGameFeaturesActive(false)). The branch is conditional
	// on WorldType == EWorldType::Editor.
	TestTrue(
		TEXT("B6: OnPreWorldFinishDestroy must call RestoreGameFeatureTargetState on the "
			 "editor-world destruction branch. The spec: 'In editor builds, an editor-world "
			 "destruction means a map switch — restore features to their .uplugin baseline "
			 "rather than deactivating them.' The .uplugin's BuiltInInitialFeatureState is "
			 "the source of truth for which features should be active on the next map; "
			 "deactivating instead skips that baseline."),
		CodeOnly.Contains(TEXT("RestoreGameFeatureTargetState"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B6: OnPreWorldFinishDestroy's editor-world branch must trigger on "
			 "EWorldType::Editor. A branch that triggers on every world (or only on PIE) "
			 "either restores baseline on PIE destruction (wrong — the editor ASC takes over) "
			 "or never restores at all (the next map loads without its boot features)."),
		CodeOnly.Contains(TEXT("EWorldType::Editor"), ESearchCase::CaseSensitive));

	// Negative: the editor-world destruction branch must NOT take the
	// SetModularGameFeaturesActive(false, ...) path. This is the named
	// failure mode the spec calls out by NAME: "conflates the two
	// destruction paths." We accept the body containing
	// SetModularGameFeaturesActive elsewhere (the ApplyAuthoritativeFeatureSet
	// path can legitimately reach it) — but it must not appear inside
	// OnPreWorldFinishDestroy.
	const bool bUsesDeactivateOnDestroy =
		CodeOnly.Contains(TEXT("SetModularGameFeaturesActive(false"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("SetModularGameFeaturesActive( false"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B6: OnPreWorldFinishDestroy must NOT call SetModularGameFeaturesActive(false, ...). "
			 "The spec names this as a failure mode by name: 'Using SetModularGameFeaturesActive"
			 "(false) on editor map switch instead of RestoreGameFeatureTargetState conflates "
			 "the two destruction paths.' Deactivate is for runtime tag-driven cycling; "
			 "Restore is for editor-world teardown."),
		bUsesDeactivateOnDestroy);

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — B7: ShutdownPIE delegate must be bound so bIsAuthorityTransitioning
// can never get stuck after a forced PIE close.
//
// The spec's named failure mode: "Forgetting ShutdownPIE — leaves
// bIsAuthorityTransitioning stuck after closing PIE." The handler itself
// (tested by B2) is useless without the FEditorDelegates::ShutdownPIE
// binding in Initialize. Without this binding, a forced PIE close
// (compile-while-PIE recovery, ALT-F4 in PIE viewport, sandbox crash
// recovery) leaves the flag set from OnEndPIE forever and the diff-apply
// path is frozen for the remainder of the editor session.
//
// This test also gates Deinitialize: the handler must be unbound on
// teardown so a re-initialised subsystem (test runner reload, plugin
// hot-reload) doesn't accumulate stale bindings that fire into a destroyed
// `this` and crash the editor.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_ShutdownPIEDelegateBound,
	"Bomber.ModularFeaturesLoader.ShutdownPIEDelegateBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_ShutdownPIEDelegateBound::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString InitBody =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::Initialize"));
	if (!TestTrue(
			TEXT("B7: BmrModularGameFeaturesLoaderSubsystem.cpp must define Initialize with a body."),
			!InitBody.IsEmpty()))
	{
		return false;
	}

	const FString InitCode = StripComments(InitBody);

	TestTrue(
		TEXT("B7: Initialize must bind FEditorDelegates::ShutdownPIE to OnEditorShutdownPIE. The "
			 "spec names the failure mode: 'Forgetting ShutdownPIE — leaves "
			 "bIsAuthorityTransitioning stuck after closing PIE.' Without the binding, a "
			 "forced PIE close (compile-while-PIE, ALT-F4 in viewport) leaves the flag set "
			 "from OnEndPIE forever and the diff-apply path is frozen for the rest of the "
			 "editor session. The handler body (B2) is the emergency-restore action; this "
			 "binding is what makes the engine actually call it."),
		InitCode.Contains(TEXT("FEditorDelegates::ShutdownPIE"), ESearchCase::CaseSensitive)
			&& InitCode.Contains(TEXT("OnEditorShutdownPIE"), ESearchCase::CaseSensitive));

	// Initialize's three editor bindings must live behind WITH_EDITOR.
	// A re-impl that drops the guard breaks non-editor builds (Game
	// target) where FEditorDelegates does not exist as a symbol.
	TestTrue(
		TEXT("B7: Initialize's editor delegate bindings must be guarded by #if WITH_EDITOR. "
			 "FEditorDelegates is not compiled in non-editor targets; a body that calls "
			 "FEditorDelegates::ShutdownPIE.AddUObject(...) outside the guard fails to link "
			 "a Game-target build."),
		InitCode.Contains(TEXT("WITH_EDITOR"), ESearchCase::CaseSensitive));

	// Deinitialize must remove all editor delegates so the subsystem
	// teardown doesn't leave a dangling weak binding that fires after
	// the subsystem instance is gone.
	const FString DeinitBody =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::Deinitialize"));
	if (!TestTrue(
			TEXT("B7: BmrModularGameFeaturesLoaderSubsystem.cpp must define Deinitialize with a body."),
			!DeinitBody.IsEmpty()))
	{
		return false;
	}

	const FString DeinitCode = StripComments(DeinitBody);

	TestTrue(
		TEXT("B7: Deinitialize must unbind FEditorDelegates::ShutdownPIE. The api_surface "
			 "pins the canonical pattern: `FEditorDelegates::ShutdownPIE.RemoveAll(this);`. "
			 "Leaving the binding live past Deinitialize lets the engine call into a "
			 "destroyed subsystem instance after a hot-reload or test-runner reset."),
		DeinitCode.Contains(TEXT("FEditorDelegates::ShutdownPIE"), ESearchCase::CaseSensitive)
			&& DeinitCode.Contains(TEXT("RemoveAll"), ESearchCase::CaseSensitive));

	// Symmetric: PreBeginPIE and EndPIE must also be unbound on
	// teardown. (B1 verifies they are bound on init.)
	TestTrue(
		TEXT("B7: Deinitialize must unbind FEditorDelegates::PreBeginPIE and EndPIE. Leaking "
			 "either binding past subsystem teardown produces a stale dispatch on the next "
			 "PIE entry that fires into a destroyed instance."),
		DeinitCode.Contains(TEXT("FEditorDelegates::PreBeginPIE"), ESearchCase::CaseSensitive)
			&& DeinitCode.Contains(TEXT("FEditorDelegates::EndPIE"), ESearchCase::CaseSensitive));

	// Also clean up the deferred-recompute ticker on teardown — the
	// ticker captures a weak `this`; leaving it scheduled past
	// Deinitialize keeps the engine spinning the callback for one more
	// frame against the dead subsystem. The api_surface pins the
	// canonical Deinitialize prefix:
	//   if (DeferredRecomputeHandle.IsValid()) {
	//     FTSTicker::GetCoreTicker().RemoveTicker(DeferredRecomputeHandle);
	//     DeferredRecomputeHandle.Reset(); }
	TestTrue(
		TEXT("B7: Deinitialize must remove the deferred-recompute ticker if one is pending. "
			 "The api_surface pins the canonical guard: `if "
			 "(DeferredRecomputeHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(...);`. "
			 "Leaving a pending ticker past teardown lets it fire one more time into a "
			 "destroyed subsystem before the weak-lambda observes the dead `this`."),
		DeinitCode.Contains(TEXT("RemoveTicker"), ESearchCase::CaseSensitive)
			&& DeinitCode.Contains(TEXT("DeferredRecomputeHandle"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Cross-cut: IsAuthoritativeAsc encodes the PIE-vs-editor authority
// rule that B3's no-authority guard depends on.
//
// The no-authority short-circuit (B3) is only correct if IsAuthoritativeAsc
// gives the right answer. The spec is explicit about the asymmetry:
// "during PIE, only the PIE world is authoritative; otherwise, editor
// and game worlds are authoritative." A re-impl that returns true for
// EWorldType::Game only (forgetting the editor-world case) breaks the
// editor-cook path — every recompute outside PIE returns "no authority"
// and tears down boot features. A re-impl that returns true for both
// editor and PIE worlds during a play session lets the editor-world ASC's
// stale tags drive the diff while PIE is alive — features the PIE world
// activates immediately get torn down by the next editor-world recompute.
//
// This test gates the structural shape: the body must consult both
// EWorldType::PIE and at least one of EWorldType::Editor / Game, and it
// must reach for GEditor->PlayWorld (or equivalent PIE-active check)
// under WITH_EDITOR.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_IsAuthoritativeAscWorldTypeRule,
	"Bomber.ModularFeaturesLoader.IsAuthoritativeAscWorldTypeRule",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_IsAuthoritativeAscWorldTypeRule::RunTest(const FString& Parameters)
{
	using namespace ModularFeaturesLoaderTests;

	const FString Source = LoadProjectFile(this, LoaderSubsystemRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body =
		ExtractMemberBody(Source, TEXT("UBmrModularGameFeaturesLoaderSubsystem::IsAuthoritativeAsc"));
	if (!TestTrue(
			TEXT("B3/cross-cut: BmrModularGameFeaturesLoaderSubsystem.cpp must define "
				 "IsAuthoritativeAsc with a body — the start/ stub is empty; the world-type "
				 "authority rule is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Defensive nulls: ASC and ASC->GetWorld(). Without these, the
	// recompute walks stale entries left in AscOwnedTags after a world
	// destruction (the destruction path drains, but the destruction
	// itself can race against an in-flight recompute) and dereferences
	// nullptr on the very first frame of the next map.
	TestTrue(
		TEXT("B3/cross-cut: IsAuthoritativeAsc must defend against a null ASC argument and a "
			 "null ASC->GetWorld(). The api_surface pins both early-outs: ASC null check "
			 "first, then GetWorld() null check, both returning false. Without these the "
			 "recompute crashes on a stale AscOwnedTags entry during a world-destruction "
			 "race."),
		CodeOnly.Contains(TEXT("!ASC"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("GetWorld"), ESearchCase::CaseSensitive));

	// The function must have SOME mechanism to detect that a PIE session
	// is active and gate the return to PIE-only in that case. The canonical
	// uses GEditor->PlayWorld; valid alternatives include consulting the
	// authority-transition state, or inspecting the tracked-ASCs set for a
	// PIE-typed world. What's NOT accepted is no PIE-active branch at all
	// (the editor world remains authoritative while PIE is alive).
	const bool bHasPIEActiveDetection =
		CodeOnly.Contains(TEXT("GEditor->PlayWorld"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GEditor && GEditor->PlayWorld"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("bIsAuthorityTransitioning"), ESearchCase::CaseSensitive)
		|| (CodeOnly.Contains(TEXT("AscOwnedTags"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("EWorldType::PIE"), ESearchCase::CaseSensitive));
	TestTrue(
		TEXT("B3/cross-cut: IsAuthoritativeAsc must have a PIE-active detection branch under "
			 "WITH_EDITOR — without one, the editor world remains authoritative while PIE is "
			 "alive and its stale tags drive every recompute. Accepted forms: GEditor->PlayWorld "
			 "check, authority-transition state check, or inspecting tracked ASCs for a "
			 "PIE-typed world."),
		CodeOnly.Contains(TEXT("WITH_EDITOR"), ESearchCase::CaseSensitive)
			&& bHasPIEActiveDetection);

	TestTrue(
		TEXT("B3/cross-cut: IsAuthoritativeAsc's PIE-active branch must return EWorldType::PIE "
			 "(and only PIE). The spec: 'during PIE, only the PIE world is authoritative.' "
			 "Returning true for Editor too lets the editor-world ASC compete with the PIE "
			 "world's ASC for authority."),
		CodeOnly.Contains(TEXT("EWorldType::PIE"), ESearchCase::CaseSensitive));

	// The non-PIE branch must accept both EWorldType::Editor AND
	// EWorldType::Game. A re-impl that returns true for Game only
	// breaks the editor-cook path (no PIE active, editor world is
	// authoritative); a re-impl that returns true for Editor only
	// breaks the standalone Game build (no editor at all).
	TestTrue(
		TEXT("B3/cross-cut: IsAuthoritativeAsc's non-PIE branch must return true for "
			 "EWorldType::Editor and EWorldType::Game both. The spec: 'otherwise, editor and "
			 "game worlds are authoritative.' Restricting to Game only breaks the editor-cook "
			 "path; restricting to Editor only breaks the standalone Game build. The "
			 "evaluator notes call this out as common error #6: 'IsAuthoritativeAsc returning "
			 "true only for EWorldType::Game in non-editor builds — breaks editor cook.'"),
		CodeOnly.Contains(TEXT("EWorldType::Editor"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("EWorldType::Game"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Runtime behavioral coverage of the recompute surface (G2: B3, B5).
//
// The structural tests above pin the code-path SHAPE of the failure-mode
// guards; this test exercises the LIVE subsystem at runtime via reflection
// to verify the same guards survive the engine's actual UFUNCTION dispatch.
//
// Three behaviors are observed live:
//   * B5 — QueueDeferredRecompute coalescing. A burst of N reflective
//     ProcessEvent calls in a single frame must not register N tickers nor
//     re-entrantly fire the recompute lambda. A missing IsValid() guard
//     would either crash on a duplicate ticker registration (under some
//     UE versions FTSTicker asserts on a repeated handle) or silently
//     queue N redundant ApplyAuthoritativeFeatureSet passes.
//   * B3 — ApplyAuthoritativeFeatureSet no-authority short-circuit. With
//     no AscOwnedTags entries (the default state in a headless harness
//     before any WorldASC_Ready broadcast), the function must short-circuit
//     before any SetModularGameFeaturesActive call. We observe via the
//     ActiveFeatures UPROPERTY: a re-impl missing the short-circuit would
//     compute a deactivate batch against the engine's currently-active
//     boot features and mutate ActiveFeatures.
//   * HasPendingTagDrivenActivations — the public pending-check is the
//     externally-visible health probe; a live call after the burst verifies
//     the subsystem's internal map / ticker handle remain queryable and
//     have not been corrupted by the previous calls.
//
// Why reflective ProcessEvent and not direct C++ calls:
//   The Bomber subsystem header is reachable from the test compile unit,
//   but pulling it in transitively requires UModularGameFeaturePluginUtils,
//   UBmrModularGameFeatureSettings, AbilitySystemComponent.h, and the
//   GameplayAbilities module — none of which the test fixture depends on.
//   The UFUNCTION dispatch is exactly the same code path the engine takes
//   when calling these methods from the deferred ticker, so the runtime
//   surface is identical.
//
// Discrimination note: in a clean automation harness AscOwnedTags is
// empty, so an impl missing the no-authority short-circuit may still
// produce a zero-delta apply (empty Aggregate → empty NewActive → empty
// ToDeactivate when ActiveFeatures is also empty). The HARD discrimination
// of B3 lives in Test 3's structural assertion (`bHasAuthoritativeAsc`
// flag + early return); this test is the runtime smoke that the live
// methods are reachable, side-effect-free under no-authority, and survive
// burst enqueues.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FModularFeaturesLoader_RuntimeRecomputeSurfaceCallable,
	"Bomber.ModularFeaturesLoader.RuntimeRecomputeSurfaceCallable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FModularFeaturesLoader_RuntimeRecomputeSurfaceCallable::RunTest(const FString& Parameters)
{
	if (!TestNotNull(
			TEXT("GEngine must be available for engine-subsystem lookup. A null GEngine here "
				 "means the test is running outside the engine boot — there is no subsystem "
				 "to probe."),
			GEngine))
	{
		return false;
	}

	// The subsystem's UClass is reachable via the package-qualified class
	// path. FindObject avoids pulling the Bomber subsystem header into this
	// translation unit (which would transitively require AbilitySystemComponent
	// and the GameplayAbilities module).
	UClass* SubsystemClass = FindObject<UClass>(
		nullptr, TEXT("/Script/Bomber.BmrModularGameFeaturesLoaderSubsystem"));
	if (!TestNotNull(
			TEXT("G2/B3,B5: UBmrModularGameFeaturesLoaderSubsystem UClass must be reflected. A "
				 "null lookup means the Bomber module did not load — every UFUNCTION below is "
				 "unreachable regardless of the cpp body."),
			SubsystemClass))
	{
		return false;
	}

	UObject* Subsystem = GEngine->GetEngineSubsystemBase(SubsystemClass);
	if (!TestNotNull(
			TEXT("G2/B3,B5: The engine subsystem instance must be alive at runtime. The engine "
				 "subsystem framework instantiates UCLASS-declared UEngineSubsystem subclasses "
				 "automatically; a null here means the UCLASS macro was clobbered or the "
				 "framework rejected the class (e.g. Abstract / NotPlaceable). Without an "
				 "instance, no UFUNCTION dispatch below is reachable."),
			Subsystem))
	{
		return false;
	}

	// ActiveFeatures is the single externally-visible piece of subsystem
	// state (UPROPERTY VisibleAnywhere, BlueprintReadOnly). The set's count
	// is what we observe across the burst + apply to detect a missing
	// short-circuit.
	FSetProperty* ActiveFeaturesProp =
		FindFProperty<FSetProperty>(SubsystemClass, TEXT("ActiveFeatures"));
	if (!TestNotNull(
			TEXT("G2/B3: ActiveFeatures (UPROPERTY TSet<FName>) must be reflected so external "
				 "observers — this test, the Editor Details panel, Blueprint callers — can read "
				 "the subsystem's view of currently-active tag-driven features. A null property "
				 "here means the header lost its UPROPERTY annotation."),
			ActiveFeaturesProp))
	{
		return false;
	}

	void* SetData = ActiveFeaturesProp->ContainerPtrToValuePtr<void>(Subsystem);
	const int32 InitialActiveCount = FScriptSetHelper(ActiveFeaturesProp, SetData).Num();

	// ── B5: QueueDeferredRecompute must coalesce duplicate enqueues ──────────
	// Burst-call the function via ProcessEvent. Under a correct
	// IsValid()-guarded impl, the first call registers a single core-ticker
	// and every subsequent call early-outs. Under a missing guard, the
	// impl registers eight tickers (or worse, asserts on a duplicate
	// handle on some UE 5.x revs). Either way, the act of bursting here
	// has to remain safe and non-blocking.
	UFunction* QueueFn =
		SubsystemClass->FindFunctionByName(TEXT("QueueDeferredRecompute"));
	if (TestNotNull(
			TEXT("G2/B5: QueueDeferredRecompute UFUNCTION must be reflected (the header declares "
				 "it BlueprintCallable). ProcessEvent dispatches through the same native "
				 "pointer as internal C++ callers, so this is the same code path the tag-event "
				 "lambda hits."),
			QueueFn))
	{
		for (int32 Burst = 0; Burst < 8; ++Burst)
		{
			Subsystem->ProcessEvent(QueueFn, nullptr);
		}
	}

	// ── B3: ApplyAuthoritativeFeatureSet must short-circuit without authority.
	// Calling it directly bypasses the ticker schedule and exercises the
	// full recompute synchronously. In a headless harness AscOwnedTags is
	// empty (no world ASC has broadcast WorldASC_Ready at module-load
	// time), so the no-authority short-circuit must trigger before any
	// SetModularGameFeaturesActive call. A missing short-circuit reaches
	// for the engine's plugin manager and (best case) calls
	// SetModularGameFeaturesActive(false, ...) on every boot-active
	// feature — visible here as a mutation of ActiveFeatures.
	UFunction* ApplyFn =
		SubsystemClass->FindFunctionByName(TEXT("ApplyAuthoritativeFeatureSet"));
	if (TestNotNull(
			TEXT("G2/B3: ApplyAuthoritativeFeatureSet UFUNCTION must be reflected (the header "
				 "declares it BlueprintCallable). This is the function the deferred ticker "
				 "dispatches to; if it is not reflected the ticker lambda's `Get().Apply...` "
				 "call would fail to bind after a hot-reload."),
			ApplyFn))
	{
		Subsystem->ProcessEvent(ApplyFn, nullptr);
	}

	const int32 FinalActiveCount = FScriptSetHelper(ActiveFeaturesProp, SetData).Num();
	TestEqual(
		TEXT("G2/B3: ActiveFeatures count must be unchanged after a burst of "
			 "QueueDeferredRecompute calls and a direct ApplyAuthoritativeFeatureSet call. A "
			 "delta here means the recompute ran a deactivate-or-activate pass against an "
			 "empty authoritative ASC set — exactly the no-authority failure mode the spec "
			 "calls out: 'tears down features the engine activated at boot via .uplugin "
			 "BuiltInInitialFeatureState'."),
		FinalActiveCount,
		InitialActiveCount);

	// The public HasPendingTagDrivenActivations probe is BlueprintCallable.
	// A live invocation after the burst confirms the subsystem stayed in a
	// healthy queryable state and that the FTSTicker handle (if it was
	// registered) does not corrupt subsequent UFUNCTION dispatch. We do
	// not assert a specific return value — in a headless harness the world
	// ASC may be null (the function returns the defensive `true`), and in
	// a PIE harness the value depends on the live authoritative tag set.
	UFunction* PendingFn =
		SubsystemClass->FindFunctionByName(TEXT("HasPendingTagDrivenActivations"));
	if (TestNotNull(
			TEXT("HasPendingTagDrivenActivations UFUNCTION must be reflected (the header "
				 "declares it BlueprintCallable, BlueprintPure). The public probe is what "
				 "Blueprint pawns / UI surfaces query to gate behavior on pending tag-driven "
				 "activations."),
			PendingFn))
	{
		struct
		{
			bool ReturnValue;
		} Params;
		Params.ReturnValue = false;
		Subsystem->ProcessEvent(PendingFn, &Params);
	}

	return true;
}
