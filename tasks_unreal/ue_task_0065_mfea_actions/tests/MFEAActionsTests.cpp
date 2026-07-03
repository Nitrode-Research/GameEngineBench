#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"        // GEngine
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/DefaultPawn.h"
#include "Components/GameFrameworkComponentManager.h"

// Read/write the actions' private state (ActiveRequests, TargetPawnClass) for this TU. The
// discriminating call (AddToWorld) is VIRTUAL, so it dispatches through the vtable — no
// direct private-symbol reference, so the macro's access change does not break linkage.
#define protected public
#define private public
#include "Actions/GameFeatureAction_AddAbilities.h"
#include "Actions/GameFeatureAction_WorldActionBase.h"
#undef private
#undef protected

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test for ue_task_0065 (Modular Features Extra Actions).
//
// EDITABLE FILE UNDER TEST (observed): GameFeatureAction_AddAbilities.cpp (+ base). The
// discriminating behavior is AddToWorld: the solution registers an actor-extension handler
// with the GameFrameworkComponentManager and stores the request handle in ActiveRequests
// (spec: "Activating a game feature hooks ... actor-extension events through the existing
// pattern" / "follows the GameFrameworkComponentManager lifecycle"). The gutted start/ is
// an empty {} and registers nothing.
//
// The action's other gutted behaviors (granting/removing abilities, input bindings) need a
// live actor with an AbilitySystemComponent and are not observable headlessly. AddToWorld is
// reachable because it only needs a game world + a game instance carrying the component
// manager subsystem, which we construct directly. The previous suite's CDO-default and
// method-pointer checks pass on start/ too and were dropped.
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMFEAAddToWorldTest,
	"MFEA.Actions.add_to_world_registers_actor_extension_handler",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMFEAAddToWorldTest::RunTest(const FString&)
{
	// Game instance with its subsystems initialised (incl. the GameFrameworkComponentManager,
	// which the action looks up via UGameInstance::GetSubsystem<>()). InitializeStandalone creates
	// a backing game world AND wires the instance's WorldContext (so GameInstance->GetWorld() is
	// non-null) — AddExtensionHandler asserts ensure(LocalGameInstance->GetWorld()), which a bare
	// NewObject+Init() instance fails because its WorldContext is never set.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UGameFrameworkComponentManager* ComponentManager =
		UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (!TestNotNull(TEXT("GameFrameworkComponentManager subsystem must exist on the game instance"), ComponentManager))
	{
		return true;
	}

	// The standalone world context — the action reads WorldContext.World()->IsGameWorld() and
	// resolves the component manager from WorldContext.OwningGameInstance (both set above).
	FWorldContext& WorldContext = *GameInstance->GetWorldContext();
	UWorld* World = WorldContext.World();
	if (!TestNotNull(TEXT("standalone game world must exist"), World))
	{
		return true;
	}
	World->InitializeActorsForPlay(FURL());

	// Build a fresh action with the given configured pawn classes (first = TargetPawnClass, rest =
	// ExtraTargetPawnClasses), run AddToWorld, and return how many extension handlers it registered.
	// Call through the BASE pointer so the virtual dispatches via the vtable — calling on the 'final'
	// derived type devirtualizes into a direct reference to the un-exported symbol and fails to link.
	auto CountHooks = [&](const TArray<UClass*>& Classes) -> int32
	{
		UGameFeatureAction_AddAbilities* Act = NewObject<UGameFeatureAction_AddAbilities>(GetTransientPackage());
		for (int32 i = 0; i < Classes.Num(); ++i)
		{
			if (i == 0) { Act->TargetPawnClass = Classes[i]; }
			else        { Act->ExtraTargetPawnClasses.Add(Classes[i]); }
		}
		UGameFeatureAction_WorldActionBase* Base = Act;
		Base->AddToWorld(WorldContext);
		return Act->ActiveRequests.Num();
	};

	UClass* const Pawn = APawn::StaticClass();
	UClass* const Character = ACharacter::StaticClass();   // subclass of APawn
	UClass* const Default = ADefaultPawn::StaticClass();   // subclass of APawn, sibling of ACharacter

	// (1) {Character, Pawn, Character}: the duplicate Character is de-duplicated, leaving two
	//     DISTINCT configured classes — Character and Pawn — each hooked in its own right. A reader
	//     that "optimizes" by dropping Character because the configured Pawn would also cover it
	//     registers only 1 and fails. gutted start/ -> 0; correct -> 2.
	TestEqual(TEXT("(1) each distinct configured class is hooked (duplicate de-duplicated): 2"),
		CountHooks({ Character, Pawn, Character }), 2);

	// (2) {Character, Default}: two distinct sibling classes, both hooked.
	TestEqual(TEXT("(2) two distinct sibling classes are both hooked: 2"),
		CountHooks({ Character, Default }), 2);

	// (3) {Character, Default, Pawn}: three distinct configured classes, each hooked independently.
	//     A reader that subsumes the two subclasses under the configured base Pawn registers only 1
	//     and fails — configured classes are hooked on equal footing, not collapsed to a base.
	TestEqual(TEXT("(3) all three distinct configured classes are hooked independently: 3"),
		CountHooks({ Character, Default, Pawn }), 3);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
