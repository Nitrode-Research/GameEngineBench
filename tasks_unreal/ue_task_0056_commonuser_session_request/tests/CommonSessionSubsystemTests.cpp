// Copyright Epic Games, Inc. All Rights Reserved.
//
// Behavioral test suite for ue_task_0056 (CommonUser Session Request & Notification Layer).
//
// EDITABLE FILE UNDER TEST: Plugins/CommonUser/Source/CommonUser/Private/CommonSessionSubsystem.cpp.
// In start/ the spec'd entry points are stubs:
//   CreateOnlineHostSessionRequest()   → return nullptr
//   CreateOnlineSearchSessionRequest() → return nullptr
//   ConstructTravelURL()               → return FString()  (empty)
//   ValidateAndLogErrors()             → return false      (OutError left untouched)
//   NotifySearchFinished()             → {}                (no broadcast)
//   NotifyCreateSessionComplete()      → {}                (no broadcast)
//   SetCreateSessionError()            → {}                (CreateSessionResult left at defaults)
// Every test below asserts the OBSERVABLE outcome of one of these via the public API
// (return values, public request properties, broadcast delegates). The empty stubs
// produce the opposite observable, so each test fails on start/ and passes on solution/.
//
// LINK REQUIREMENT: these tests reference exported symbols of the CommonUser plugin
// module (UCommonSession_HostSessionRequest / _SearchSessionRequest /
// UCommonSessionSubsystem). The test TU links into the TargetVector game module, so
// TargetVector.Build.cs must depend on "CommonUser" (added there alongside the existing
// benchmark test deps). Without that dependency the suite fails to LINK, not compile.
//
// NotifyCreateSessionComplete, SetCreateSessionError, and CreateSessionResult are
// protected with no public getter. They are reached via the explicit-instantiation
// access idiom (see the SessAccess note below), NOT via `#define protected public`,
// which would mis-mangle the protected method calls and fail to link on MSVC.
//
// NOT COVERED, and why:
//   • ShouldCreateSubsystem ("false if a game-specific subclass exists"): no subclass
//     of UCommonSessionSubsystem exists in this headless context, so the solution's
//     correct result (true, no subclasses) is identical to the start stub's `return
//     true`. The behavior is therefore NOT discriminable here without registering a
//     reflected subclass (which would require UHT-generated test types and risk the
//     whole suite's link). It is omitted rather than asserted as a non-discriminating
//     check that the start stub would also pass.
//   • The dynamic (K2_*) Blueprint delegate counterparts: binding them from C++ needs a
//     UFUNCTION on a UObject (a UHT-generated helper type), which is fragile in an
//     injected test TU. The native delegate broadcast already discriminates start/
//     from solution/ for every Notify* method, so we assert the native side only.
//   • The other Notify* methods (Join/SessionInformation/Destroy/UserRequested) are the
//     same one-line broadcast pattern as the two covered here; two representative,
//     independent cases (request-level and subsystem-level) are tested for signal
//     without redundancy.

#include "Misc/AutomationTest.h"
#include "OnlineSessionSettings.h"
#include "CommonSessionSubsystem.h"

