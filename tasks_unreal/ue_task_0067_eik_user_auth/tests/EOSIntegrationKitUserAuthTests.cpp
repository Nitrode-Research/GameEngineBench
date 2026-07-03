#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogEIKUserAuthTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0067 (EOSIntegrationKit User Auth).
//
// EDITABLE FILE UNDER TEST: UserManagerEOS.cpp/.h. This is a "restore behavior" task:
// start/ short-circuits every login/logout path by INSERTING, at the top of each
// function, a failure trigger + early return, e.g. in Login():
//     TriggerOnLoginCompleteDelegates(LocalUserNum, false, EmptyId, "Login disabled in benchmark branch");
//     return false;
// so start/ never routes through the real EOS user-manager path — it just fires an
// immediate failure and returns false. The solution removes those short-circuits and
// runs the real EOS login routing (Login("noeas_+_<method>") dispatches to
// LoginViaConnectInterface and returns true).
//
// OBSERVABLE CONTRACT (via the public IOnlineIdentity interface — FUserManagerEOS IS
// the EIK subsystem's identity interface, so GetIdentityInterface()->Login() dispatches
// to the editable code):
//   • Login(...) returns true when it routes through the real path (start returns false).
//   • The login is NOT completed synchronously with the tell-tale "disabled in benchmark
//     branch" failure that the start stub fires.
// The earlier suite asserted constructor defaults and member-function-pointer existence
// (structural; the short-circuited start also passes them) and is dropped.
//
// SET-UP: drive the live EIK online subsystem (DefaultPlatformService=EIK), which owns
// a real FUserManagerEOS with an initialized EOS platform, so the real login path has
// valid EOS handles and does not crash. The broken game-mode/EOS boot is bypassed via
// the DefaultEngine.ini override; the EIK subsystem is created independently.
//
// NOT COVERED, and why: actual EOS authentication success (needs real credentials +
// network), and the async completion result — neither is available/deterministic
// headless. We assert the routing decision (real path vs short-circuit), which is the
// editable behavior and is synchronous.
// ─────────────────────────────────────────────────────────────────────────────

namespace EIKAuthTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");
	static const TCHAR* DisabledMarker = TEXT("disabled in benchmark branch");
}

// ── REQUIRED — PIE: Login routes through the real EOS path (not the short-circuit) ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIKLoginRoutesThroughRealPathTest,
	"EOSIntegrationKit.UserAuth.login_routes_through_real_user_manager_path",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEIKLoginRoutesThroughRealPathTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(EIKAuthTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	static bool bFiredDisabledFailure;
	static bool bLoginReturn;
	static bool bGotIdentity;
	bFiredDisabledFailure = false;
	bLoginReturn = false;
	bGotIdentity = false;

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		IOnlineSubsystem* OSS = IOnlineSubsystem::Get(FName(TEXT("EIK")));
		if (!OSS)
		{
			AddError(TEXT("EIK online subsystem is not available — cannot test the EOS user manager"));
			return true;
		}
		IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
		if (!Identity.IsValid())
		{
			AddError(TEXT("EIK identity interface (FUserManagerEOS) is not available"));
			return true;
		}
		bGotIdentity = true;

		// The identity interface must report this project's own auth-type identifier.
		TestEqual(TEXT("Identity must report this project's auth-type identifier"),
			Identity->GetAuthType(), FString(TEXT("eik")));

		// Capture a synchronous OnLoginComplete (the start stub fires it immediately).
		Identity->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateLambda(
			[](int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& Error)
			{
				if (Error.Contains(EIKAuthTests::DisabledMarker))
				{
					bFiredDisabledFailure = true;
				}
			}));

		// "noeas_+_deviceid": the real path logs/dispatches to LoginViaConnectInterface
		// and returns true; the start stub fires the "disabled" failure and returns false.
		FOnlineAccountCredentials Creds(TEXT("noeas_+_deviceid"), TEXT("benchmark-test-id"), TEXT("benchmark-test-token"));
		bLoginReturn = Identity->Login(0, Creds);

		UE_LOG(LogEIKUserAuthTests, Display, TEXT("[eik-diag] gotIdentity=%d loginReturn=%d firedDisabled=%d"),
			(int32)bGotIdentity, (int32)bLoginReturn, (int32)bFiredDisabledFailure);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (!bGotIdentity)
		{
			return true; // already errored
		}
		TestTrue(TEXT("Login must route through the real EOS user-manager path and return true"),
			bLoginReturn);
		TestFalse(TEXT("Login must NOT short-circuit with the benchmark-disabled failure"),
			bFiredDisabledFailure);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
