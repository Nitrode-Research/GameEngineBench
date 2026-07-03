// Copyright (c) 2026 GameDevBench. Generated Map Orchestrator automation tests (Bomber).
//
// Tests target ABmrGeneratedMap orchestration: round-start population, the
// completion-signal path, off-thread cost, actor reuse, and replicated-property
// registration. PIE tests bring up the project's `Main` map in a single
// standalone PIE session — HasAuthority is true, exercising the server-side
// orchestration code paths exactly as they run on a listen-server host.
//
// Notes on observability:
//   * `OnGeneratedLevelActors` is a DECLARE_DYNAMIC_MULTICAST_DELEGATE, so it
//     can only be bound via UFUNCTION reflection. Tests instead observe the
//     replicated `GenerateLevelActorsToken` UPROPERTY (whose advancement is
//     the on-the-wire completion signal) together with `MapComponents.Num()`,
//     the replicated container of spawned level actors. Both are read through
//     the same FProperty API the editor and serializers use.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

// Bomber
#include "Actors/BmrGeneratedMap.h"
#include "Components/BmrMapComponent.h"
#include "Structures/BmrMapComponentsContainer.h"
#include "Subsystems/BmrGeneratedMapSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGeneratedMapOrchestratorTests, Log, All);

// ---------------------------------------------------------------------------
// Access to the `MapComponents` UPROPERTY via UE reflection (FStructProperty).
// `MapComponents` is declared `protected` on ABmrGeneratedMap, so the production
// header stays untouched and we read the value through the same FProperty API
// the editor and serializers use.
// ---------------------------------------------------------------------------
namespace GeneratedMapPrivateAccess
{
	static const FBmrMapComponentsContainer& GetMapComponents(const ABmrGeneratedMap* Map)
	{
		static const FBmrMapComponentsContainer EmptyContainer;
		if (!Map)
		{
			return EmptyContainer;
		}
		const FStructProperty* Prop = CastField<FStructProperty>(
			ABmrGeneratedMap::StaticClass()->FindPropertyByName(TEXT("MapComponents")));
		if (!Prop)
		{
			return EmptyContainer;
		}
		const FBmrMapComponentsContainer* Value =
			Prop->ContainerPtrToValuePtr<FBmrMapComponentsContainer>(Map);
		return Value ? *Value : EmptyContainer;
	}

	// Mutable accessor used by branch tests that need to set the container's
	// LocalReplicationToken directly to simulate a known on-wire state before
	// invoking OnRep_GenerateLevelActorsToken().
	static FBmrMapComponentsContainer* GetMutableMapComponents(ABmrGeneratedMap* Map)
	{
		if (!Map)
		{
			return nullptr;
		}
		const FStructProperty* Prop = CastField<FStructProperty>(
			ABmrGeneratedMap::StaticClass()->FindPropertyByName(TEXT("MapComponents")));
		if (!Prop)
		{
			return nullptr;
		}
		return Prop->ContainerPtrToValuePtr<FBmrMapComponentsContainer>(Map);
	}
}

namespace GeneratedMapOrchestratorTests
{
	static const TCHAR* MainMapPath = TEXT("/Game/Bomber/Maps/Main");

	// The Bomber data-asset bootstrap is async; the initial generation does
	// not fire until DA registry rows finish loading. Give it generous walls.
	static constexpr float DataAssetReadyTimeoutSeconds = 30.f;
	static constexpr float GenerationTimeoutSeconds = 20.f;

	// State shared across latent commands within a single test invocation.
	struct FRunState
	{
		TWeakObjectPtr<ABmrGeneratedMap> GeneratedMap;
		int32 BaselineLevelActors = 0;
		// Snapshot of MapComponents -> UBmrMapComponent set at the end of the
		// initial generation; used to detect pool reuse on regenerate.
		TSet<TWeakObjectPtr<const UBmrMapComponent>> BaselineComponents;
		double GenerateLevelActorsMainThreadSeconds = 0.0;
	};

	static FRunState GRunState;

	static void ResetRunState()
	{
		GRunState = FRunState{};
	}

