// Copyright (c) 2026 GameDevBench. NMM Player-Controller-Component automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Plugins/.../NewMainMenu/Source/NewMainMenu/Private/Components/NMMPlayerControllerComponent.cpp
//
// UNMMPlayerControllerComponent is the per-controller coordinator the New
// Main Menu game feature mounts onto every ABmrPlayerController. It owns
// four entangled concerns that all change on the same triggers:
//   * Input contexts (cinematic vs state-driven menu inputs),
//   * Mouse-visibility settings (cinematic vs menu),
//   * Background music (start when the game state enters Menu, stop on
//     GameStarting and at cinematic entry),
//   * Save-game lifecycle (async load on data-asset arrival, fresh-save
//     fallback when no slot exists).
// The component activates per controller while the menu plugin is alive
// and tears its bindings down on OnUnregister, including forwarding the
// MenuUnloaded broadcast to the server on non-authority clients.
//
// Why source-inspection coverage:
//   Direct runtime coverage of this surface needs a hydrated NewMainMenu
//   game-feature plugin (not loaded by default), the menu level
//   ("/Game/Bomber/Maps/MenuMain") with its placed spot owners, an
//   ABmrPlayerController possessed pawn, the global message subsystem and
//   the DAL data-asset registry both populated with NMM rows, and the menu
//   widgets initialized. None of that exists as a reusable PIE harness in
//   this project; reproducing it inside an automation test would dwarf the
//   surface under test. The NewMainMenu plugin is also not a Bomber.Build.cs
//   dependency, so we cannot include any of its headers
//   (UNMMPlayerControllerComponent, FNmmStateTag, UNMMSaveGameData, …) from
//   a Source/Bomber/Tests/ file without changing the module graph.
//
//   The spec and the evaluator notes pin down the five load-bearing
//   failure modes the component can suffer (the B1–B5 checklist in the
//   task brief):
//     B1: listeners bound in BeginPlay race data-asset availability.
//     B2: a stale MenuUnloaded message from a prior session replays on the
//         freshly-bound listener.
//     B3: cinematic-enable that forgets to disable game-state contexts
//         first leaks movement / bomb-placement inputs into cinematics.
//     B4: OnUnregister that doesn't ServerBroadcastMessage MenuUnloaded on
//         non-authority leaves the server StateTree wedged in Menu after
//         the client has left.
//     B5: the no-slot fallback that propagates the freshly-created save as
//         the active save (instead of the original parameter) skips the
//         persistence cycle for next session.
//   Each test below targets one of these failure modes, scanning the
//   comment-stripped body of the relevant member function so that anti-
//   pattern descriptions written in comments (the canonical solution
//   contains "Disable all other first", "Notify that Main Menu is being
//   unloaded before any cleanup", "Fallback to cache it") cannot satisfy
//   the assertions. Additional tests cover the broader behavior groups
//   the spec headings call out (music routing, widget-readiness short-
//   circuit, where the listener binding actually lives) so a re-impl that
//   moves a failure mode out of one function and into another still fails.
//   A final test reaches the plugin's UClass via the engine reflection
//   database to catch UFUNCTION / UPROPERTY rename regressions that source
//   scans alone wouldn't notice.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogNmmPlayerControllerComponentTests, Log, All);

namespace NmmPlayerControllerComponentTests
{
	// The stripped file lives inside the NewMainMenu plugin (a Game Feature
	// plugin), not the Bomber game module. The header is unchanged in this
	// task — the public surface is still visible, but every interesting body
	// in the .cpp was replaced with a safe stub.
	static const TCHAR* ComponentCppRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Private/Components/NMMPlayerControllerComponent.cpp");
	static const TCHAR* ComponentHeaderRelPath =
		TEXT("Plugins/GameFeatures/NewMainMenu/Source/NewMainMenu/Public/Components/NMMPlayerControllerComponent.h");

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

	// Strip C++ line and block comments so substring scans don't false-match
	// anti-pattern descriptions written in comments. The canonical solution's
	// own comments include "Disable all other first", "Notify that Main Menu
	// is being unloaded before any cleanup", "Clear stale MenuUnloaded from
	// previous session", "Fallback to cache it" — every one of these would
	// pass a naive substring test even against a stub body. String literals
	// are preserved so TEXT("Cinematic") inside a log call still shows up.
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

	// Brace-depth walker to extract the body of a member-function definition.
	// Counts braces only, not parens, so nested initialiser lists / lambdas
	// (e.g. delegate BindUObject's lambda captures) don't confuse the scan.
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

	// Find the byte offset of the first match in a body, accounting for the
	// body's position in the parent source. Used to verify ordering of two
	// calls inside the same function (e.g. broadcast-before-cleanup,
	// disable-all-before-enable-cinematic).
	static int32 FindOffsetIn(const FString& Body, const FString& Needle)
	{
		return Body.Find(Needle, ESearchCase::CaseSensitive);
	}
} // namespace NmmPlayerControllerComponentTests