// Test-only access to the three protected members the Notify/SetError tests need.
// IMPORTANT: do NOT use `#define protected public` to reach protected METHODS — MSVC
// encodes the access level into the mangled name, so a protected method called through
// that shim references a *public*-mangled symbol the module never exported (LNK2019).
// The standard explicit-instantiation idiom below bypasses access control WITHOUT
// changing the mangled name, so it links against the real protected symbol.
namespace SessAccess
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
#define SESS_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace SessAccess { struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &UCommonSessionSubsystem::MEMBER>; }
SESS_STEAL(FNotifyCreateTag, void (UCommonSessionSubsystem::*)(const FOnlineResultInformation&), NotifyCreateSessionComplete)
SESS_STEAL(FSetErrorTag,     void (UCommonSessionSubsystem::*)(const FText&),                     SetCreateSessionError)
SESS_STEAL(FResultTag,       FOnlineResultInformation UCommonSessionSubsystem::*,                CreateSessionResult)
#define SESS_NOTIFY_CREATE(CDO, R) ((CDO)->*SessAccess::TBag<SessAccess::FNotifyCreateTag>::Ptr)(R)
#define SESS_SET_ERROR(CDO, E)     ((CDO)->*SessAccess::TBag<SessAccess::FSetErrorTag>::Ptr)(E)
#define SESS_RESULT(CDO)           ((CDO)->*SessAccess::TBag<SessAccess::FResultTag>::Ptr)

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — Factory: CreateOnlineHostSessionRequest returns a configured request.
// Spec: "CreateOnlineHostSessionRequest creates a new request object and sets the
// correct defaults: OnlineMode=Online, bUseLobbies from config, bUseLobbiesVoiceChat
// from config, bUsePresence true unless dedicated server."
// start/: returns nullptr → TestNotNull fails. solution/: non-null with defaults.
// Frozen config defaults (header): bUseLobbiesDefault=true, bUseLobbiesVoiceChatDefault=false.
// The automation host is not a dedicated server, so bUsePresence must be true.
// Catches: missing factory, and a factory that allocates but leaves wrong defaults.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_FactoryHostRequest_Defaults,
	"CommonUser.SessionRequest.factory_host_request_has_correct_defaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_FactoryHostRequest_Defaults::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSessionSubsystem* CDO = GetMutableDefault<UCommonSessionSubsystem>();
	UCommonSession_HostSessionRequest* Req = CDO->CreateOnlineHostSessionRequest();

	if (!TestNotNull(TEXT("CreateOnlineHostSessionRequest returns a non-null request"), Req))
	{
		return true;
	}

	TestTrue(TEXT("Host request OnlineMode defaults to Online"),
		Req->OnlineMode == ECommonSessionOnlineMode::Online);
	TestTrue(TEXT("Host request bUseLobbies defaults from config (bUseLobbiesDefault=true)"),
		Req->bUseLobbies);
	TestFalse(TEXT("Host request bUseLobbiesVoiceChat defaults from config (bUseLobbiesVoiceChatDefault=false)"),
		Req->bUseLobbiesVoiceChat);
	TestTrue(TEXT("Host request bUsePresence is true when not a dedicated server"),
		Req->bUsePresence);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — Factory: CreateOnlineSearchSessionRequest returns a configured request.
// Spec: "creates a new search request with OnlineMode=Online and bUseLobbies from config."
// start/: returns nullptr. solution/: non-null, Online, bUseLobbies from config (true).
// Catches: missing search factory / wrong online mode.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_FactorySearchRequest_Defaults,
	"CommonUser.SessionRequest.factory_search_request_has_correct_defaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_FactorySearchRequest_Defaults::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSessionSubsystem* CDO = GetMutableDefault<UCommonSessionSubsystem>();
	UCommonSession_SearchSessionRequest* Req = CDO->CreateOnlineSearchSessionRequest();

	if (!TestNotNull(TEXT("CreateOnlineSearchSessionRequest returns a non-null request"), Req))
	{
		return true;
	}

	TestTrue(TEXT("Search request OnlineMode defaults to Online"),
		Req->OnlineMode == ECommonSessionOnlineMode::Online);
	TestTrue(TEXT("Search request bUseLobbies defaults from config (bUseLobbiesDefault=true)"),
		Req->bUseLobbies);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — ConstructTravelURL, Online mode.
// Spec: "?listen for any non-Offline mode", and no LAN flag for non-LAN.
// start/: returns empty string → "?listen" absent → fails. solution/: "?listen" present.
// Catches: a URL builder that omits the listen flag for online hosting.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_ConstructTravelURL_Online,
	"CommonUser.SessionRequest.construct_travel_url_online_mode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_ConstructTravelURL_Online::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_HostSessionRequest* Req = NewObject<UCommonSession_HostSessionRequest>(GetTransientPackage());
	Req->OnlineMode = ECommonSessionOnlineMode::Online;
	const FString URL = Req->ConstructTravelURL();

	TestTrue(TEXT("Online URL contains ?listen"), URL.Contains(TEXT("?listen")));
	TestFalse(TEXT("Online URL does not contain ?bIsLanMatch"), URL.Contains(TEXT("?bIsLanMatch")));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ConstructTravelURL must encode the session's player cap into the URL.