	static UWorld* GetServerPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		UWorld* Fallback = nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType != EWorldType::PIE || !Ctx.World())
			{
				continue;
			}
			const ENetMode NetMode = Ctx.World()->GetNetMode();
			if (NetMode == NM_Standalone || NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
			{
				return Ctx.World();
			}
			Fallback = Ctx.World();
		}
		return Fallback;
	}

	static ABmrGeneratedMap* FindGeneratedMap(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		if (UBmrGeneratedMapSubsystem* Subsystem = World->GetSubsystem<UBmrGeneratedMapSubsystem>())
		{
			if (ABmrGeneratedMap* Tracked = Subsystem->GetGeneratedMap(false))
			{
				return Tracked;
			}
		}
		for (TActorIterator<ABmrGeneratedMap> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	static int32 CountLevelActors(UWorld* World)
	{
		if (!World)
		{
			return 0;
		}
		const ULevel* Level = World->GetCurrentLevel();
		return Level ? Level->Actors.Num() : 0;
	}

	static TSet<TWeakObjectPtr<const UBmrMapComponent>> SnapshotMapComponentSet(ABmrGeneratedMap* Map)
	{
		TSet<TWeakObjectPtr<const UBmrMapComponent>> Out;
		if (!Map)
		{
			return Out;
		}
		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);
		for (int32 Idx = 0; Idx < Container.Num(); ++Idx)
		{
			if (const UBmrMapComponent* Comp = Container[Idx])
			{
				Out.Emplace(Comp);
			}
		}
		return Out;
	}

	// Write an int32 UPROPERTY by name. Returns true on success. Mutating the
	// replicated `GenerateLevelActorsToken` directly lets the branch tests
	// stage the exact wire-state that triggers each OnRep code path.
	static bool WriteIntProperty(ABmrGeneratedMap* Map, const TCHAR* PropertyName, int32 Value)
	{
		if (!Map)
		{
			return false;
		}
		const FProperty* Prop = ABmrGeneratedMap::StaticClass()->FindPropertyByName(FName(PropertyName));
		const FIntProperty* IntProp = CastField<FIntProperty>(Prop);
		if (!IntProp)
		{
			return false;
		}
		IntProp->SetPropertyValue_InContainer(Map, Value);
		return true;
	}

	// Invoke a UFUNCTION on the Generated Map by name with no parameters.
	// Returns true if the function was found and invoked.
	static bool CallNoArgUFunction(ABmrGeneratedMap* Map, const TCHAR* FunctionName)
	{
		if (!Map)
		{
			return false;
		}
		UFunction* Func = Map->FindFunction(FName(FunctionName));
		if (!Func)
		{
			return false;
		}
		Map->ProcessEvent(Func, nullptr);
		return true;
	}

	// Read an int32 UPROPERTY by name. Returns INDEX_NONE on failure.
	static int32 ReadIntProperty(const ABmrGeneratedMap* Map, const TCHAR* PropertyName)
	{
		if (!Map)
		{
			return INDEX_NONE;
		}
		const FProperty* Prop = ABmrGeneratedMap::StaticClass()->FindPropertyByName(FName(PropertyName));
		const FIntProperty* IntProp = CastField<FIntProperty>(Prop);
		if (!IntProp)
		{
			return INDEX_NONE;
		}
		return IntProp->GetPropertyValue_InContainer(Map);
	}

	// True once a generation has completed: MapComponents is populated and the
	// replicated token is strictly positive. The solution's GenerateLevelActors()
	// synchronously calls DestroyAllLevelActors() *before* it dispatches the
	// async cell-graph work, and DestroyAllLevelActors() zeroes both Items and
	// the replicated token. Because the regenerate call site is its own latent
	// command, by the time the wait-loop latent command starts polling the
	// token has already been clamped to 0. The token only climbs back above
	// zero once the OnSpawned continuation fires on the game thread at the
	// end of the new generation — so observing `Token > 0` together with a
	// non-empty container reliably indicates the new run completed.
	//
	// Note: we deliberately do NOT compare against a pre-call snapshot, because
	// after a same-size regenerate the token returns to the same value it had
	// before (same actor count → same LocalReplicationToken increment count).
	static bool IsGenerationComplete(ABmrGeneratedMap* Map)
	{
		if (!Map)
		{
			return false;
		}
		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);
		if (Container.Num() == 0)
		{
			return false;
		}
		const int32 Token = ReadIntProperty(Map, TEXT("GenerateLevelActorsToken"));
		return Token > 0;
	}
}

