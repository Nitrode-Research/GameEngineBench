#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "CommonUserBasicPresence.h"
#include "CommonSessionSubsystem.h" // ECommonSessionInformationState

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test for ue_task_0059 (CommonUser Auth And Presence).
//
// EDITABLE FILE UNDER TEST (observed): CommonUserBasicPresence.cpp. The discriminating,
// backend-free behavior is SessionStateToBackendKey(ECommonSessionInformationState): the
// solution maps each session state to its configured PresenceStatus* backend key; start/
// is gutted and returns an empty string for every state.
//
// WHY ONLY THIS: the rest of the auth/presence behavior (login, privilege resolution,
// presence push, online-context resolution) runs through the EOS/online subsystem, which
// does not initialise in this headless / frozen-content project, so it is not observable
// here. The previous suite's CDO-default and method-pointer-existence checks pass on
// start/ too — the privilege cache and IsLoggedIn are NOT gutted in start/, and a declared
// method always has a non-null address — so they do not discriminate and are dropped. This
// is a pure C++ test: no world, PIE, or online backend.
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonUserPresenceBackendKeyTest,
	"CommonUser.AuthPresence.session_state_maps_to_configured_backend_key",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCommonUserPresenceBackendKeyTest::RunTest(const FString&)
{
	// UCommonUserBasicPresence is a UGameInstanceSubsystem (ClassWithin = GameInstance), so
	// it must be created inside a GameInstance — a transient-package Outer trips a
	// ClassWithin ensure that would fail the test. SessionStateToBackendKey itself only
	// reads member keys, so a bare (uninitialised) GameInstance Outer is sufficient.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UCommonUserBasicPresence* Presence = NewObject<UCommonUserBasicPresence>(GameInstance);
	if (!TestNotNull(TEXT("Basic presence must be constructible"), Presence))
	{
		return true;
	}

	// Configure a distinct backend key per session state, so the mapping is observable
	// independently of the (empty) defaults.
	Presence->PresenceStatusMainMenu = TEXT("MainMenuKey");
	Presence->PresenceStatusMatchmaking = TEXT("MatchmakingKey");
	Presence->PresenceStatusInGame = TEXT("InGameKey");

	// The solution maps each session state to its configured key; the gutted start/
	// returns an empty string for every state, so each assertion fails on start/ and
	// passes on the solution. Asserting the configured value (not just non-empty) also
	// verifies the states map to the CORRECT keys, not just to some non-empty string.
	TestEqual(TEXT("OutOfGame must map to the configured main-menu key, not an empty string"),
		Presence->SessionStateToBackendKey(ECommonSessionInformationState::OutOfGame), FString(TEXT("MainMenuKey")));
	TestEqual(TEXT("Matchmaking must map to the configured matchmaking key, not an empty string"),
		Presence->SessionStateToBackendKey(ECommonSessionInformationState::Matchmaking), FString(TEXT("MatchmakingKey")));
	TestEqual(TEXT("InGame must map to the configured in-game key, not an empty string"),
		Presence->SessionStateToBackendKey(ECommonSessionInformationState::InGame), FString(TEXT("InGameKey")));

	return true;
}
