#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineSessionInterface.h"

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test for ue_task_0066 (EOSIntegrationKit Session/Lobby flow).
//
// EDITABLE FILES UNDER TEST: OnlineSessionEOS.{cpp,h} (the "EIK" online subsystem's session
// interface, FOnlineSessionEOS). The gutted start/ injects an early short-circuit at the top of
// ~40 session/lobby methods (e.g. "TriggerOnCreateSessionCompleteDelegates(SessionName, false);
// return false;") so each reports failure and skips its real body; solution/ runs the real code.
//
// We reach the implementation through the EXPORTED IOnlineSubsystem / IOnlineSession API rather
// than constructing FOnlineSessionEOS directly: that class lives in the plugin's Private/ folder
// and is NOT marked ONLINESUBSYSTEMEIK_API, so a cross-module construction would emit a vtable
// referencing non-exported virtuals and fail to link. Going through the live subsystem also gives
// a real (non-null) EOSSubsystem, so the methods' async tails (EOSSubsystem->ExecuteNextTick(...))
// don't crash.
//
// Discriminator: SendSessionInviteToFriends over an EMPTY friend list. Use the FUniqueNetId
// overload directly: the LocalUserNum overload needs a logged-in user in headless automation.
// start/ is a bare "return false;"; solution/ iterates the empty list and returns true without
// touching EOS handles. So start -> false, solution -> true.
//
// NOTE: this only runs if the EIK subsystem and its session interface come up in the headless
// editor. If they don't, the test errors (rather than passing vacuously) so a non-discriminating
// pass can never be mistaken for success.
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIKSendInviteToEmptyFriendListTest,
	"EOSIntegrationKit.SessionLobby.send_invite_to_empty_friend_list_succeeds_through_real_loop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEIKSendInviteToEmptyFriendListTest::RunTest(const FString&)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(FName(TEXT("EIK")));
	if (!TestNotNull(TEXT("EIK online subsystem must be available"), Subsystem))
	{
		return true;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!TestTrue(TEXT("EIK session interface must be valid"), SessionInterface.IsValid()))
	{
		return true;
	}

	// Zero friends -> the solution's loop runs no iterations and returns true; it never reaches
	// SendSessionInviteToFriend (which would need a logged-in user). The gutted start/ stub returns
	// false unconditionally.
	const FUniqueNetIdStringRef LocalUserId =
		FUniqueNetIdString::Create(TEXT("BenchmarkLocalUser"), TEXT("EIK"));
	const TArray<FUniqueNetIdRef> NoFriends;
	const bool bResult =
		SessionInterface->SendSessionInviteToFriends(*LocalUserId, FName(TEXT("BenchmarkSession")), NoFriends);

	TestTrue(
		TEXT("SendSessionInviteToFriends over an empty friend list must complete the real loop and "
			 "return true; the gutted start/ stub returns false"),
		bResult);

	return true;
}