// ---------------------------------------------------------------------------
// Test 1 — Replication registration (no PIE, header metadata + source inspection)
//
// B9 DIRECT: GetLifetimeReplicatedProps must register MapComponents,
//            GenerateLevelActorsToken, and AbilitySystemComponent.
//
// Two parts:
//   (a) FProperty/CPF_Net check: each property must carry the UPROPERTY
//       Replicated/ReplicatedUsing specifier. CPF_Net is set at compile time
//       from the header, so this catches a missing specifier but NOT a stub
//       GetLifetimeReplicatedProps body — the header specifiers are intact
//       in the stripped state.
//   (b) Source-file inspection: load BmrGeneratedMap.cpp and verify the body
//       of GetLifetimeReplicatedProps actually registers each of the three
//       properties via a DOREPLIFETIME-prefixed macro. This is what was
//       stripped — the start/ body is just `Super::GetLifetimeReplicatedProps`.
//       Calling GetLifetimeReplicatedProps on a live instance crashes outside
//       a fully-initialized net-driver context (see ue_task_0031 Test 6
//       header comment), so source inspection is the established pattern in
//       this codebase for this kind of registration check.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGeneratedMapOrchestrator_ReplicationProps,
	"Bomber.GeneratedMapOrchestrator.ReplicationProps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeneratedMapOrchestrator_ReplicationProps::RunTest(const FString& Parameters)
{
	const UClass* Class = ABmrGeneratedMap::StaticClass();
	if (!TestNotNull(TEXT("ABmrGeneratedMap class must exist"), Class))
	{
		return false;
	}

	const ABmrGeneratedMap* CDO = Cast<const ABmrGeneratedMap>(Class->GetDefaultObject());
	if (!TestNotNull(TEXT("ABmrGeneratedMap CDO must be obtainable"), CDO))
	{
		return false;
	}

	// -----------------------------------------------------------------------
	// (a) Header metadata: each property must have CPF_Net set.
	// This only proves the UPROPERTY(Replicated) specifier exists; the
	// stripped state still passes this part because only the .cpp body was
	// gutted. Source inspection below is what gates on the registration.
	// -----------------------------------------------------------------------
	auto IsReplicatedByName = [Class](const TCHAR* PropertyName) -> bool
	{
		const FProperty* Prop = Class->FindPropertyByName(FName(PropertyName));
		return Prop != nullptr && (Prop->PropertyFlags & CPF_Net) != 0;
	};

	TestTrue(
		TEXT("B9: MapComponents must be declared as a replicated UPROPERTY on ABmrGeneratedMap. "
			 "Without this, the FastArray of spawned level actors never replicates and clients see an empty grid."),
		IsReplicatedByName(TEXT("MapComponents")));

	TestTrue(
		TEXT("B9: GenerateLevelActorsToken must be declared as a replicated UPROPERTY "
			 "(client-side OnRep is the only signal that produces the completion broadcast)."),
		IsReplicatedByName(TEXT("GenerateLevelActorsToken")));

	TestTrue(
		TEXT("B9: AbilitySystemComponent must be declared as a replicated UPROPERTY "
			 "(environmental abilities require the ASC reference on clients)."),
		IsReplicatedByName(TEXT("AbilitySystemComponent")));

	// -----------------------------------------------------------------------
	// (b) Source-file inspection: GetLifetimeReplicatedProps must actually
	// register each replicated property in its body. The stub body only
	// calls Super, so all three name-bound DOREPLIFETIME-macro checks fail.
	// -----------------------------------------------------------------------
	const FString SourcePath =
		FPaths::ProjectDir() / TEXT("Source/Bomber/Private/Actors/BmrGeneratedMap.cpp");
	const FString AbsPath = FPaths::ConvertRelativePathToFull(SourcePath);

	FString Source;
	if (!TestTrue(
			FString::Printf(TEXT("BmrGeneratedMap.cpp must be readable at %s"), *AbsPath),
			FFileHelper::LoadFileToString(Source, *AbsPath)))
	{
		return false;
	}

	// Locate the function definition (the qualified definition, not a forward
	// declaration), then scope subsequent checks to the body of that function
	// only — bounded by the next `void ABmrGeneratedMap::` member definition.
	const int32 DefIdx = Source.Find(
		TEXT("void ABmrGeneratedMap::GetLifetimeReplicatedProps"),
		ESearchCase::CaseSensitive);
	if (!TestTrue(
			TEXT("B9: BmrGeneratedMap.cpp must define ABmrGeneratedMap::GetLifetimeReplicatedProps"),
			DefIdx != INDEX_NONE))
	{
		return false;
	}

	const int32 BodyStart = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DefIdx);
	if (!TestTrue(
			TEXT("B9: ABmrGeneratedMap::GetLifetimeReplicatedProps must have a body"),
			BodyStart != INDEX_NONE))
	{
		return false;
	}

	// End of body: matching closing brace. The body is short and contains no
	// nested braces in idiomatic implementations, but be defensive against
	// initializer lists / nested scopes by tracking depth.
	int32 BodyEnd = INDEX_NONE;
	{
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
					BodyEnd = i;
					break;
				}
			}
		}
	}
	if (!TestTrue(
			TEXT("B9: ABmrGeneratedMap::GetLifetimeReplicatedProps body must have a matching closing brace"),
			BodyEnd != INDEX_NONE))
	{
		return false;
	}

	const FString FuncBody = Source.Mid(BodyStart, BodyEnd - BodyStart + 1);

	// True when the function body contains a `DOREPLIFETIME...(...)` macro
	// invocation that mentions PropertyName as one of its arguments. Accepts
	// any DOREPLIFETIME variant (plain, _CONDITION, _WITH_PARAMS,
	// _WITH_PARAMS_FAST, _CONDITION_NOTIFY, etc.) — what matters is that the
	// property is registered, not which exact variant is used.
	auto BodyRegistersProperty = [&FuncBody](const TCHAR* PropertyName) -> bool
	{
		const int32 PropertyNameLen = FCString::Strlen(PropertyName);
		int32 SearchFrom = 0;
		while (SearchFrom < FuncBody.Len())
		{
			const int32 MacroIdx = FuncBody.Find(
				TEXT("DOREPLIFETIME"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (MacroIdx == INDEX_NONE)
			{
				return false;
			}
			const int32 OpenParen = FuncBody.Find(
				TEXT("("), ESearchCase::CaseSensitive, ESearchDir::FromStart, MacroIdx);
			const int32 CloseParen = (OpenParen != INDEX_NONE)
				? FuncBody.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenParen)
				: INDEX_NONE;
			if (OpenParen == INDEX_NONE || CloseParen == INDEX_NONE)
			{
				return false;
			}
			const FString MacroArgs = FuncBody.Mid(OpenParen + 1, CloseParen - OpenParen - 1);
			const int32 PropIdx = MacroArgs.Find(PropertyName, ESearchCase::CaseSensitive);
			if (PropIdx != INDEX_NONE)
			{
				// Whole-word match: surrounding chars must not be identifier
				// chars, otherwise "MapComponents" would falsely match a
				// "MapComponentsContainer" argument.
				const bool bLeftOk = (PropIdx == 0) ||
					(!FChar::IsAlnum(MacroArgs[PropIdx - 1]) && MacroArgs[PropIdx - 1] != TCHAR('_'));
				const int32 RightIdx = PropIdx + PropertyNameLen;
				const bool bRightOk = (RightIdx >= MacroArgs.Len()) ||
					(!FChar::IsAlnum(MacroArgs[RightIdx]) && MacroArgs[RightIdx] != TCHAR('_'));
				if (bLeftOk && bRightOk)
				{
					return true;
				}
			}
			SearchFrom = CloseParen + 1;
		}
		return false;
	};

	TestTrue(
		TEXT("B9: ABmrGeneratedMap::GetLifetimeReplicatedProps must register MapComponents via a "
			 "DOREPLIFETIME-prefixed macro (e.g. DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MapComponents, Params)). "
			 "Without this, the FastArray of spawned level actors never replicates and clients see an empty grid. "
			 "start/ body only calls Super and therefore registers nothing."),
		BodyRegistersProperty(TEXT("MapComponents")));

	TestTrue(
		TEXT("B9: ABmrGeneratedMap::GetLifetimeReplicatedProps must register GenerateLevelActorsToken via a "
			 "DOREPLIFETIME-prefixed macro. The client-side OnRep on this token is the only signal that "
			 "produces the completion broadcast in the catch-up branch; start/ stub registers nothing."),
		BodyRegistersProperty(TEXT("GenerateLevelActorsToken")));

	TestTrue(
		TEXT("B9: ABmrGeneratedMap::GetLifetimeReplicatedProps must register AbilitySystemComponent via a "
			 "DOREPLIFETIME-prefixed macro. Environmental abilities require the replicated ASC reference on "
			 "clients; start/ stub registers nothing."),
		BodyRegistersProperty(TEXT("AbilitySystemComponent")));

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — PIE runtime pipeline (single standalone PIE session)
//
// Phases:
//   0. Bring up PIE on /Game/Bomber/Maps/Main and locate the Generated Map.
//      Wait for the initial async generation to complete.
//   1. B1 DIRECT — verify the initial population produced level actors.
//   2. B5/B11 DIRECT — drive an explicit regenerate; observe completion via
//      the replicated token (written immediately before
//      OnGeneratedLevelActors.Broadcast() in the solution).
//      Verify the broadcast-line invariants: MapComponents populated, token
//      advanced past the pre-call snapshot.
//   3. B4/B14 DIRECT — after multiple regenerates of the same size, the level's
//      Actors.Num() must not scale with the regenerate count, AND map-component
//      instances must overlap with the first round (proving Pool Manager reuse,
//      not destroy-and-respawn).
//   4. B3/B10 DIRECT — drive a regenerate at 41×41 and verify the synchronous
//      portion of SetLevelSize()/GenerateLevelActors() takes only a small
//      fraction of a generous budget on the main thread. A synchronous-on-
//      game-thread implementation of the cell-graph computation at 1681 cells
//      blows past this budget.
//   5. B6 DIRECT — SetLevelSize updates the actor's scale to match.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGeneratedMapOrchestrator_PIE_Lifecycle,
	"Bomber.GeneratedMapOrchestrator.PIE_Lifecycle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeneratedMapOrchestrator_PIE_Lifecycle::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace GeneratedMapOrchestratorTests;

	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	// false = standalone PIE; HasAuthority is true, so the server-side
	// orchestration runs exactly as it does on a listen-server host.
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// -----------------------------------------------------------------------
	// Phase 0a — locate the Generated Map; the subsystem may take a moment.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetServerPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		if (ABmrGeneratedMap* Map = FindGeneratedMap(World))
		{
			GRunState.GeneratedMap = Map;
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (!GRunState.GeneratedMap.IsValid())
		{
			if (ABmrGeneratedMap* Map = FindGeneratedMap(GetServerPIEWorld()))
			{
				GRunState.GeneratedMap = Map;
			}
		}
		TestTrue(TEXT("ABmrGeneratedMap actor must be placed in /Game/Bomber/Maps/Main"),
			GRunState.GeneratedMap.IsValid());
		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 0b — wait for the initial async generation to produce level
	// actors. In the stripped state the empty GenerateLevelActors() stub
	// produces nothing and the wait times out — Phase 1 then fails on
	// MapComponents.Num() > 0.
	// -----------------------------------------------------------------------
	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + DataAssetReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (Map && IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	// -----------------------------------------------------------------------
	// Phase 1 — B1: initial round-start population produced level actors.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must still be valid after PIE bootstrap"), Map))
		{
			return true;
		}

		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);
		const int32 InitialCount = Container.Num();

		TestTrue(
			FString::Printf(TEXT("B1: Initial generation must populate MapComponents (>0). Got %d. "
								 "An empty GenerateLevelActors() stub, a missing async-finish step, "
								 "or a never-broadcast completion leaves the grid empty here."),
				InitialCount),
			InitialCount > 0);

		// At minimum the base generator places 4 players at corners.
		TestTrue(
			FString::Printf(TEXT("B1: Initial generation should produce at least the corner players (>=4). Got %d."),
				InitialCount),
			InitialCount >= 4);

		GRunState.BaselineLevelActors = CountLevelActors(Map->GetWorld());
		GRunState.BaselineComponents = SnapshotMapComponentSet(Map);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 2 — B5/B11: drive an explicit regenerate, verify completion
	// invariants. The broadcast-line observable in GenerateLevelActors_Finish
	// is `GenerateLevelActorsToken = LocalReplicationToken` immediately
	// before `Broadcast()`. The solution's regenerate path synchronously
	// resets the token to 0 inside DestroyAllLevelActors() before queuing
	// the async cell-graph work, and only writes the token back to a
	// positive value once OnSpawned fires. So observing
	// (a) MapComponents populated and (b) Token > 0 after waiting proves
	// the broadcast fired for the new run.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!Map)
		{
			return true;
		}

		const double Start = FPlatformTime::Seconds();
		Map->GenerateLevelActors();
		const double End = FPlatformTime::Seconds();
		GRunState.GenerateLevelActorsMainThreadSeconds = End - Start;
		return true;
	}));

	// Wait for the new regenerate to complete. The synchronous reset inside
	// GenerateLevelActors() already clamped the token to 0 before this loop
	// starts, so polling for `Token > 0` correctly waits for the new run
	// rather than returning instantly on the previous run's state.
	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + GenerationTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (Map && IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must remain valid during regenerate"), Map))
		{
			return true;
		}

		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);

		// B11 DIRECT: when the completion signal is emitted, MapComponents
		// must be populated — the broadcast must not arrive before any actor
		// is registered.
		TestTrue(
			FString::Printf(TEXT("B5/B11: After regenerate, MapComponents must be populated (Num=%d). "
								 "A broadcast that fires from the server's spawn path before any actor "
								 "is registered here would leave Num=0."),
				Container.Num()),
			Container.Num() > 0);

		// The token observable for the broadcast line. The solution writes
		// `GenerateLevelActorsToken = LocalReplicationToken` immediately
		// before `OnGeneratedLevelActors.Broadcast()` on the completion path
		// (after first zeroing it in DestroyAllLevelActors). Observing the
		// token back at a positive value with a populated container is the
		// strongest indirect proof the broadcast was emitted for this run.
		const int32 TokenAfter = ReadIntProperty(Map, TEXT("GenerateLevelActorsToken"));

		TestTrue(
			FString::Printf(TEXT("B5: GenerateLevelActorsToken must be positive after regenerate "
								 "completes (TokenAfter=%d). The completion token is what gates the "
								 "OnGeneratedLevelActors broadcast on every peer."),
				TokenAfter),
			TokenAfter > 0);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 3 — B4/B14: drive several regenerates and verify Actors.Num()
	// stays bounded; map-component instances must overlap with the first
	// round (pool reuse, not destroy-and-respawn).
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!Map)
		{
			return true;
		}

		// Refresh baseline after Phase 2's regenerate so we compare against
		// post-regen state (Phase 1 baseline was pre-Phase-2-regen).
		GRunState.BaselineComponents = SnapshotMapComponentSet(Map);
		GRunState.BaselineLevelActors = CountLevelActors(Map->GetWorld());

		Map->GenerateLevelActors();
		return true;
	}));

	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + GenerationTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (!Map)
			{
				return true;
			}
			// The synchronous reset inside GenerateLevelActors() already
			// clamped Token to 0 before this loop starts, so polling for
			// Token > 0 waits for the new run rather than returning
			// instantly on the previous run's state.
			if (IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	// Drain another frame so any queued duplicate work would complete too.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must remain valid after multi-regenerate"), Map))
		{
			return true;
		}

		// B4 DIRECT: world actor count must not grow proportionally with
		// regenerate count. Tolerance accommodates incidental engine actors
		// (e.g. pool factory transient holders) added during regen.
		const int32 CurrentLevelActors = CountLevelActors(Map->GetWorld());
		const int32 Baseline = GRunState.BaselineLevelActors;
		const int32 GrowthAllowance = FMath::Max(16, Baseline / 4);

		TestTrue(
			FString::Printf(TEXT("B4/B14: GetCurrentLevel()->Actors.Num() must not grow proportionally "
								 "across regenerates. Baseline=%d, after multi-regenerate=%d, "
								 "tolerance=%d. A destroy-and-respawn implementation explodes this count "
								 "before GC reclaims the old instances."),
				Baseline, CurrentLevelActors, GrowthAllowance),
			CurrentLevelActors <= Baseline + GrowthAllowance);

		// B4 DIRECT: the same UBmrMapComponent instances should still be
		// present after a same-size regenerate. The Pool Manager hands the
		// previous round's pooled actors (and their map components) back to
		// the next round. A destroy+respawn implementation always produces
		// brand-new component pointers — Shared would be 0.
		const TSet<TWeakObjectPtr<const UBmrMapComponent>>& Before = GRunState.BaselineComponents;
		const TSet<TWeakObjectPtr<const UBmrMapComponent>> After = SnapshotMapComponentSet(Map);

		int32 Shared = 0;
		for (const TWeakObjectPtr<const UBmrMapComponent>& Weak : After)
		{
			if (Before.Contains(Weak))
			{
				++Shared;
			}
		}

		if (Before.Num() > 0 && After.Num() > 0)
		{
			TestTrue(
				FString::Printf(TEXT("B4/B14: Regenerate of the same level size must reuse existing "
									 "level-actor instances via the Pool Manager rather than destroying "
									 "and respawning. Shared UBmrMapComponent count across regenerate=%d "
									 "(Before=%d, After=%d). A destroy+respawn implementation yields Shared=0."),
					Shared, Before.Num(), After.Num()),
				Shared > 0);
		}

		return true;
	}));

	// -----------------------------------------------------------------------
	// Phase 4 — B3/B10: the synchronous main-thread portion of
	// SetLevelSize()/GenerateLevelActors() must be small even at moderate
	// sizes. The cell-graph computation is the expensive part and must run
	// off-thread. Phase 5 (B6) is observed in the same step: scale must have
	// updated. 21x21 (441 cells) is large enough that a synchronous-on-game-
	// thread cell-graph still blows the budget, but small enough to avoid
	// destabilizing the editor on slow CI hosts.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!Map)
		{
			return true;
		}

		// SetLevelSize() requires both dimensions > 0; ActorTransformToGridTransform
		// snaps them to odd values, so 21,21 is preserved.
		const double Start = FPlatformTime::Seconds();
		Map->SetLevelSize(FIntPoint(21, 21));
		const double End = FPlatformTime::Seconds();
		GRunState.GenerateLevelActorsMainThreadSeconds = End - Start;
		return true;
	}));

	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + GenerationTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (Map && IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must remain valid after 41x41 regenerate"), Map))
		{
			return true;
		}

		// B6 DIRECT: after SetLevelSize(21,21), the actor scale must match.
		// On a connected client the same transform replicates via the standard
		// actor-transform replication path.
		const FVector Scale = Map->GetActorScale3D();
		TestEqual(TEXT("B6: SetLevelSize must update actor scale X to 21"),
			static_cast<int32>(FMath::RoundToInt32(Scale.X)), static_cast<int32>(21));
		TestEqual(TEXT("B6: SetLevelSize must update actor scale Y to 21"),
			static_cast<int32>(FMath::RoundToInt32(Scale.Y)), static_cast<int32>(21));

		// B3/B10 DIRECT: the synchronous portion of GenerateLevelActors()
		// (dispatch + grid-cell build, NOT the cell-graph computation) must
		// be far under any reasonable per-frame budget. A 1 s budget is very
		// generous on slow CI hosts; a synchronous-on-game-thread
		// implementation of the cell-graph computation at 21×21 (441 cells)
		// still blows past this.
		constexpr double MainThreadBudgetSeconds = 1.0;
		TestTrue(
			FString::Printf(TEXT("B3/B10: SetLevelSize()/GenerateLevelActors() must dispatch the "
								 "cell-graph computation off the game thread. Synchronous portion "
								 "took %.4f s at 21x21 (budget %.4f s). A synchronous-on-game-thread "
								 "implementation blows past this."),
				GRunState.GenerateLevelActorsMainThreadSeconds, MainThreadBudgetSeconds),
			GRunState.GenerateLevelActorsMainThreadSeconds < MainThreadBudgetSeconds);

		// B3 corollary: the 41x41 regenerate must actually produce a level
		// (the token must advance to a positive value, mirroring Phase 2).
		const int32 TokenAfter = ReadIntProperty(Map, TEXT("GenerateLevelActorsToken"));
		TestTrue(
			FString::Printf(TEXT("B3/B6: SetLevelSize must trigger a regenerate that completes "
								 "(GenerateLevelActorsToken advanced, got %d)."),
				TokenAfter),
			TokenAfter > 0);

		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);
		TestTrue(
			FString::Printf(TEXT("B6: After SetLevelSize, the new generation must produce level actors "
								 "(MapComponents.Num=%d)."),
				Container.Num()),
			Container.Num() > 0);

		return true;
	}));

	// -----------------------------------------------------------------------
	// Teardown — stop PIE, clear shared state.
	// -----------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));

