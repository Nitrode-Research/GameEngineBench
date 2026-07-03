#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/ScriptInterface.h"
#include "LoadingScreenManager.h"
#include "LoadingProcessTask.h"
#include "LoadingProcessInterface.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLoadingScreenTests, Log, All);

// Test-only access to the private CheckForAnyNeedToShowLoadingScreen(). Explicit-
// instantiation idiom (NOT `#define private public`, which would mis-mangle the
// private method on MSVC and fail to link — see project-test-authoring-infra).
namespace LoadAccess
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
#define LOAD_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace LoadAccess { struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &ULoadingScreenManager::MEMBER>; }
LOAD_STEAL(FCheckNeedTag, bool (ULoadingScreenManager::*)(), CheckForAnyNeedToShowLoadingScreen)

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0062 (CommonLoadingScreen Manager).
//
// EDITABLE FILES UNDER TEST: LoadingScreenManager.cpp/.h (+ LoadingProcessTask, the
// interface). In start/ the decision core is a stub:
//   CheckForAnyNeedToShowLoadingScreen() → sets a "stripped in benchmark" reason and
//   returns false unconditionally — so the manager NEVER decides to show the screen,
//   no matter what demands it. The solution evaluates the real conditions: map-load
//   state, pending travel, and every registered ILoadingProcessInterface processor.
//
// OBSERVABLE CONTRACT: a registered processor that demands the loading screen makes
// CheckForAnyNeedToShowLoadingScreen() return true AND records that processor's reason
// in the (publicly readable) debug reason. Asserting the reason isolates the processor
// path from the function's other early-outs (e.g. "no local PC"), so the test verifies
// the registered-demand behavior specifically, not just a true/false flip.
//
// SPAWN/SETUP: the manager is a UGameInstanceSubsystem, so the test uses the real
// subsystem from the PIE game instance (it dereferences GetGameInstance() unguarded —
// a NewObject manager would crash). ULoadingProcessTask is the engine's concrete
// ILoadingProcessInterface implementation (its ShouldShowLoadingScreen returns true
// with its Reason, identical in start/ and solution/), so it is a reliable demander.
// The broken game-mode/EOS boot is bypassed via the DefaultEngine.ini override.
//
// The earlier suite asserted CDO defaults and member-function-pointer existence —
// structural checks the empty start stub also passes — and is dropped.
//
// NOT COVERED, and why: the actual loading-screen WIDGET show/hide and input-blocking
// go through UI/viewport paths that are not reliably exercisable in headless
// automation; the decision core (what the spec calls "the existing processors or
// map-loading conditions require it") is the deterministic, content-light discriminator.
// ─────────────────────────────────────────────────────────────────────────────

namespace LoadingScreenTests
{
	static const TCHAR* TestMap = TEXT("/Game/Tests/TestLevels/VoidWorld");
	static const TCHAR* ProcessorReason = TEXT("UnrealBenchTestProcessor");

	static UWorld* GetTestWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if ((Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game) && Ctx.World())
				return Ctx.World();
		}
		return nullptr;
	}

	static ULoadingScreenManager* GetManager(UWorld* World)
	{
		if (!World) return nullptr;
		UGameInstance* GI = World->GetGameInstance();
		return GI ? GI->GetSubsystem<ULoadingScreenManager>() : nullptr;
	}
}

// ── REQUIRED — PIE: a registered processor drives the manager to show the screen ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingScreenProcessorDemandTest,
	"CommonLoadingScreen.Manager.registered_processor_drives_show_decision",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLoadingScreenProcessorDemandTest::RunTest(const FString&)
{
#if WITH_EDITOR
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(LoadingScreenTests::TestMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = LoadingScreenTests::GetTestWorld();
		ULoadingScreenManager* Manager = LoadingScreenTests::GetManager(World);
		if (!Manager)
		{
			AddError(TEXT("Could not obtain the ULoadingScreenManager subsystem from the PIE game instance"));
			return true;
		}

		// A processor that demands the loading screen with a distinctive reason.
		ULoadingProcessTask* Task = NewObject<ULoadingProcessTask>(Manager);
		Task->SetShowLoadingScreenReason(LoadingScreenTests::ProcessorReason);
		Manager->RegisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface>(Task));

		const bool bNeed = (Manager->*LoadAccess::TBag<LoadAccess::FCheckNeedTag>::Ptr)();
		const FString Reason = Manager->GetDebugReasonForShowingOrHidingLoadingScreen();

		UE_LOG(LogLoadingScreenTests, Display, TEXT("[loadscreen-diag] need=%d reason='%s'"), (int32)bNeed, *Reason);

		TestTrue(TEXT("A registered demanding processor must make the manager decide to show the loading screen"),
			bNeed);
		// The reason for an external-processor demand must be tagged with its origin: the
		// manager prefixes the processor's reason with "Processor: ". A reconstruction that
		// forwards the raw processor reason (stock behavior) produces the untagged string and
		// fails this exact-match check.
		TestEqual(TEXT("The show decision must be attributed to the registered processor's reason, tagged with its origin"),
			Reason, FString(TEXT("Processor: ")) + FString(LoadingScreenTests::ProcessorReason));

		Manager->UnregisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface>(Task));
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
#endif
	return true;
}
