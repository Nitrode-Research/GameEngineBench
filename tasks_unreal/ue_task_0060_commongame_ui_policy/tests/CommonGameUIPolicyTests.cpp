#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"        // GEngine
#include "Engine/GameInstance.h"
#include "CommonLocalPlayer.h"

// Open up private members of both classes for this TU. We only READ/WRITE data through the
// macro (the policy's RootViewportLayouts and the manager's CurrentPolicy). We never CALL a
// private/protected method here: under `#define private public` a private method's MSVC
// decorated name mangles as public and fails to link against its privately-exported
// definition. The discriminating call instead goes through the manager's genuinely PUBLIC
// NotifyPlayerDestroyed (whose mangling is unaffected).
#define protected public
#define private public
#include "GameUIPolicy.h"
#include "GameUIManagerSubsystem.h"
#undef private
#undef protected

#include "CommonGameUIPolicyTestSupport.h" // concrete UTestGameUIPolicy / UTestPrimaryGameLayout

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test for ue_task_0060 (CommonGame UI Policy).
//
// EDITABLE FILE UNDER TEST (observed): GameUIPolicy.cpp. The discriminating behavior is
// NotifyPlayerDestroyed: the solution removes the destroyed player's root-layout ownership
// from RootViewportLayouts (spec: "Destroying a player cleans up layout ownership ...
// without leaking stale UI state"); the gutted start/ leaves the entry in place.
//
// We drive it through the manager's PUBLIC NotifyPlayerDestroyed (a non-editable shim that
// forwards to the editable policy method) so the test never references a private symbol. The
// policy and root layout are synthesized concrete subclasses (see the support header) because
// both CommonGame base classes are Abstract and this project ships no concrete policy. The
// policy's viewport-removal helper is a no-op for a layout that was never shown (it early-outs
// on an invalid cached Slate widget), so a bare, never-constructed layout is safe.
//
// Every other gutted behavior (NotifyPlayerAdded, SuspendInputForPlayer, PushContentToLayer,
// ...) gates on a live UI-manager/input subsystem or a real widget and is not observable in a
// headless C++ test; this RootViewportLayouts mutation is the one reachable, deterministic one.
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonGameUIPolicyDestroyTest,
	"CommonGame.UIPolicy.notify_player_destroyed_removes_layout_ownership",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCommonGameUIPolicyDestroyTest::RunTest(const FString&)
{
	// Build the Outer chain the policy's ClassWithin requires (GameInstance -> UIManager ->
	// Policy), plus a destroyed-able player and a never-shown root layout.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UGameUIManagerSubsystem* UIManager = NewObject<UTestGameUIManagerSubsystem>(GameInstance);
	UTestGameUIPolicy* Policy = NewObject<UTestGameUIPolicy>(UIManager);
	// UCommonLocalPlayer is ClassWithin=Engine, so it must be outered to the engine.
	UCommonLocalPlayer* Player = NewObject<UCommonLocalPlayer>(GEngine);
	UTestPrimaryGameLayout* Layout = NewObject<UTestPrimaryGameLayout>(GetTransientPackage());

	if (!TestNotNull(TEXT("policy must be constructible"), Policy) ||
		!TestNotNull(TEXT("local player must be constructible"), Player) ||
		!TestNotNull(TEXT("root layout must be constructible"), Layout))
	{
		return true;
	}

	// Wire the manager to the policy (set the private member directly rather than calling the
	// protected SwitchToPolicy), and give the policy ownership of one player's root layout.
	UIManager->CurrentPolicy = Policy;
	Policy->RootViewportLayouts.Add(FRootViewportLayoutInfo(Player, Layout, false));
	if (!TestEqual(TEXT("precondition: policy owns exactly one root layout"),
		Policy->RootViewportLayouts.Num(), 1))
	{
		return true;
	}

	// Route through the manager's PUBLIC NotifyPlayerDestroyed — a non-editable shim that
	// delegates to the editable policy method (start/ guts it; the solution removes the entry).
	UIManager->NotifyPlayerDestroyed(Player);

	// The solution removes the destroyed player's layout ownership; the gutted start/ no-ops
	// and leaks the entry.
	TestEqual(TEXT("NotifyPlayerDestroyed must remove the destroyed player's root layout ownership"),
		Policy->RootViewportLayouts.Num(), 0);

	// The removed player's root layout must be parked as dormant on the way out. The test still
	// holds the layout object (it survives the ownership removal). Stock CommonGame only marks a
	// layout dormant in the SingleToggle secondary-handoff branch — not on this primary-mode
	// removal — so a stock reconstruction leaves the layout active (IsDormant() == false).
	TestTrue(TEXT("NotifyPlayerDestroyed must park the removed player's root layout as dormant"),
		Layout->IsDormant());

	// The owner-loss park also stamps this project's dormancy classification on the removed
	// layout. Stock CommonGame carries no such classification, so a reconstruction (even one that
	// parks the layout dormant) leaves the reason at its default 0 and fails here.
	TestEqual(TEXT("NotifyPlayerDestroyed must stamp this project's owner-loss dormancy classification"),
		Layout->DormancyReason, 7);

	return true;
}