#else
	AddWarning(TEXT("PIE-based lifecycle test skipped: requires WITH_EDITOR."));
#endif

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Container dirty-mark API + OnRep token branches
//
// B8/B15 DIRECT: the replicated MapComponents container is a fast array, and
//                every mutation (add or remove) must go through MarkItemDirty
//                (which forwards to MarkArrayDirty). The standard wire-visible
//                side effect of MarkArrayDirty is FFastArraySerializer::
//                ArrayReplicationKey ticking forward. A raw `Items.Add` /
//                `Items.Remove` (the B15 failure mode that works on
//                listen-server and silently desyncs connected clients) leaves
//                ArrayReplicationKey at its initial value. After a populated
//                round it must be strictly positive, and after a regenerate
//                it must advance again — both invariants below.
//
// B13 DIRECT: when the server starts a fresh generation it clamps the
//             replicated GenerateLevelActorsToken to zero. The client-side
//             OnRep_GenerateLevelActorsToken must, on seeing that packet,
//             reset the local replication counter to zero so the next round's
//             IncrementReplicationToken calls count from a clean baseline.
//             We stage this exact state on the server-side instance (the
//             function's logic is purely a function of the actor's fields,
//             so server vs. client doesn't change which branch fires) and
//             verify the reset side effect.
//
// B12 PARTIAL: a second OnRep invocation on the same (token=0) state must
//              remain idempotent (no growth, no negative drift, no crash) —
//              this is the structural guard against the "broadcast fires
//              twice on a client when packets arrive in a particular order"
//              failure mode.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGeneratedMapOrchestrator_ContainerAndTokenBranches,
	"Bomber.GeneratedMapOrchestrator.ContainerAndTokenBranches",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeneratedMapOrchestrator_ContainerAndTokenBranches::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace GeneratedMapOrchestratorTests;

	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	// Locate the Generated Map.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (ABmrGeneratedMap* Map = FindGeneratedMap(GetServerPIEWorld()))
		{
			GRunState.GeneratedMap = Map;
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	// Wait for the initial generation to complete (MapComponents populated + token > 0).
	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + DataAssetReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			if (!GRunState.GeneratedMap.IsValid())
			{
				if (ABmrGeneratedMap* Map = FindGeneratedMap(GetServerPIEWorld()))
				{
					GRunState.GeneratedMap = Map;
				}
			}
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (Map && IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	// ---- B8/B15 DIRECT: container dirty-mark advances ArrayReplicationKey ----
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must be valid after PIE bootstrap"), Map))
		{
			return true;
		}

		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);

		// FBmrMapComponentsContainer : FIrisFastArraySerializer : FFastArraySerializer.
		// FFastArraySerializer::ArrayReplicationKey is incremented by MarkArrayDirty
		// (called transitively from MarkItemDirty). A raw `Items.Add` / `Items.Remove`
		// implementation never touches the counter, so it would stay at 0.
		TestTrue(
			FString::Printf(TEXT("B8/B15: After the initial round of generation populates "
								 "MapComponents (Num=%d), the fast-array's ArrayReplicationKey "
								 "must be > 0 (got %d). Raw TArray::Add/Remove leaves this counter "
								 "untouched and would silently desync connected clients."),
				Container.Num(), Container.ArrayReplicationKey),
			Container.ArrayReplicationKey > 0);

		// Snapshot for the post-regenerate advance check below.
		GRunState.BaselineLevelActors = Container.ArrayReplicationKey;
		return true;
	}));

	// Drive an explicit regenerate so the container churns through both
	// the Remove path (DestroyAllLevelActors) and the Add path (SpawnActorsByTypes).
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get())
		{
			Map->GenerateLevelActors();
		}
		return true;
	}));

	{
		const double DeadlineSeconds = FPlatformTime::Seconds() + GenerationTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([DeadlineSeconds]() -> bool
		{
			ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
			if (Map && IsGenerationComplete(Map))
			{
				return true;
			}
			return FPlatformTime::Seconds() >= DeadlineSeconds;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must remain valid through regenerate"), Map))
		{
			return true;
		}

		const FBmrMapComponentsContainer& Container = GeneratedMapPrivateAccess::GetMapComponents(Map);
		const int32 KeyAfter = Container.ArrayReplicationKey;
		const int32 KeyBefore = GRunState.BaselineLevelActors;

		// Both the Remove path (in DestroyAllLevelActors → RemoveFromGrid) and
		// the Add path (in AddToGrid via SpawnActorsByTypes) must invoke the
		// dirty-mark API. A regenerate that destroys + respawns the same number
		// of actors with raw TArray::Add/Remove would leave the key unchanged.
		TestTrue(
			FString::Printf(TEXT("B8/B15: After a regenerate the fast-array's ArrayReplicationKey "
								 "must advance (before=%d, after=%d). Each Remove (RemoveFromGrid) "
								 "and Add (AddToGrid) is required to go through the container's "
								 "MarkItemDirty API — raw TArray mutations leave this counter flat."),
				KeyBefore, KeyAfter),
			KeyAfter > KeyBefore);

		return true;
	}));

	// ---- B13 DIRECT: OnRep_GenerateLevelActorsToken reset-on-zero branch ----
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		ABmrGeneratedMap* Map = GRunState.GeneratedMap.Get();
		if (!TestNotNull(TEXT("Generated Map must be valid for OnRep branch test"), Map))
		{
			return true;
		}

		FBmrMapComponentsContainer* Container = GeneratedMapPrivateAccess::GetMutableMapComponents(Map);
		if (!TestNotNull(TEXT("MapComponents container must be accessible via reflection"), Container))
		{
			return true;
		}

		// Stage a wire-state that ONLY a correct OnRep can untangle:
		//   server-just-started-new-round packet → GenerateLevelActorsToken = 0
		//   plus a stale LocalReplicationToken from a previous round.
		const int32 SavedToken = ReadIntProperty(Map, TEXT("GenerateLevelActorsToken"));
		const int32 SavedLocal = Container->LocalReplicationToken;

		Container->LocalReplicationToken = 99;
		TestTrue(TEXT("Pre-condition: GenerateLevelActorsToken UPROPERTY must be writable via reflection"),
			WriteIntProperty(Map, TEXT("GenerateLevelActorsToken"), 0));

		// Trigger the OnRep callback directly via UFunction reflection. The OnRep
		// is `UFUNCTION()` no-arg, so ProcessEvent with null params is correct.
		TestTrue(TEXT("B13: OnRep_GenerateLevelActorsToken must be discoverable as a UFUNCTION "
					  "(must be declared with UFUNCTION() to be wired as a ReplicatedUsing handler)"),
			CallNoArgUFunction(Map, TEXT("OnRep_GenerateLevelActorsToken")));

		TestEqual(
			TEXT("B13: When the replicated token resets to 0 (server signalling a new generation), "
				 "OnRep must zero the local replication counter so subsequent IncrementReplicationToken "
				 "calls count from a clean baseline. Without this reset, a client that observed a partial "
				 "previous round would never broadcast for the new round."),
			Container->LocalReplicationToken, 0);

		// ---- B12 PARTIAL: idempotence — repeating the OnRep on the same
		// (token=0) state must not corrupt counters or trigger any growth.
		CallNoArgUFunction(Map, TEXT("OnRep_GenerateLevelActorsToken"));
		TestEqual(
			TEXT("B12: A repeated OnRep_GenerateLevelActorsToken on the same (token=0) state "
				 "must remain idempotent — LocalReplicationToken must still be 0 (no double-broadcast "
				 "side effects, no negative drift)."),
			Container->LocalReplicationToken, 0);

		// Restore the staged state so teardown proceeds without polluting it.
		if (SavedToken != INDEX_NONE)
		{
			WriteIntProperty(Map, TEXT("GenerateLevelActorsToken"), SavedToken);
		}
		Container->LocalReplicationToken = SavedLocal;
		return true;
	}));

	// Teardown.
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));

#else
	AddWarning(TEXT("Container/OnRep branch test skipped: requires WITH_EDITOR."));
#endif

	return true;
}