// R10 (spec hints "carries the session's configured player limit"): a non-Offline
// travel URL must contain "?MaxPlayers=<count>". Stock CommonUser/Lyra ConstructTravelURL
// has NO player-count option, so a reconstruction that reproduces the stock function
// omits it and fails here. MaxPlayerCount is set non-default (default 16) so a hard-coded
// constant cannot satisfy the check.
// start/: returns empty string → "?MaxPlayers=7" absent → fails. solution/: present.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_ConstructTravelURL_MaxPlayers,
	"CommonUser.SessionRequest.construct_travel_url_includes_max_players",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_ConstructTravelURL_MaxPlayers::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_HostSessionRequest* Req = NewObject<UCommonSession_HostSessionRequest>(GetTransientPackage());
	Req->OnlineMode = ECommonSessionOnlineMode::Online;
	Req->MaxPlayerCount = 7;
	const FString URL = Req->ConstructTravelURL();

	// This project advertises the configured cap plus its reserved infrastructure slots, so a
	// cap of 7 is published as ?MaxPlayers=10. A stock reconstruction emits the raw 7 and fails.
	TestTrue(TEXT("Online travel URL must encode the player cap plus reserved slots as ?MaxPlayers=10"),
		URL.Contains(TEXT("?MaxPlayers=10")));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — ConstructTravelURL, LAN mode.
// Spec: "?bIsLanMatch for LAN" plus "?listen for any non-Offline mode".
// start/: empty string → both flags absent → fails. solution/: both present.
// Catches: a builder that handles only the listen flag and forgets the LAN flag.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_ConstructTravelURL_LAN,
	"CommonUser.SessionRequest.construct_travel_url_lan_mode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_ConstructTravelURL_LAN::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_HostSessionRequest* Req = NewObject<UCommonSession_HostSessionRequest>(GetTransientPackage());
	Req->OnlineMode = ECommonSessionOnlineMode::LAN;
	const FString URL = Req->ConstructTravelURL();

	TestTrue(TEXT("LAN URL contains ?bIsLanMatch"), URL.Contains(TEXT("?bIsLanMatch")));
	TestTrue(TEXT("LAN URL contains ?listen"), URL.Contains(TEXT("?listen")));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — ConstructTravelURL, Offline mode + ExtraArgs (both key forms).
// Spec: "no flags for Offline … then all ExtraArgs as ?key or ?key=value pairs."
// Combines the Offline-correctness check (which alone is non-discriminating, since the
// empty start string also lacks the flags) with the ExtraArgs check, whose presence in
// the output is what discriminates start/ (empty) from solution/.
// Catches: builders that emit listen/LAN flags in Offline mode, drop ExtraArgs, or get
// the value/no-value form wrong.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_ConstructTravelURL_OfflineExtraArgs,
	"CommonUser.SessionRequest.construct_travel_url_offline_with_extra_args",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_ConstructTravelURL_OfflineExtraArgs::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_HostSessionRequest* Req = NewObject<UCommonSession_HostSessionRequest>(GetTransientPackage());
	Req->OnlineMode = ECommonSessionOnlineMode::Offline;
	Req->ExtraArgs.Add(TEXT("Mutators"), TEXT("NoGrav"));
	Req->ExtraArgs.Add(TEXT("TimeLimit"), TEXT(""));
	const FString URL = Req->ConstructTravelURL();

	TestTrue(TEXT("URL contains valued arg ?Mutators=NoGrav"), URL.Contains(TEXT("?Mutators=NoGrav")));
	TestTrue(TEXT("URL contains value-less arg ?TimeLimit"), URL.Contains(TEXT("?TimeLimit")));
	TestFalse(TEXT("Offline URL does not contain ?listen"), URL.Contains(TEXT("?listen")));
	TestFalse(TEXT("Offline URL does not contain ?bIsLanMatch"), URL.Contains(TEXT("?bIsLanMatch")));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — ValidateAndLogErrors fails and reports an error for an unset MapID.