// ---------------------------------------------------------------------------
// Test 1 — B1: BeginPlay binds NO global-message listeners.
//
// The spec's named failure mode: "Binding listeners in BeginPlay races data-
// asset availability." The data asset is loaded asynchronously through the
// DAL subsystem; a listener bound to MenuStateChanged or GameState_Changed in
// BeginPlay can fire before UNMMDataAsset is resolvable, and every handler in
// this component reads from that asset (input contexts, music, save slot
// metadata). Until the asset is ready, the handlers either crash on
// UNMMDataAsset::Get() (which checkf-asserts on a null DAL row) or silently
// no-op against a half-populated state machine.
//
// The canonical BeginPlay does exactly two things — register a data-asset
// arrival callback, and clear a stale MenuUnloaded message — and nothing
// else. Every listener binding is the responsibility of OnDataAssetLoaded
// (Test 6).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_BeginPlayBindsNoListeners,
	"Bomber.NMMPlayerControllerComponent.BeginPlayBindsNoListeners",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_BeginPlayBindsNoListeners::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::BeginPlay"));
	if (!TestTrue(
			TEXT("B1: NMMPlayerControllerComponent.cpp must define BeginPlay with a body — the "
				 "start/ stub is empty; the data-asset registration and stale-message clear are "
				 "what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// BeginPlay must call Super::BeginPlay() — it's an override.
	TestTrue(
		TEXT("B1: BeginPlay must call Super::BeginPlay() — the UActorComponent base does tick "
			 "registration and component-instance bookkeeping; skipping it puts the engine in "
			 "an inconsistent state for the rest of the controller's lifetime."),
		CodeOnly.Contains(TEXT("Super::BeginPlay"), ESearchCase::CaseSensitive));

	// BeginPlay must register a data-asset arrival callback through the DAL
	// subsystem. The canonical spelling is `UDalSubsystem::Get().ListenForDataAsset<
	// UNMMDataAsset>(this, &ThisClass::OnDataAssetLoaded)`.
	TestTrue(
		TEXT("B1: BeginPlay must register a data-asset arrival callback (UDalSubsystem::Get()."
			 "ListenForDataAsset<UNMMDataAsset>(this, &ThisClass::OnDataAssetLoaded)). That "
			 "callback is where the rest of the listener binding belongs — moving the binding "
			 "into BeginPlay races the async asset load and the very first message that fires "
			 "before the asset resolves crashes on UNMMDataAsset::Get()."),
		CodeOnly.Contains(TEXT("ListenForDataAsset"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("OnDataAssetLoaded"), ESearchCase::CaseSensitive));

	// Negative: BeginPlay must NOT directly subscribe to the menu / game /
	// widgets-initialized global messages. The named failure mode is exactly
	// this — moving the subscribe path out of OnDataAssetLoaded and into
	// BeginPlay. The canonical encoding for the subscribe is
	// `CallOrStartListeningForGlobalMessage` or `StartListeningForGlobalMessage`
	// or the BIND_ON_WIDGETS_INITIALIZED macro; none must appear in BeginPlay.
	const bool bBindsGlobalMessage =
		CodeOnly.Contains(TEXT("CallOrStartListeningForGlobalMessage"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("StartListeningForGlobalMessage"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("BIND_ON_WIDGETS_INITIALIZED"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B1: BeginPlay must NOT bind global-message listeners (no "
			 "CallOrStartListeningForGlobalMessage / StartListeningForGlobalMessage / "
			 "BIND_ON_WIDGETS_INITIALIZED in its body). The spec's named failure mode: "
			 "'Binding listeners in BeginPlay races data-asset availability.' The async DAL "
			 "load is the gate; the rest of the wiring lives in OnDataAssetLoaded."),
		bBindsGlobalMessage);

	// Negative: BeginPlay must not kick off the async save load either —
	// AsyncLoadGameFromSlot also reads UNMMDataAsset metadata indirectly
	// through UNMMSaveGameData::GetSaveSlotName(), so it suffers the same
	// data-asset race if started in BeginPlay.
	TestFalse(
		TEXT("B1: BeginPlay must NOT kick off the async save-game load — AsyncLoadGameFromSlot "
			 "consumes the save-slot metadata that lives on the NMM data asset, so it suffers "
			 "the same race as the message listeners. The canonical place to start the load is "
			 "OnDataAssetLoaded."),
		CodeOnly.Contains(TEXT("AsyncLoadGameFromSlot"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — B2: BeginPlay clears the stale MenuUnloaded message.
//
// The spec: "Not clearing the stale MenuUnloaded message on init — fires on
// the newly-bound listener immediately." MenuUnloaded is broadcast on
// OnUnregister (Test 3). The global message subsystem caches the most recent
// broadcast per tag so late subscribers can read state on subscription;
// without an explicit clear at init, the cached message from the *previous*
// session replays on the freshly-bound listener as soon as OnDataAssetLoaded
// subscribes to anything. That replay would tell the component the menu had
// just been unloaded — at the exact moment it just *loaded*.
//
// The canonical encoding is
//   UGlobalMessageSubsystem::ClearCachedMessages(NmmGameplayTags::Event::MenuUnloaded, this)
// in BeginPlay, BEFORE OnDataAssetLoaded ever runs.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_BeginPlayClearsStaleMenuUnloaded,
	"Bomber.NMMPlayerControllerComponent.BeginPlayClearsStaleMenuUnloaded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_BeginPlayClearsStaleMenuUnloaded::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::BeginPlay"));
	if (!TestTrue(
			TEXT("B2: NMMPlayerControllerComponent.cpp must define BeginPlay with a body."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// BeginPlay must clear cached messages — and the call must be against the
	// MenuUnloaded tag specifically. A generic ClearCachedMessages call against
	// a different tag (e.g. MenuStateChanged) or an unscoped clear-all sweep
	// would not match the spec's failure-mode signal.
	TestTrue(
		TEXT("B2: BeginPlay must call UGlobalMessageSubsystem::ClearCachedMessages. The cached-"
			 "message store on the global message subsystem replays the most recent broadcast "
			 "per tag to late subscribers; without an explicit clear, the previous session's "
			 "MenuUnloaded broadcast fires on the newly-bound listener the instant "
			 "OnDataAssetLoaded subscribes — the spec's named failure mode: 'fires on the "
			 "newly-bound listener immediately.'"),
		CodeOnly.Contains(TEXT("ClearCachedMessages"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B2: BeginPlay's ClearCachedMessages call must target the MenuUnloaded tag "
			 "specifically (NmmGameplayTags::Event::MenuUnloaded). The clear's whole purpose is "
			 "to stop the prior-session unload event from replaying on the fresh listener; a "
			 "clear against a different tag (or a tag-less clear-all) would either miss the "
			 "stale event or sweep messages other listeners legitimately depend on."),
		CodeOnly.Contains(TEXT("MenuUnloaded"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — B3: SetCinematicInputContextEnabled disables ALL game-state input
//          contexts before enabling the cinematic ones.
//
// The spec: "Not disabling all contexts before enabling cinematic ones —
// gameplay inputs leak during cinematics." A cinematic spot is supposed to
// own input completely while it plays: arrow keys, bomb-place, ability cast
// must all be inert. The component's contract is that on the enable path,
// every game-state-driven context (the entire FBmrGameStateTag::ParentTag
// family) is first cleared on the player controller, and only then are the
// cinematic-specific contexts added. The disable path is asymmetric — it
// only removes the cinematic contexts because the game-state-driven ones
// are managed by the bitmask path (SetManagedInputContextsEnabled) and will
// re-enable themselves on the next state change.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_CinematicEnableDisablesAllFirst,
	"Bomber.NMMPlayerControllerComponent.CinematicEnableDisablesAllFirst",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_CinematicEnableDisablesAllFirst::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMPlayerControllerComponent::SetCinematicInputContextEnabled"));
	if (!TestTrue(
			TEXT("B3: NMMPlayerControllerComponent.cpp must define SetCinematicInputContextEnabled "
				 "with a body — the start/ stub is empty; the asymmetric enable/disable wiring is "
				 "what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The enable path must call SetAllInputContextsEnabled(false, ...) — that
	// is the broad-scope disable that clears every game-state-driven context.
	// The first positional argument is the enable flag (false to disable);
	// the second is the tag scope to disable (the parent tag of the game-
	// state context family). A re-impl that calls only `SetupInputContexts`
	// for the cinematic family is the named failure mode.
	TestTrue(
		TEXT("B3: SetCinematicInputContextEnabled's enable path must call "
			 "SetAllInputContextsEnabled(false, …) to clear every game-state-driven context "
			 "before adding the cinematic ones. The spec's named failure mode: 'Not disabling "
			 "all contexts before enabling cinematic ones — gameplay inputs leak during "
			 "cinematics.' A re-impl that just appends the cinematic contexts leaves the "
			 "player's movement / bomb-place actions live behind the cinematic UI."),
		CodeOnly.Contains(TEXT("SetAllInputContextsEnabled"), ESearchCase::CaseSensitive));

	// The disable target must be the game-state parent tag — FBmrGameStateTag::
	// ParentTag — so the disable sweeps the whole family in one call. A
	// disable that names individual leaves (Menu, GameStarting, …) would
	// miss any future state added under the parent.
	TestTrue(
		TEXT("B3: SetCinematicInputContextEnabled's disable-all call must scope by "
			 "FBmrGameStateTag::ParentTag — the parent tag whose subtree covers every game-"
			 "state-driven context. Naming individual leaves (Menu, GameStarting, In-Game) "
			 "leaks any new state added later, and the family is exactly what the spec calls "
			 "out as needing to be cleared."),
		CodeOnly.Contains(TEXT("ParentTag"), ESearchCase::CaseSensitive));

	// Order matters: the disable-all must precede the cinematic enable. The
	// canonical body fetches the cinematic contexts via
	// UNMMDataAsset::Get().GetInputContexts(FNmmStateTag::Cinematic, ...) and
	// then calls SetAllInputContextEnabled(bEnable, CinematicContexts). Walk
	// the offsets of the two calls and assert the disable-all appears first.
	const int32 DisableAllIdx = FindOffsetIn(CodeOnly, TEXT("SetAllInputContextsEnabled"));
	const int32 GetContextsIdx = FindOffsetIn(CodeOnly, TEXT("GetInputContexts"));
	if (DisableAllIdx != INDEX_NONE && GetContextsIdx != INDEX_NONE)
	{
		TestTrue(
			TEXT("B3: The disable-all (SetAllInputContextsEnabled(false, ParentTag)) must "
				 "precede the cinematic-contexts fetch / enable. A body that adds the cinematic "
				 "contexts first and then disables-all would clobber its own additions; a body "
				 "that calls them in either order based on bEnable would skip the disable in "
				 "the disable path (which is the intended asymmetry — the disable path only "
				 "needs to remove the cinematic contexts)."),
			DisableAllIdx < GetContextsIdx);
	}

	// The enable path's gate must be the bEnable parameter — the disable
	// path of SetCinematicInputContextEnabled MUST NOT call the broad-scope
	// disable (the spec calls this asymmetry out explicitly). The canonical
	// encoding is `if (bEnable) { MyPC.SetAllInputContextsEnabled(false,
	// ParentTag); }` — a single conditional that gates only the disable-all,
	// not the cinematic-contexts setup that runs in both directions.
	TestTrue(
		TEXT("B3: The disable-all branch must be gated on bEnable (`if (bEnable)` or "
			 "equivalent). The disable path of SetCinematicInputContextEnabled must NOT clear "
			 "every context — only the enable path does, and only because the game-state "
			 "contexts otherwise leak into the cinematic. The disable path leaves the game-"
			 "state contexts alone; they re-arm via the managed-contexts path."),
		CodeOnly.Contains(TEXT("if (bEnable"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("if(bEnable"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("if (bEnable)"), ESearchCase::CaseSensitive));

	// Both branches must still reach the cinematic-context surface — the
	// disable path calls the same function with bEnable=false to *remove*
	// the cinematic contexts. The function takes a list and a boolean,
	// spelled SetAllInputContextEnabled (note: singular `Context`, not the
	// `Contexts` of the broad-scope call). A re-impl that drops this call
	// would leave the cinematic contexts enabled forever once added.
	TestTrue(
		TEXT("B3: Both branches must call SetAllInputContextEnabled(bEnable, CinematicContexts) "
			 "on the cinematic-context list — the enable path adds them, the disable path "
			 "removes them. Without this call the disable path is a no-op and cinematic "
			 "contexts stay enabled after the cinematic ends."),
		CodeOnly.Contains(TEXT("SetAllInputContextEnabled"), ESearchCase::CaseSensitive));

	// The list of cinematic contexts must come from the NMM data asset's
	// cinematic family — UNMMDataAsset::Get().GetInputContexts(
	// FNmmStateTag::Cinematic, ...). Reading from any other source (a
	// hard-coded array, an unrelated state tag) drifts as the data asset
	// adds or removes cinematic contexts.
	TestTrue(
		TEXT("B3: The cinematic-context list must come from "
			 "UNMMDataAsset::Get().GetInputContexts(FNmmStateTag::Cinematic, …). A hard-coded "
			 "list or one that reads a different state would drift the moment a new cinematic "
			 "context is authored in the data asset."),
		CodeOnly.Contains(TEXT("GetInputContexts"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("Cinematic"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — B4: OnUnregister broadcasts MenuUnloaded BEFORE cleanup and
//          forwards to the server on non-authority clients.
//
// Two failures live here:
//   * Order: cleanup before broadcast means downstream listeners reading
//     state during their MenuUnloaded handler see torn-down internals
//     (input contexts already removed, music subsystem state cleared, save
//     game already destroyed). The broadcast must precede every cleanup
//     step.
//   * Authority: on a non-authority client, OnUnregister fires only on the
//     client; the server's UNMMPlayerControllerComponent for the *same*
//     controller pair does not run its OnUnregister at the same time. The
//     server-side StateTree that listens on MenuUnloaded would never see
//     the client leave the menu without an explicit ServerBroadcastMessage
//     relay. The spec's named failure mode: "Not forwarding MenuUnloaded to
//     the server — server state tree diverges from client."
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_OnUnregisterBroadcastsAndForwardsToServer,
	"Bomber.NMMPlayerControllerComponent.OnUnregisterBroadcastsAndForwardsToServer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_OnUnregisterBroadcastsAndForwardsToServer::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnUnregister"));
	if (!TestTrue(
			TEXT("B4: NMMPlayerControllerComponent.cpp must define OnUnregister with a body — "
				 "the start/ stub is empty; the broadcast / forward / cleanup sequence is what "
				 "was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The body must broadcast a MenuUnloaded global message. The canonical
	// spelling builds an FGameplayEventData with EventTag set to
	// NmmGameplayTags::Event::MenuUnloaded and calls
	// UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData).
	TestTrue(
		TEXT("B4: OnUnregister must broadcast a MenuUnloaded global message via "
			 "UGlobalMessageSubsystem::BroadcastGlobalMessage. Without that broadcast, "
			 "downstream listeners (other plugin components, UI VMs, audio cue subsystem) "
			 "never see the menu leaving and stay wedged in their last-seen state."),
		CodeOnly.Contains(TEXT("BroadcastGlobalMessage"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("MenuUnloaded"), ESearchCase::CaseSensitive));

	// The broadcast must precede the cleanup. Cleanup signals to look for:
	// removing input contexts (`RemoveInputContexts`), destroying the sound
	// (`DestroySingleSound2D`), or releasing the save data (`SaveGameData =
	// nullptr` / `ConditionalBeginDestroy`). The canonical encoding does the
	// broadcast as the first user-visible side-effect, followed by every
	// cleanup step.
	const int32 BroadcastIdx = FindOffsetIn(CodeOnly, TEXT("BroadcastGlobalMessage"));
	const int32 RemoveContextsIdx = FindOffsetIn(CodeOnly, TEXT("RemoveInputContexts"));
	const int32 DestroySoundIdx = FindOffsetIn(CodeOnly, TEXT("DestroySingleSound2D"));
	const int32 SaveNullIdx = FindOffsetIn(CodeOnly, TEXT("SaveGameData = nullptr"));

	// At least one of the cleanup markers must appear AND it must appear
	// strictly after the broadcast. A re-impl that destroys the save game
	// before broadcasting (which then attempts to read SaveGameData's state
	// from a teardown handler) is the named ordering failure.
	if (BroadcastIdx != INDEX_NONE)
	{
		bool bAnyCleanup = false;
		bool bAllAfterBroadcast = true;
		for (int32 Idx : {RemoveContextsIdx, DestroySoundIdx, SaveNullIdx})
		{
			if (Idx == INDEX_NONE)
			{
				continue;
			}
			bAnyCleanup = true;
			if (Idx < BroadcastIdx)
			{
				bAllAfterBroadcast = false;
			}
		}
		TestTrue(
			TEXT("B4: OnUnregister must perform at least one cleanup step (input-context "
				 "removal, sound destruction, or save-data release) so it actually tears down "
				 "the component's transient state. A broadcast-only OnUnregister leaks input "
				 "bindings, sounds, and save objects across menu sessions."),
			bAnyCleanup);

		TestTrue(
			TEXT("B4: OnUnregister must broadcast MenuUnloaded BEFORE any cleanup (input-context "
				 "removal, sound destruction, or save-data release). The spec: 'On unregister, "
				 "the MenuUnloaded event must broadcast before any cleanup — downstream "
				 "subsystems still need to read state.' A handler that reads SaveGameData / "
				 "input-context state during its MenuUnloaded reaction would see a torn-down "
				 "component if the cleanup ran first."),
			bAllAfterBroadcast);
	}

	// Non-authority forward to the server. The named failure mode: "Not
	// forwarding MenuUnloaded to the server — server state tree diverges
	// from client." The canonical encoding is
	//   if (!MyPC->HasAuthority()) { MyPC->ServerBroadcastMessage(EventData); }
	TestTrue(
		TEXT("B4: OnUnregister must consult HasAuthority and call ServerBroadcastMessage on "
			 "the non-authority path. The spec's named failure mode: 'Not forwarding "
			 "MenuUnloaded to the server — server state tree diverges from client.' The server "
			 "doesn't see the client's OnUnregister directly; without the explicit RPC relay, "
			 "the server's StateTree stays in Menu after the client has left."),
		CodeOnly.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("ServerBroadcastMessage"), ESearchCase::CaseSensitive));

	// The HasAuthority check must be a negation (server-relay applies when
	// the controller is NOT the authority — i.e. on a client). A re-impl
	// that calls ServerBroadcastMessage from authority would send a
	// server-only RPC originating from the server, which the engine ignores
	// (no remote to relay to) but also masks that the gating was wrong.
	TestTrue(
		TEXT("B4: The ServerBroadcastMessage call must gate on `!HasAuthority` (or an "
			 "equivalent inversion). Server-side OnUnregister already runs on the server; the "
			 "RPC relay only matters when the OnUnregister handler is the *client's* copy."),
		CodeOnly.Contains(TEXT("!MyPC->HasAuthority"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("!HasAuthority"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("HasAuthority() == false"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("false == MyPC->HasAuthority"), ESearchCase::CaseSensitive));

	// Cleanup must include Super::OnUnregister() — the base class still owns
	// component registration bookkeeping. The canonical encoding puts it as
	// the very last line so cleanup runs against a still-valid component
	// state.
	TestTrue(
		TEXT("B4: OnUnregister must call Super::OnUnregister() — the base class still owns "
			 "component-registration bookkeeping; skipping it would leave the engine treating "
			 "the component as registered after teardown."),
		CodeOnly.Contains(TEXT("Super::OnUnregister"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — B5: OnAsyncLoadGameFromSlotCompleted's no-slot fallback passes
//          the ORIGINAL parameter to SetSaveGameData, not the freshly-
//          created save.
//
// The spec's named failure mode: "Setting the freshly-created save as the
// active save in the no-slot fallback — skips persistence for next
// session." Subtle: when there is no save slot yet, the canonical body
// creates a fresh UNMMSaveGameData and calls SaveDataAsync() on it (so the
// slot exists for the *next* session), but then passes the original
// `SaveGame` parameter (which is null in this case) into
// SetSaveGameData. SetSaveGameData casts; the cast yields null; the
// `!InSaveGameData` early-out fires; SaveGameData remains nullptr for
// this session. Next session, the slot exists and AsyncLoadGameFromSlot
// returns a real save.
//
// A re-impl that passes the freshly-created object to SetSaveGameData
// short-circuits this persistence dance: SaveGameData is set to the fresh
// in-memory object, and because the cached-save check skips re-loading,
// the save written to disk is never read back — every restart starts from
// a blank save until the user explicitly resaves.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_NoSlotFallbackPassesOriginalParameter,
	"Bomber.NMMPlayerControllerComponent.NoSlotFallbackPassesOriginalParameter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_NoSlotFallbackPassesOriginalParameter::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// Try both the BlueprintNativeEvent _Implementation suffix and the bare
	// name in case a re-impl drops the BNE wrapper.
	FString Body =
		ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnAsyncLoadGameFromSlotCompleted_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnAsyncLoadGameFromSlotCompleted"));
	}
	if (!TestTrue(
			TEXT("B5: NMMPlayerControllerComponent.cpp must define OnAsyncLoadGameFromSlotCompleted "
				 "(or its _Implementation BNE body) with a body — the start/ stub is empty; the "
				 "no-slot fallback is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The fallback path must create a fresh UNMMSaveGameData object — the
	// canonical encoding is `NewObject<UNMMSaveGameData>(this)`.
	TestTrue(
		TEXT("B5: OnAsyncLoadGameFromSlotCompleted's no-slot fallback must create a fresh "
			 "UNMMSaveGameData via NewObject<UNMMSaveGameData>(...). Without that creation, "
			 "the missing-slot case has nothing to persist and the next session also fails to "
			 "find a slot."),
		CodeOnly.Contains(TEXT("NewObject<UNMMSaveGameData>"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("NewObject< UNMMSaveGameData >"), ESearchCase::CaseSensitive));

	// The fresh save must be persisted via SaveDataAsync — that is what
	// makes the slot exist for the next session. A re-impl that creates
	// the object and never writes it produces the named failure: 'skips
	// persistence for next session.'
	TestTrue(
		TEXT("B5: The freshly-created save must be persisted via SaveDataAsync() so the slot "
			 "exists for the next session. The spec's named failure mode: 'Setting the "
			 "freshly-created save as the active save in the no-slot fallback — skips "
			 "persistence for next session.' The persistence step is what makes the fallback "
			 "a fallback rather than a permanent in-memory hack."),
		CodeOnly.Contains(TEXT("SaveDataAsync"), ESearchCase::CaseSensitive));

	// The terminal SetSaveGameData call must take the ORIGINAL parameter
	// (`SaveGame`), not the freshly-created `InSaveGameData`. SetSaveGameData
	// performs a cast — when SaveGame is null (the fallback case), the cast
	// produces null and the early-out in SetSaveGameData fires, leaving
	// SaveGameData nullptr for this session. Passing the fresh object
	// short-circuits that early-out and the next session never reloads
	// the persisted slot.
	TestTrue(
		TEXT("B5: The terminal SetSaveGameData call must pass the ORIGINAL `SaveGame` parameter "
			 "(not `InSaveGameData`). Subtle but load-bearing: SetSaveGameData casts; passing "
			 "the fresh in-memory object short-circuits the cast's early-out and assigns it as "
			 "the active save. The session then reads the in-memory object instead of the "
			 "persisted slot, so the slot written to disk is never read back. The canonical "
			 "encoding is `SetSaveGameData(SaveGame)` AT THE END of the function."),
		CodeOnly.Contains(TEXT("SetSaveGameData(SaveGame)"), ESearchCase::CaseSensitive));

	// Negative: the terminal call must not pass `InSaveGameData` — the exact
	// anti-pattern the spec calls out. (We look for the wrong spelling so a
	// re-impl that accidentally produces it fails this assertion.)
	const bool bWrongTerminalArg =
		CodeOnly.Contains(TEXT("SetSaveGameData(InSaveGameData)"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B5: The terminal SetSaveGameData must NOT pass `InSaveGameData` — that propagates "
			 "the freshly-created in-memory save as the active save and skips persistence for "
			 "the next session. The fresh save exists only to write the slot for next time; the "
			 "active save for THIS session is the cast result of the original parameter, which "
			 "in the fallback case is null."),
		bWrongTerminalArg);

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Where the listener binding actually lives.
//
// The B1 / Test 1 negative is "no listeners in BeginPlay." This test is the
// positive counterpart: the binding lives in OnDataAssetLoaded, the BNE the
// DAL subsystem invokes once the NMM data asset finishes resolving. That
// callback must wire ALL of:
//   * GameState_Changed (drives the music routing — Test 7),
//   * MenuStateChanged  (drives the cinematic suppress / input-context update),
//   * Widgets-initialized (re-triggers the managed-context apply once UI is up),
//   * The async save-game load (kicks off the slot fetch).
//
// A re-impl that wires only a subset (e.g. binds MenuStateChanged but
// forgets GameState_Changed) leaves the menu without music; a re-impl that
// skips the widgets-initialized bind never recovers from the widget-not-
// ready short-circuit (Test 8) so input contexts stay missing for the
// entire session.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_OnDataAssetLoadedWiresEverything,
	"Bomber.NMMPlayerControllerComponent.OnDataAssetLoadedWiresEverything",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_OnDataAssetLoadedWiresEverything::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	FString Body =
		ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnDataAssetLoaded_Implementation"));
	if (Body.IsEmpty())
	{
		Body = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnDataAssetLoaded"));
	}
	if (!TestTrue(
			TEXT("G1: NMMPlayerControllerComponent.cpp must define OnDataAssetLoaded (or its "
				 "_Implementation BNE body) with a body — that is where the listener wiring, "
				 "save-load kickoff, and auto-camera-possess disable belong (NOT in BeginPlay; "
				 "see Test 1)."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Game-state listener must be wired here — drives music start/stop. The
	// canonical spelling is
	// `UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(
	//     BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged)`
	TestTrue(
		TEXT("G1: OnDataAssetLoaded must subscribe to BmrGameplayTags::Event::GameState_Changed "
			 "via UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage (or equivalent). "
			 "That listener is the only place that routes Menu→PlayMainMenuMusic and "
			 "GameStarting→StopMainMenuMusic; without it, the menu music never starts."),
		CodeOnly.Contains(TEXT("GameState_Changed"), ESearchCase::CaseSensitive)
			&& (CodeOnly.Contains(TEXT("CallOrStartListeningForGlobalMessage"), ESearchCase::CaseSensitive)
				|| CodeOnly.Contains(TEXT("StartListeningForGlobalMessage"), ESearchCase::CaseSensitive)));

	// Menu-state listener.
	TestTrue(
		TEXT("G1: OnDataAssetLoaded must subscribe to NmmGameplayTags::Event::MenuStateChanged. "
			 "That listener routes Cinematic-state entry to StopMainMenuMusic / SetIgnoreMoveInput "
			 "and updates the managed input contexts on every menu-state transition; without it "
			 "the menu state machine's view never reaches the input/audio stack."),
		CodeOnly.Contains(TEXT("MenuStateChanged"), ESearchCase::CaseSensitive));

	// Widgets-initialized bind. The canonical encoding uses the
	// BIND_ON_WIDGETS_INITIALIZED macro that wraps the widgets subsystem's
	// delegate; a re-impl that uses the underlying delegate directly is
	// fine as long as the binding exists.
	TestTrue(
		TEXT("G1: OnDataAssetLoaded must bind a widgets-initialized callback so the managed "
			 "input contexts re-arm once the UI is ready. SetManagedInputContextsEnabled short-"
			 "circuits when the main-menu widget is null (Test 8); without the widgets-"
			 "initialized re-trigger, the contexts stay missing for the rest of the session."),
		CodeOnly.Contains(TEXT("BIND_ON_WIDGETS_INITIALIZED"), ESearchCase::CaseSensitive)
			|| CodeOnly.Contains(TEXT("OnWidgetsInitialized"), ESearchCase::CaseSensitive));

	// Async save load. The canonical encoding builds a delegate and passes
	// it to USaveUtilsLibrary::AsyncLoadGameFromSlot with the NMM save-slot
	// metadata.
	TestTrue(
		TEXT("G1: OnDataAssetLoaded must kick off the async save-game load (USaveUtilsLibrary::"
			 "AsyncLoadGameFromSlot bound to OnAsyncLoadGameFromSlotCompleted). The save load "
			 "depends on slot metadata that lives on the now-resolved data asset; this is the "
			 "earliest correct place to start it."),
		CodeOnly.Contains(TEXT("AsyncLoadGameFromSlot"), ESearchCase::CaseSensitive)
			&& CodeOnly.Contains(TEXT("OnAsyncLoadGameFromSlotCompleted"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Music routing: Menu→Play, GameStarting→Stop. Cinematic entry also
//          suppresses move input and stops menu music.
//
// The spec splits music routing across two handlers:
//   * OnGameStateChanged reacts to the outer game-state machine — Menu state
//     starts the main-menu music, GameStarting stops it.
//   * OnNewMainMenuStateChanged reacts to the inner menu-state machine —
//     entering FNmmStateTag::Cinematic also stops the music AND suppresses
//     move input (the player shouldn't be able to walk around during a
//     character-spot cinematic).
//
// A re-impl that fuses the two — e.g. starts the music in
// OnNewMainMenuStateChanged — never plays the music on the first menu open
// because the menu-state handler doesn't fire until after the user navigates.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_MusicRoutingAndCinematicSuppress,
	"Bomber.NMMPlayerControllerComponent.MusicRoutingAndCinematicSuppress",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_MusicRoutingAndCinematicSuppress::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// --- Game-state routing -------------------------------------------------
	FString GameStateBody =
		ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnGameStateChanged_Implementation"));
	if (GameStateBody.IsEmpty())
	{
		GameStateBody = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnGameStateChanged"));
	}
	if (!TestTrue(
			TEXT("G7: NMMPlayerControllerComponent.cpp must define OnGameStateChanged (or its "
				 "_Implementation BNE body) with a body — that is the music router."),
			!GameStateBody.IsEmpty()))
	{
		return false;
	}

	const FString GameStateCode = StripComments(GameStateBody);

	TestTrue(
		TEXT("G7: OnGameStateChanged must route the Menu state to PlayMainMenuMusic. The spec's "
			 "Music section: 'The main-menu music plays when the game state enters Menu.' A "
			 "router that handles GameStarting but never Menu produces a silent menu."),
		GameStateCode.Contains(TEXT("PlayMainMenuMusic"), ESearchCase::CaseSensitive)
			&& GameStateCode.Contains(TEXT("Menu"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("G7: OnGameStateChanged must route the GameStarting state to StopMainMenuMusic. "
			 "Without that branch the menu music continues to play into the gameplay session, "
			 "competing with the gameplay music track."),
		GameStateCode.Contains(TEXT("StopMainMenuMusic"), ESearchCase::CaseSensitive)
			&& GameStateCode.Contains(TEXT("GameStarting"), ESearchCase::CaseSensitive));

	// The router must test against FBmrGameStateTag tags via HasTag (or a
	// matches-tag equivalent) — the payload is an FGameplayEventData whose
	// InstigatorTags is an FGameplayTagContainer. Reading a different field
	// (Instigator->GetState) bypasses the broadcast contract.
	const bool bUsesTagTest =
		GameStateCode.Contains(TEXT("HasTag"), ESearchCase::CaseSensitive)
		|| GameStateCode.Contains(TEXT("HasTagExact"), ESearchCase::CaseSensitive)
		|| GameStateCode.Contains(TEXT("MatchesTag"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("G7: OnGameStateChanged must consult the payload's InstigatorTags via a tag "
			 "containment test (HasTag / HasTagExact / MatchesTag). The payload is an "
			 "FGameplayEventData; reading a different field bypasses the broadcast contract "
			 "and the router never matches."),
		bUsesTagTest);

	// --- Menu-state cinematic suppression ----------------------------------
	FString MenuStateBody =
		ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnNewMainMenuStateChanged_Implementation"));
	if (MenuStateBody.IsEmpty())
	{
		MenuStateBody = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnNewMainMenuStateChanged"));
	}
	if (!TestTrue(
			TEXT("G7: NMMPlayerControllerComponent.cpp must define OnNewMainMenuStateChanged (or "
				 "its _Implementation BNE body) with a body — that is the cinematic-entry "
				 "router that suppresses move input and stops menu music."),
			!MenuStateBody.IsEmpty()))
	{
		return false;
	}

	const FString MenuStateCode = StripComments(MenuStateBody);

	TestTrue(
		TEXT("G7: OnNewMainMenuStateChanged must suppress move input on cinematic entry "
			 "(SetIgnoreMoveInput(true)). The spec: 'Cinematic mode also suppresses move "
			 "input.' Without this, the player can walk around during the cinematic."),
		MenuStateCode.Contains(TEXT("SetIgnoreMoveInput"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("G7: OnNewMainMenuStateChanged must stop menu music on cinematic entry "
			 "(StopMainMenuMusic). The spec: 'Cinematic mode also … stops the menu music.' "
			 "Without this, menu music plays over the cinematic's own audio track."),
		MenuStateCode.Contains(TEXT("StopMainMenuMusic"), ESearchCase::CaseSensitive));

	// The handler must also call SetManagedInputContextsEnabled (the bitmask
	// path) and SetCinematicInputContextEnabled (the cinematic path) on
	// every state transition, both gated on the inferred cinematic bool.
	TestTrue(
		TEXT("G7: OnNewMainMenuStateChanged must invoke both the managed-contexts and "
			 "cinematic-context updates on every state transition (SetManagedInputContextsEnabled "
			 "and SetCinematicInputContextEnabled). They are the two parallel input-context "
			 "paths the spec describes: state-driven contexts via the managed path, "
			 "cinematic-only contexts via the cinematic path."),
		MenuStateCode.Contains(TEXT("SetManagedInputContextsEnabled"), ESearchCase::CaseSensitive)
			&& MenuStateCode.Contains(TEXT("SetCinematicInputContextEnabled"), ESearchCase::CaseSensitive));

	// The handler must consult FNmmStateTag::Cinematic — the cinematic branch
	// is what gates the move-input suppression and music stop.
	TestTrue(
		TEXT("G7: OnNewMainMenuStateChanged must reference FNmmStateTag::Cinematic so the "
			 "cinematic-entry path is correctly identified. A handler that runs the "
			 "suppression for every state would freeze move input and silence music outside "
			 "of cinematics; a handler that never references Cinematic can't gate it at all."),
		MenuStateCode.Contains(TEXT("Cinematic"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Widget readiness short-circuit.
//
// The spec: "The managed input context update short-circuits if the main
// menu widget is not yet initialized." UNMMUtils::GetMainMenuWidget()
// returns nullptr until the menu UMG widget is constructed and added to the
// viewport; the input-context apply path reads input action lists that the
// widget pre-populates, so running the apply against a null widget either
// reads a partially-initialized data asset or skips the side-effect the
// caller depends on. The canonical encoding is a top-of-function early-out:
//   if (UNMMUtils::GetMainMenuWidget() == nullptr) { return; }
//
// The widget-ready event (OnWidgetsInitialized) is the re-trigger that
// recovers from the early-out once the widget is up. The combination —
// early-out plus re-trigger — is what makes the state-change-then-widget-
// ready race resolve cleanly.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_ManagedContextsShortCircuitOnNullWidget,
	"Bomber.NMMPlayerControllerComponent.ManagedContextsShortCircuitOnNullWidget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_ManagedContextsShortCircuitOnNullWidget::RunTest(const FString& Parameters)
{
	using namespace NmmPlayerControllerComponentTests;

	const FString Source = LoadProjectFile(this, ComponentCppRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(
		Source, TEXT("UNMMPlayerControllerComponent::SetManagedInputContextsEnabled"));
	if (!TestTrue(
			TEXT("G8: NMMPlayerControllerComponent.cpp must define SetManagedInputContextsEnabled "
				 "with a body — the start/ stub is empty; the widget-readiness short-circuit is "
				 "what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The function must consult UNMMUtils::GetMainMenuWidget() at top — the
	// short-circuit is meaningless if the apply has already run side effects.
	TestTrue(
		TEXT("G8: SetManagedInputContextsEnabled must consult UNMMUtils::GetMainMenuWidget() "
			 "and short-circuit when it returns nullptr. The widget pre-populates the input-"
			 "action wiring the apply depends on; running the apply against a null widget "
			 "either no-ops silently or reads partially-initialized data, depending on the "
			 "code path that gets there first."),
		CodeOnly.Contains(TEXT("GetMainMenuWidget"), ESearchCase::CaseSensitive));

	// The null check must be followed by an early-out — the canonical
	// encoding is `return;` (no further work). A re-impl that uses the
	// widget value for a branch but then still runs the apply on the
	// null path is the named regression.
	TestTrue(
		TEXT("G8: SetManagedInputContextsEnabled's null-widget branch must return early. The "
			 "spec: 'The managed input context update short-circuits if the main menu widget "
			 "is not yet initialized.' An if/else that runs the apply along a different path "
			 "would still hit the widget-dependent code; the canonical encoding is `if (… == "
			 "nullptr) { return; }`."),
		CodeOnly.Contains(TEXT("return"), ESearchCase::CaseSensitive));

	// The re-trigger lives in OnWidgetsInitialized. Without the re-trigger,
	// the early-out is permanent for any state-change that happened before
	// the widget came up.
	FString OnWidgetsBody =
		ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnWidgetsInitialized_Implementation"));
	if (OnWidgetsBody.IsEmpty())
	{
		OnWidgetsBody = ExtractMemberBody(Source, TEXT("UNMMPlayerControllerComponent::OnWidgetsInitialized"));
	}
	if (TestTrue(
			TEXT("G8: NMMPlayerControllerComponent.cpp must define OnWidgetsInitialized (or its "
				 "_Implementation BNE body) with a body — that is the re-trigger that recovers "
				 "from the widget-not-ready short-circuit."),
			!OnWidgetsBody.IsEmpty()))
	{
		const FString OnWidgetsCode = StripComments(OnWidgetsBody);
		TestTrue(
			TEXT("G8: OnWidgetsInitialized must re-call SetManagedInputContextsEnabled — that is "
				 "the recovery path for state changes that fired before the widget was up. The "
				 "canonical encoding passes the current menu state (UNMMUtils::GetMainMenuState()) "
				 "so the apply is for whatever state the menu is in *now*."),
			OnWidgetsCode.Contains(TEXT("SetManagedInputContextsEnabled"), ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 9 — Runtime UClass surface: the plugin's reflected class is
//          registered with the contract the spec depends on.
//
// The NewMainMenu plugin's runtime module loads on engine startup (it's
// listed in Bomber.uproject with EnabledByDefault=true; only its Game-
// Feature *activation* is deferred via ExplicitlyLoaded=true). That means
// UClass / UFUNCTION / UPROPERTY reflection data is registered before any
// test runs, even though Bomber.Build.cs does not name the plugin as a
// dependency (so we still cannot include the plugin's headers).
//
// This test reaches the contract via the engine reflection database
// instead of via source-text scanning. It catches API-rename regressions
// that source scans alone would miss: a re-impl that renames a UFUNCTION
// breaks every Blueprint caller (the menu blueprints invoke this surface
// by name) and every C++ caller that uses ProcessEvent.
//
// All checks degrade gracefully when the plugin module is not registered
// in the test process (some CI configurations strip Game-Feature plugins
// out of the editor target). In that mode the source-inspection tests
// above continue to enforce the same contract structurally.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNmmPlayerControllerComponent_RuntimeUClassSurface,
	"Bomber.NMMPlayerControllerComponent.RuntimeUClassSurface",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNmmPlayerControllerComponent_RuntimeUClassSurface::RunTest(const FString& Parameters)
{
	UClass* ComponentClass =
		FindObject<UClass>(nullptr, TEXT("/Script/NewMainMenu.NMMPlayerControllerComponent"));
	if (!ComponentClass)
	{
		AddInfo(TEXT("NewMainMenu plugin's runtime module is not registered in this process — "
					 "runtime UClass surface check is skipped. The source-inspection tests "
					 "above continue to enforce the same contract."));
		return true;
	}

	// The class must derive from UActorComponent — the controller mounts it
	// as an actor component, not a UObject or USceneComponent (which would
	// fail to compile against the AddComponentByClass path the plugin uses).
	UClass* ActorComponentClass = FindObject<UClass>(nullptr, TEXT("/Script/Engine.ActorComponent"));
	if (TestNotNull(
			TEXT("G9: Engine's UActorComponent must be discoverable via reflection — sanity check."),
			ActorComponentClass))
	{
		TestTrue(
			TEXT("G9: UNMMPlayerControllerComponent must derive from UActorComponent. The "
				 "controller mounts this via AddComponentByClass-style flows; a different base "
				 "(UObject, USceneComponent, UPawnComponent) would either fail registration or "
				 "subscribe to the wrong tick group."),
			ComponentClass->IsChildOf(ActorComponentClass));
	}

	// The five public UFUNCTIONs the spec names (input contexts, mouse,
	// save data, music) must be reflected so the menu blueprints and the
	// rest of the C++ surface can call them.
	const TCHAR* ExpectedUFunctions[] = {
		TEXT("SetSaveGameData"),
		TEXT("SetCinematicInputContextEnabled"),
		TEXT("SetCinematicMouseVisibilityEnabled"),
		TEXT("SetManagedInputContextsEnabled"),
		TEXT("PlayMainMenuMusic"),
		TEXT("StopMainMenuMusic"),
	};
	for (const TCHAR* FnName : ExpectedUFunctions)
	{
		UFunction* Fn = ComponentClass->FindFunctionByName(FName(FnName));
		TestNotNull(
			*FString::Printf(
				TEXT("G9: UNMMPlayerControllerComponent must expose %s as a UFUNCTION at "
					 "runtime — the menu blueprints and the rest of the plugin call this surface "
					 "by name. Renaming or removing the function breaks every blueprint that "
					 "invokes it (the bind is by FName, not by symbol)."),
				FnName),
			Fn);
	}

	// The four BlueprintNativeEvents are also UFUNCTIONs (BNE wraps them as
	// reflected functions). The data-asset arrival callback in particular is
	// invoked by the DAL subsystem via ProcessEvent.
	const TCHAR* ExpectedBNEs[] = {
		TEXT("OnDataAssetLoaded"),
		TEXT("OnGameStateChanged"),
		TEXT("OnNewMainMenuStateChanged"),
		TEXT("OnWidgetsInitialized"),
		TEXT("OnAsyncLoadGameFromSlotCompleted"),
	};
	for (const TCHAR* FnName : ExpectedBNEs)
	{
		UFunction* Fn = ComponentClass->FindFunctionByName(FName(FnName));
		TestNotNull(
			*FString::Printf(
				TEXT("G9: UNMMPlayerControllerComponent must expose %s as a UFUNCTION at "
					 "runtime — the BlueprintNativeEvent surface the data-asset / global-message "
					 "subsystems invoke by reflection. A re-impl that drops the BNE attribute "
					 "would leave the engine unable to dispatch the callback at all."),
				FnName),
			Fn);
	}

	// SaveGameData UPROPERTY must exist as an object pointer — the cached
	// active save the component owns. A re-impl that drops the UPROPERTY
	// flag leaves the field outside the reflection database; the GC can't
	// track it and the BNE _Implementation can't read it back.
	const FObjectProperty* SaveGameDataProp = CastField<FObjectProperty>(
		ComponentClass->FindPropertyByName(FName(TEXT("SaveGameData"))));
	TestNotNull(
		TEXT("G9: UNMMPlayerControllerComponent must declare SaveGameData as an object "
			 "UPROPERTY — the GC must track the cached save and the Blueprint surface "
			 "(GetSaveGameData / SetSaveGameData) must reach it via reflection."),
		SaveGameDataProp);

	return true;
}