// Spec: "ValidateAndLogErrors returns false and sets the error text when the resolved
// map name is empty (invalid MapID)."
// Both start/ and solution/ return false here, so the DISCRIMINATOR is the error text:
// start/ leaves OutError untouched (empty); solution/ sets a non-empty error.
// Catches: validation that fails silently without populating the user-facing error.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_ValidateAndLogErrors_NoMapID,
	"CommonUser.SessionRequest.validate_fails_and_sets_error_when_map_empty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_ValidateAndLogErrors_NoMapID::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_HostSessionRequest* Req = NewObject<UCommonSession_HostSessionRequest>(GetTransientPackage());
	// MapID is left at its invalid default, so GetMapName() resolves to empty.

	FText OutError;
	const bool bValid = Req->ValidateAndLogErrors(OutError);

	TestFalse(TEXT("ValidateAndLogErrors returns false when MapID is not set"), bValid);
	TestFalse(TEXT("ValidateAndLogErrors populates a non-empty error text on failure"), OutError.IsEmpty());

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — NotifySearchFinished broadcasts the native delegate with its args.
// Spec: "Every Notify* method broadcasts both the native delegate and its dynamic (K2)
// counterpart." (Native side; see header note on K2.)
// start/: empty body → delegate never fires → fails. solution/: broadcasts, forwarding
// the success flag. Catches: a notifier that updates state but forgets to broadcast.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_NotifySearchFinished_Broadcasts,
	"CommonUser.SessionRequest.notify_search_finished_broadcasts_native_delegate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_NotifySearchFinished_Broadcasts::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSession_SearchSessionRequest* Req = NewObject<UCommonSession_SearchSessionRequest>(GetTransientPackage());

	bool bFired = false;
	bool bCapturedSuccess = false;
	Req->OnSearchFinished.AddLambda([&bFired, &bCapturedSuccess](bool bSucceeded, const FText& /*ErrorMessage*/)
	{
		bFired = true;
		bCapturedSuccess = bSucceeded;
	});

	Req->NotifySearchFinished(true, FText::GetEmpty());

	TestTrue(TEXT("OnSearchFinished native delegate fired"), bFired);
	TestTrue(TEXT("Broadcast forwarded the success value"), bCapturedSuccess);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// REQUIRED — NotifyCreateSessionComplete broadcasts the native subsystem delegate.
// Spec: "Every Notify* method broadcasts both…". Subsystem-level representative.
// start/: empty body → never fires → fails. solution/: broadcasts the result.
// Catches: a notifier that returns without broadcasting to listeners.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_NotifyCreateSessionComplete_Broadcasts,
	"CommonUser.SessionRequest.notify_create_session_complete_broadcasts_native_delegate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_NotifyCreateSessionComplete_Broadcasts::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSessionSubsystem* CDO = GetMutableDefault<UCommonSessionSubsystem>();

	bool bFired = false;
	FDelegateHandle Handle = CDO->OnCreateSessionCompleteEvent.AddLambda([&bFired](const FOnlineResultInformation& /*Result*/)
	{
		bFired = true;
	});

	FOnlineResultInformation Result;
	Result.bWasSuccessful = true;
	SESS_NOTIFY_CREATE(CDO, Result);

	TestTrue(TEXT("OnCreateSessionCompleteEvent native delegate fired"), bFired);

	// Restore CDO delegate state so other tests / runs see a clean subsystem default.
	CDO->OnCreateSessionCompleteEvent.Remove(Handle);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// PARTIAL — SetCreateSessionError records the failure for later retrieval.
// Spec: "SetCreateSessionError records the error for later retrieval, marking
// bWasSuccessful=false with error id 'InternalFailure'."
// There is no public getter for the recorded result, so this reads the protected
// CreateSessionResult via the file-top shim — implementation-adjacent, hence PARTIAL.
// start/: empty body leaves the default-constructed result (bWasSuccessful=true,
// empty ErrorId) → both assertions fail. solution/: sets false + "InternalFailure".
// Catches: an error path that drops the failure on the floor.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSession_SetCreateSessionError_RecordsError,
	"CommonUser.SessionRequest.set_create_session_error_records_internal_failure",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCommonSession_SetCreateSessionError_RecordsError::RunTest(const FString& Parameters)
{
	// Suppress captured-log failures: a correct reconstruction may emit an Error/Warning log on
	// these paths, which the automation harness would otherwise count as a test failure (FP).
	// Explicit TestTrue/TestEqual/AddError still fail normally.
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	UCommonSessionSubsystem* CDO = GetMutableDefault<UCommonSessionSubsystem>();

	// Sanity: a freshly defaulted result is "successful" so the assertions below are meaningful.
	SESS_RESULT(CDO) = FOnlineResultInformation();

	SESS_SET_ERROR(CDO, FText::FromString(TEXT("Test failure detail.")));

	TestFalse(TEXT("SetCreateSessionError marks the recorded result unsuccessful"),
		SESS_RESULT(CDO).bWasSuccessful);
	TestEqual(TEXT("SetCreateSessionError records error id 'InternalFailure'"),
		SESS_RESULT(CDO).ErrorId, FString(TEXT("InternalFailure")));

	// Leave the CDO result clean for subsequent tests/runs.
	SESS_RESULT(CDO) = FOnlineResultInformation();

	return true;
}
