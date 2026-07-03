// Copyright (c) 2026 GameDevBench. Player-state behavior automation tests (Bomber).
//
// Tests target the single stripped file in this task:
//   Source/Bomber/Private/GameFramework/BmrPlayerState.cpp
//
// ABmrPlayerState is the persistent identity for a player — it survives pawn
// pool round-trips, owns the Gameplay Ability System component, holds the
// replicated chosen-character mesh data, and runs end-game tabulation.
// The behaviors the spec calls out:
//   * GAS init goes through the engine's FAttributeSetInitter — NOT
//     per-attribute SetNumericAttributeBase writes (B1 failure mode).
//   * SetChosenMeshData is authority-only on the assignment path; non-authority
//     locally-controlled clients route via ServerSetChosenMeshData (B4 failure
//     mode); OnRep_ChosenMeshData and the authority path both converge on
//     ApplyChosenMeshData which broadcasts OnChosenMeshDataChanged.
//   * GetPlayerTag resolves from the replicated ChosenMeshData row — NOT from
//     the pawn — so it survives pool round-trips while the pawn is absent.
//   * ApplyIsABot flips the ASC's EGameplayEffectReplicationMode between
//     Minimal (bot) and Mixed (human).
//   * ApplyIsPlayerDead defers UpdateEndGameState to next tick — never
//     synchronous in the same frame (B2 failure mode).
//   * OnDeactivated is suppressed — must NOT chain Super::OnDeactivated() or
//     the player state is destroyed on disconnect, breaking pool reuse on
//     reconnect (B3 failure mode).
//
// Strategy: PIE-based runtime tests for the behaviors observable through a
// directly-spawned ABmrPlayerState (field assignment, derived getters, ASC
// replication mode). Source inspection covers behaviors that are inherently
// multi-machine (non-authority client → server RPC routing), behaviors
// driven by engine-internal lifecycle hooks that aren't reflection-callable
// (OnDeactivated), and the broadcast → ApplyChosenMeshData chain (the
// delegate is a DYNAMIC_MULTICAST, which UE binds only to UFUNCTIONs on
// UObjects — not callable from a free test class — so the broadcast itself
// can't be directly observed without a UCLASS test helper that would need
// its own .generated.h, which a single test .cpp can't provide).

// Bomber (game module — kept at top per task rules)
#include "GameFramework/BmrPlayerState.h"
#include "Bomber.h" // EBmrEndGameState
#include "DataRegistries/BmrPlayerRow.h"
#include "Structures/BmrMeshData.h"
#include "Structures/BmrPlayerTag.h"

// UE
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogPlayerStateTests, Log, All);

namespace PlayerStateTests
{
	// The stripped file the task targets. spec.yaml lists the file under
	// "PlayerState/" but the actual on-disk path is "GameFramework/".
	static const TCHAR* PlayerStateRelPath =
		TEXT("Source/Bomber/Private/GameFramework/BmrPlayerState.cpp");

	// The Bomber Main map carries enough of the gameplay subsystem registration
	// (GAS globals, gameplay tags) for an ABmrPlayerState spawned in PIE to
	// initialise sanely. The PlayerState tests don't depend on placed actors
	// or map cells — just on a fully-bootstrapped game world.
	static const TCHAR* MainMapPath = TEXT("/Game/Bomber/Maps/Main");

	// Generation of the Main map is async (data-asset registry + cell graph).
	// Give it generous walls on slow CI hosts.
	static constexpr float MapReadyTimeoutSeconds = 30.f;

	struct FRunState
	{
		TWeakObjectPtr<ABmrPlayerState> PS;
		bool bWorldReady = false;
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

	// Spawn an ABmrPlayerState in a PIE world. PlayerState has no hard runtime
	// dependency on a controller / pawn — every getter under test either reads
	// its own replicated state (GetPlayerTag from ChosenMeshData) or routes
	// through subsystems the world already provides.
	static ABmrPlayerState* SpawnPlayerState(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ABmrPlayerState>(
			ABmrPlayerState::StaticClass(), FTransform::Identity, SpawnParams);
	}

	// Load BmrPlayerState.cpp and reuse the body extractor / comment stripper
	// from the other Bomber tests so substring scans don't false-match against
	// anti-pattern descriptions written in comments.
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
} // namespace PlayerStateTests

// ---------------------------------------------------------------------------
// Test 1 — PIE: SetChosenMeshData on authority writes the field.
//
// B7 DIRECT: when SetChosenMeshData is called on an authoritative PlayerState
// with a valid FBmrMeshData, the field must be assigned. The same call also
// drives ApplyChosenMeshData (the broadcast pathway) — but observing the
// broadcast directly is impractical: OnChosenMeshDataChanged is a
// DYNAMIC_MULTICAST_DELEGATE, which UE binds only to UFUNCTIONs on UObjects.
// A test-class lambda cannot subscribe (no AddLambda on dynamic delegates),
// and adding a UCLASS test helper would require a .generated.h that a single
// test .cpp can't provide. The broadcast wiring is covered by Test 2 via
// source inspection of the body chain.
//
// Standalone PIE actors are authoritative (HasAuthority()==true), so the
// `!HasAuthority()` early-return path is not taken here — this test
// exercises the assignment path directly.
//
// The discriminator: a stub body (the start/ state) leaves the field
// untouched. A body that ROUTES to the server RPC unconditionally (without
// checking HasAuthority) would also leave the field untouched on the
// authority side because ServerSetChosenMeshData has no useful effect on
// the local authority pass — only re-entering SetChosenMeshData under the
// authority branch produces the field write.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_PIE_AuthoritySetChosenMeshDataWritesField,
	"Bomber.PlayerState.PIE_AuthoritySetChosenMeshDataWritesField",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_PIE_AuthoritySetChosenMeshDataWritesField::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace PlayerStateTests;
	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	{
		const double Deadline = FPlatformTime::Seconds() + MapReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Deadline]() -> bool
		{
			UWorld* World = GetServerPIEWorld();
			if (World && World->HasBegunPlay())
			{
				GRunState.bWorldReady = true;
				return true;
			}
			return FPlatformTime::Seconds() >= Deadline;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetServerPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}
		if (!TestTrue(TEXT("PIE world must have begun play"), GRunState.bWorldReady))
		{
			return true;
		}

		ABmrPlayerState* PS = SpawnPlayerState(World);
		if (!TestNotNull(TEXT("ABmrPlayerState must be spawnable in PIE"), PS))
		{
			return true;
		}
		GRunState.PS = PS;

		if (!TestTrue(
				TEXT("Standalone PIE PlayerState must be authoritative — this test exercises the "
					 "authority branch of SetChosenMeshData."),
				PS->HasAuthority()))
		{
			return true;
		}

		// Initial state: ChosenMeshData defaults to Empty (RowName=None).
		// Without this fixture invariant the assignment side effect can't be
		// distinguished from a no-op.
		TestFalse(
			TEXT("Fresh PlayerState's ChosenMeshData must start as Empty (RowName=NAME_None) — "
				 "the test would otherwise not observe the assignment side effect."),
			PS->GetChosenMeshData().IsValid());

		// Build a valid mesh data (the canonical solution validates IsValid()
		// and SkinRowName.IsValid() — both names must be non-None).
		FBmrMeshData NewData;
		NewData.RowName = FName(TEXT("TestRow"));
		NewData.SkinRowName = FName(TEXT("TestSkin"));
		if (!TestTrue(TEXT("Test fixture: NewData.IsValid() must be true"), NewData.IsValid()))
		{
			return true;
		}

		PS->SetChosenMeshData(NewData);

		TestEqual(
			TEXT("B7: After authoritative SetChosenMeshData, ChosenMeshData.RowName must equal "
				 "the passed value. A body that early-returns on the authority path or only "
				 "routes via the server RPC without writing the field leaves this at None."),
			PS->GetChosenMeshData().RowName, NewData.RowName);

		TestEqual(
			TEXT("B7: After authoritative SetChosenMeshData, ChosenMeshData.SkinRowName must equal "
				 "the passed value. The whole-struct write is what makes the chosen-skin replicate "
				 "alongside the chosen-character row."),
			PS->GetChosenMeshData().SkinRowName, NewData.SkinRowName);

		// A second call with DIFFERENT data must also take effect — distinguishes
		// a one-shot stub that hard-codes a single assignment.
		FBmrMeshData OtherData;
		OtherData.RowName = FName(TEXT("OtherRow"));
		OtherData.SkinRowName = FName(TEXT("OtherSkin"));
		PS->SetChosenMeshData(OtherData);

		TestEqual(
			TEXT("B7: A second authoritative SetChosenMeshData with a DIFFERENT value must take "
				 "effect. Distinguishes an implementation that writes the field once and then "
				 "early-returns on all subsequent calls (e.g. an over-eager IsValid() guard that "
				 "treats any non-Empty current state as 'already set')."),
			PS->GetChosenMeshData().RowName, OtherData.RowName);

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));

#else
	AddWarning(TEXT("PIE-based SetChosenMeshData test skipped: requires WITH_EDITOR."));
#endif

	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — Source: SetChosenMeshData routes non-authority locally-controlled
//          clients through the server RPC, and both the authority + OnRep
//          paths funnel through ApplyChosenMeshData (B4 / B6 / B8 / B9).
//
// The spec's failure mode list, item 4: "Locally-controlled non-authority
// clients writing replicated properties directly instead of going through the
// server." The canonical SetChosenMeshData body:
//
//   if (!HasAuthority())
//   {
//       if (IsPlayerStateLocallyControlled())
//       {
//           ServerSetChosenMeshData(InMeshData);
//       }
//       return;
//   }
//   ChosenMeshData = InMeshData;  // authority writes only past this point
//   MARK_PROPERTY_DIRTY_FROM_NAME(...);
//   ApplyChosenMeshData();
//
// A re-impl that writes ChosenMeshData unconditionally produces a client-only
// write that the server never sees: the client's mesh changes locally, the
// server's view of the chosen character stays stale, the next replication
// pass reverts the client to the server's stale value. This is invisible in
// single-player PIE (authority is true), so runtime observation requires a
// real client/server pair — source inspection is the practical discriminator.
//
// The matching apply-broadcast chain (ApplyChosenMeshData → broadcast) is
// also covered here because the DYNAMIC_MULTICAST delegate can't be observed
// from a free test class — the only practical discriminator for the
// "broadcast on assignment" / "broadcast on OnRep" wiring is that both the
// authority assignment path and the OnRep handler call into
// ApplyChosenMeshData, and ApplyChosenMeshData calls the delegate.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_SetChosenMeshDataNonAuthorityRoutesAndApplyChain,
	"Bomber.PlayerState.SetChosenMeshDataNonAuthorityRoutesAndApplyChain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_SetChosenMeshDataNonAuthorityRoutesAndApplyChain::RunTest(const FString& Parameters)
{
	using namespace PlayerStateTests;

	const FString Source = LoadProjectFile(this, PlayerStateRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	// ----- SetChosenMeshData body: authority routing + apply call -----
	const FString SetBody = ExtractMemberBody(Source, TEXT("ABmrPlayerState::SetChosenMeshData"));
	if (!TestTrue(
			TEXT("BmrPlayerState.cpp must define SetChosenMeshData with a body — the start/ stub "
				 "leaves the body empty; the auth-routing + apply logic is what was stripped."),
			!SetBody.IsEmpty()))
	{
		return false;
	}

	const FString SetCode = StripComments(SetBody);

	TestTrue(
		TEXT("B4 failure mode: SetChosenMeshData must consult HasAuthority(). The canonical body "
			 "branches on authority; a body without the check writes the replicated field on "
			 "either side of the network and produces client-only writes the server never sees."),
		SetCode.Contains(TEXT("HasAuthority"), ESearchCase::CaseSensitive));

	TestTrue(
		TEXT("B4 failure mode: SetChosenMeshData must call ServerSetChosenMeshData on the "
			 "non-authority branch. The spec is explicit: \"Non-authority locally-controlled "
			 "clients request the change through the server.\" Without this call, a client's "
			 "SetChosenMeshData becomes a no-op (or worse, a local-only write that the next "
			 "replication pass reverts)."),
		SetCode.Contains(TEXT("ServerSetChosenMeshData"), ESearchCase::CaseSensitive));

	// The authority path must funnel through ApplyChosenMeshData — the shared
	// apply point. Without this, the server writes the field but never
	// broadcasts OnChosenMeshDataChanged, and any listener bound to the
	// delegate (menu camera, nameplate, outfit applier) misses every change
	// that originates on the authority.
	TestTrue(
		TEXT("B7 + B9: SetChosenMeshData's authority branch must call ApplyChosenMeshData — the "
			 "shared apply path that broadcasts OnChosenMeshDataChanged. Without this call the "
			 "server's authoritative write is invisible to local listeners on the authority "
			 "(menu camera, nameplate, outfit applier all subscribe to the broadcast)."),
		SetCode.Contains(TEXT("ApplyChosenMeshData"), ESearchCase::CaseSensitive));

	// ----- ServerSetChosenMeshData_Implementation must defer to the assignment -----
	const FString ServerImplBody = ExtractMemberBody(
		Source, TEXT("ABmrPlayerState::ServerSetChosenMeshData_Implementation"));
	if (!TestTrue(
			TEXT("B6 failure mode: ServerSetChosenMeshData_Implementation must have a body — the "
				 "start/ stub leaves the RPC body empty; without it, the client's request reaches "
				 "the server but does nothing."),
			!ServerImplBody.IsEmpty()))
	{
		return false;
	}

	const FString ServerImplCode = StripComments(ServerImplBody);
	const bool bRpcAssignsOrRoutes =
		ServerImplCode.Contains(TEXT("SetChosenMeshData"), ESearchCase::CaseSensitive)
		|| ServerImplCode.Contains(TEXT("ChosenMeshData ="), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("ServerSetChosenMeshData_Implementation must execute the authoritative assignment "
			 "(either by re-entering SetChosenMeshData — which on the server now runs the "
			 "authority branch — or by directly assigning the ChosenMeshData field). A body that "
			 "only logs or broadcasts leaves the server's source-of-truth stale."),
		bRpcAssignsOrRoutes);

	// ----- OnRep_ChosenMeshData must drive ApplyChosenMeshData -----
	const FString OnRepBody = ExtractMemberBody(
		Source, TEXT("ABmrPlayerState::OnRep_ChosenMeshData"));
	if (!TestTrue(
			TEXT("B8: BmrPlayerState.cpp must define OnRep_ChosenMeshData with a body — the "
				 "start/ stub leaves it empty; without the body, clients never react to the "
				 "replicated chosen-character update."),
			!OnRepBody.IsEmpty()))
	{
		return false;
	}

	const FString OnRepCode = StripComments(OnRepBody);
	TestTrue(
		TEXT("B8: OnRep_ChosenMeshData must call ApplyChosenMeshData (the shared apply path that "
			 "broadcasts OnChosenMeshDataChanged). A stub OnRep leaves clients with the replicated "
			 "field silently updated — every client-side listener (camera, nameplate, outfit "
			 "applier) misses the change. The spec requires server and client apply-paths to "
			 "converge on the same routine."),
		OnRepCode.Contains(TEXT("ApplyChosenMeshData"), ESearchCase::CaseSensitive));

	// ----- ApplyChosenMeshData must broadcast OnChosenMeshDataChanged -----
	const FString ApplyBody = ExtractMemberBody(
		Source, TEXT("ABmrPlayerState::ApplyChosenMeshData"));
	if (!TestTrue(
			TEXT("B9: BmrPlayerState.cpp must define ApplyChosenMeshData with a body — the start/ "
				 "stub leaves it empty; the broadcast wiring is what was stripped."),
			!ApplyBody.IsEmpty()))
	{
		return false;
	}

	const FString ApplyCode = StripComments(ApplyBody);
	TestTrue(
		TEXT("B9: ApplyChosenMeshData must broadcast OnChosenMeshDataChanged. The spec is "
			 "explicit: both the server apply-path and the client OnRep-path converge on this "
			 "function, which 'updates the pawn's visuals and broadcasts the change.' A body "
			 "that only touches the pawn's MapComponent but never broadcasts breaks every "
			 "non-pawn listener (the menu camera, the scoreboard, the outfit applier)."),
		ApplyCode.Contains(TEXT("OnChosenMeshDataChanged"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — PIE: GetPlayerTag resolves from ChosenMeshData, NOT from the pawn.
//
// B10 DIRECT: the spec is explicit — "the player tag derived from the chosen
// character must be resolvable even when the pawn is absent — it comes from
// the data asset row, not the pawn." The named failure mode is reading
// through GetPawn() and crashing (or returning None) whenever the pawn has
// been pool-returned mid-death. A correct implementation does its lookup
// against ChosenMeshData.RowName even with no possessed pawn.
//
// The PlayerState spawned here has NO possessed pawn — calling GetPlayerTag
// must therefore not depend on GetPawn(). With ChosenMeshData at its default
// (empty), the lookup yields FBmrPlayerTag::None. The discriminator: a stub
// that dereferences GetPawn() would crash on either call; a correct body
// returns None gracefully.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_PIE_GetPlayerTagReadsFromChosenMeshDataNotPawn,
	"Bomber.PlayerState.PIE_GetPlayerTagReadsFromChosenMeshDataNotPawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_PIE_GetPlayerTagReadsFromChosenMeshDataNotPawn::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace PlayerStateTests;
	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	{
		const double Deadline = FPlatformTime::Seconds() + MapReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Deadline]() -> bool
		{
			UWorld* World = GetServerPIEWorld();
			if (World && World->HasBegunPlay())
			{
				GRunState.bWorldReady = true;
				return true;
			}
			return FPlatformTime::Seconds() >= Deadline;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetServerPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}
		if (!TestTrue(TEXT("PIE world must have begun play"), GRunState.bWorldReady))
		{
			return true;
		}

		ABmrPlayerState* PS = SpawnPlayerState(World);
		if (!TestNotNull(TEXT("ABmrPlayerState must be spawnable in PIE"), PS))
		{
			return true;
		}

		// Confirm pawn-absent setup: an isolated PlayerState has no possessed
		// pawn. This is exactly the failure-mode case the spec calls out
		// ("pawn is pool-managed and can be absent during death"). A
		// GetPlayerTag impl that reaches through GetPawn() would crash here.
		if (!TestNull(
				TEXT("Test fixture: a directly-spawned PlayerState has no pawn — this is the "
					 "scenario the spec's pool-managed-pawn rationale targets."),
				PS->GetPawn()))
		{
			return true;
		}

		// Empty-ChosenMeshData call: the row lookup yields no match → return
		// FBmrPlayerTag::None. Crucially this call must not crash with the
		// pawn null — the spec's whole reason for owning the tag on the
		// PlayerState is exactly that the pawn is unreliable across pool
		// round-trips.
		const FBmrPlayerTag& EmptyTag = PS->GetPlayerTag();
		TestEqual(
			TEXT("B10: GetPlayerTag() on a PlayerState with empty ChosenMeshData must return "
				 "FBmrPlayerTag::None. The lookup goes through FBmrPlayerRow::GetRowByName "
				 "(ChosenMeshData.RowName) — when the row name is None, no row matches, and the "
				 "spec-mandated fallback is None."),
			EmptyTag, FBmrPlayerTag::None);

		FStructProperty* MeshDataProp = CastField<FStructProperty>(
			ABmrPlayerState::StaticClass()->FindPropertyByName(TEXT("ChosenMeshData")));
		if (!TestNotNull(TEXT("ChosenMeshData must be a UPROPERTY"), MeshDataProp))
		{
			return true;
		}

		// First, write ChosenMeshData with a non-existent row name and confirm
		// GetPlayerTag still returns None (the row lookup misses, so the
		// fallback fires). The point of THIS write is to prove that the
		// getter consults ChosenMeshData.RowName at all — a body that always
		// dereferences the pawn would have crashed on either call.
		{
			FBmrMeshData NonExistentData;
			NonExistentData.RowName = FName(TEXT("__NonExistentRow_GetPlayerTagTest__"));
			NonExistentData.SkinRowName = FName(TEXT("__NonExistentSkin__"));
			if (FBmrMeshData* MeshDataPtr = MeshDataProp->ContainerPtrToValuePtr<FBmrMeshData>(PS))
			{
				*MeshDataPtr = NonExistentData;
			}

			const FBmrPlayerTag& PopulatedTag = PS->GetPlayerTag();
			TestEqual(
				TEXT("B10: GetPlayerTag() with a ChosenMeshData.RowName that resolves to no "
					 "registered row must return FBmrPlayerTag::None — the canonical fallback. "
					 "The test name carries '__NonExistent__' to keep this immune to the actual "
					 "registered character rows. Crucially this call MUST NOT reach through the "
					 "pawn (still null) — the spec's whole reason for owning the tag on the "
					 "PlayerState is exactly that the pawn is unreliable across pool round-trips."),
				PopulatedTag, FBmrPlayerTag::None);
		}

		// Discriminator: resolve a REAL registered row from the data registry
		// via the Default player tag (the canonical fallback character — every
		// map registers one under this tag). Write that row name into
		// ChosenMeshData and verify GetPlayerTag returns the matching non-None
		// tag.
		//
		// This is the case the stripped stub cannot satisfy: a body that
		// hard-returns FBmrPlayerTag::None — or one that always reads through
		// GetPawn() (the named B10 failure mode) — gives back None here, but
		// a correct ChosenMeshData → FBmrPlayerRow::GetRowByName → PlayerTag
		// chain yields a populated FBmrPlayerTag with a Player.* gameplay tag.
		// Without this positive case the test would also pass against a stub
		// since both None-checks above are satisfied by an unconditional
		// "return None".
		const FName DefaultRowName = FBmrPlayerRow::GetRowNameByPlayerTag(FBmrPlayerTag::Default);
		if (!TestTrue(
				TEXT("Test fixture: FBmrPlayerRow::GetRowNameByPlayerTag(FBmrPlayerTag::Default) "
					 "must resolve to a registered row. The Main map's data registries must be "
					 "loaded by the time PIE world has begun play — without a registered Default "
					 "character row, the test cannot discriminate a hard-coded 'return None' stub "
					 "from a correct ChosenMeshData → row lookup implementation."),
				!DefaultRowName.IsNone()))
		{
			return true;
		}

		{
			FBmrMeshData RegisteredData;
			RegisteredData.RowName = DefaultRowName;
			RegisteredData.SkinRowName = FName(TEXT("__AnySkin__"));
			if (FBmrMeshData* MeshDataPtr = MeshDataProp->ContainerPtrToValuePtr<FBmrMeshData>(PS))
			{
				*MeshDataPtr = RegisteredData;
			}

			const FBmrPlayerTag& RegisteredTag = PS->GetPlayerTag();
			TestNotEqual(
				TEXT("B10: GetPlayerTag() with a ChosenMeshData.RowName that resolves to a "
					 "registered FBmrPlayerRow must NOT return FBmrPlayerTag::None — it must "
					 "yield the row's PlayerTag. A stub body that hard-returns None passes the "
					 "empty/non-existent cases above by accident but fails here, because the "
					 "spec requires the getter to actually consult the data registry. The pawn "
					 "is still null at this point — proving the lookup goes through "
					 "ChosenMeshData (the replicated identity field) rather than the pawn."),
				RegisteredTag, FBmrPlayerTag::None);

			TestEqual(
				TEXT("B10: GetPlayerTag() with a ChosenMeshData.RowName resolving to the Default "
					 "character row must return FBmrPlayerTag::Default — i.e. the row's stored "
					 "PlayerTag. The row name was resolved via "
					 "FBmrPlayerRow::GetRowNameByPlayerTag(FBmrPlayerTag::Default), so the "
					 "round-trip through ChosenMeshData.RowName must land back on the same tag. "
					 "A getter that returns some other tag (e.g. None, or a hard-coded value) "
					 "breaks every consumer that uses the player tag as the per-character "
					 "identity (GAS attribute initter, character-selection UI, skin applier)."),
				RegisteredTag, FBmrPlayerTag::Default);
		}

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));

#else
	AddWarning(TEXT("PIE-based GetPlayerTag test skipped: requires WITH_EDITOR."));
#endif

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — PIE: ApplyIsABot flips ASC replication mode (B4).
//
// B4 DIRECT: bots and humans need different EGameplayEffectReplicationMode on
// the player's ASC — bots use Minimal (the server doesn't need to replicate
// every GE to the bot's "owner" since there is no remote client driving it),
// humans use Mixed (so the controlling client gets the full GE stream while
// other clients only get attribute/tag updates).
//
// Discriminator: a fresh ABmrPlayerState has IsABot()==false by default;
// ApplyIsABot() must therefore drive ASC->ReplicationMode to Mixed. A stub
// ApplyIsABot (the start/ body) does nothing and the mode stays at whatever
// the constructor set (or the engine default).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_PIE_ApplyIsABotSwitchesAscReplicationMode,
	"Bomber.PlayerState.PIE_ApplyIsABotSwitchesAscReplicationMode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_PIE_ApplyIsABotSwitchesAscReplicationMode::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	using namespace PlayerStateTests;
	ResetRunState();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(MainMapPath)));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	{
		const double Deadline = FPlatformTime::Seconds() + MapReadyTimeoutSeconds;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Deadline]() -> bool
		{
			UWorld* World = GetServerPIEWorld();
			if (World && World->HasBegunPlay())
			{
				GRunState.bWorldReady = true;
				return true;
			}
			return FPlatformTime::Seconds() >= Deadline;
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetServerPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}
		if (!TestTrue(TEXT("PIE world must have begun play"), GRunState.bWorldReady))
		{
			return true;
		}

		ABmrPlayerState* PS = SpawnPlayerState(World);
		if (!TestNotNull(TEXT("ABmrPlayerState must be spawnable in PIE"), PS))
		{
			return true;
		}

		UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
		if (!TestNotNull(
				TEXT("PlayerState's AbilitySystemComponent must be constructed in the constructor "
					 "(CreateDefaultSubobject). Without it, ApplyIsABot has nothing to mutate."),
				ASC))
		{
			return true;
		}

		// Default IsABot()==false for a freshly-spawned PlayerState — ApplyIsABot
		// must therefore drive the human (Mixed) branch.
		if (!TestFalse(
				TEXT("Test fixture: fresh PlayerState IsABot() must default to false (so we "
					 "exercise the Mixed branch first)."),
				PS->IsABot()))
		{
			return true;
		}

		// Drive ApplyIsABot via UFunction reflection (BlueprintProtected, so
		// not directly callable from outside, but reflection-discoverable).
		UFunction* ApplyIsABotFunc = PS->FindFunction(TEXT("ApplyIsABot"));
		if (!TestNotNull(
				TEXT("B4: ApplyIsABot must be declared with UFUNCTION() — the spec requires it as "
					 "the central apply path for the human-vs-bot replication-mode decision."),
				ApplyIsABotFunc))
		{
			return true;
		}
		PS->ProcessEvent(ApplyIsABotFunc, nullptr);

		TestEqual(
			TEXT("B4: ApplyIsABot on a NON-bot PlayerState must set the ASC's ReplicationMode to "
				 "Mixed. Humans need Mixed so the controlling client gets the full GE stream "
				 "(prediction-friendly) while non-owning clients only get attribute/tag updates. "
				 "A stub ApplyIsABot leaves the mode at whatever the constructor set; the "
				 "discriminator is the post-call switch to Mixed."),
			static_cast<int32>(ASC->ReplicationMode),
			static_cast<int32>(EGameplayEffectReplicationMode::Mixed));

		// Now flip IsABot via the inherited bIsABot bitfield UPROPERTY and
		// re-call ApplyIsABot. The mode must switch to Minimal.
		FBoolProperty* IsBotProp = CastField<FBoolProperty>(
			ABmrPlayerState::StaticClass()->FindPropertyByName(TEXT("bIsABot")));
		if (TestNotNull(
				TEXT("APlayerState::bIsABot must be a UPROPERTY (inherited; declared in engine)."),
				IsBotProp))
		{
			IsBotProp->SetPropertyValue_InContainer(PS, true);
			if (!TestTrue(TEXT("Test fixture: bIsABot=true write must take effect"), PS->IsABot()))
			{
				return true;
			}

			PS->ProcessEvent(ApplyIsABotFunc, nullptr);

			TestEqual(
				TEXT("B4: ApplyIsABot on a BOT PlayerState must set the ASC's ReplicationMode to "
					 "Minimal. Bots have no controlling client, so the full Mixed GE stream is "
					 "wasted bandwidth. A re-impl that hard-codes the mode (or only switches one "
					 "direction) leaves bots paying for human-style replication."),
				static_cast<int32>(ASC->ReplicationMode),
				static_cast<int32>(EGameplayEffectReplicationMode::Minimal));
		}

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() -> bool
	{
		ResetRunState();
		return true;
	}));

#else
	AddWarning(TEXT("PIE-based ApplyIsABot test skipped: requires WITH_EDITOR."));
#endif

	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Static: GetLifetimeReplicatedProps registers the required props
//          and the header declares them with the right specifiers (B21).
//
// The spec lists the replicated identity fields: ChosenMeshData,
// EndGameState, bIsPlayerDead, OpponentsKilledNum. Each must (a) carry
// CPF_Net (the Replicated/ReplicatedUsing specifier) and (b) be registered
// inside GetLifetimeReplicatedProps via a DOREPLIFETIME-prefixed macro.
// A body that only calls Super registers none of these, and the chosen
// character / dead state / round stats never cross the wire.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_GetLifetimeReplicatedPropsRegistersRequiredProperties,
	"Bomber.PlayerState.GetLifetimeReplicatedPropsRegistersRequiredProperties",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_GetLifetimeReplicatedPropsRegistersRequiredProperties::RunTest(const FString& Parameters)
{
	using namespace PlayerStateTests;

	const UClass* Class = ABmrPlayerState::StaticClass();
	if (!TestNotNull(TEXT("ABmrPlayerState class must exist"), Class))
	{
		return false;
	}

	// Header-side: each of the four replicated identity fields must declare
	// CPF_Net so it participates in replication at all.
	const TCHAR* const RequiredReplicatedNames[] = {
		TEXT("ChosenMeshData"),
		TEXT("EndGameState"),
		TEXT("bIsPlayerDead"),
		TEXT("OpponentsKilledNum"),
	};

	for (const TCHAR* PropName : RequiredReplicatedNames)
	{
		const FProperty* Prop = Class->FindPropertyByName(FName(PropName));
		if (!TestNotNull(
				*FString::Printf(
					TEXT("B21: ABmrPlayerState must declare %s as a UPROPERTY. The spec lists "
						 "this as one of the four replicated identity fields."),
					PropName),
				Prop))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(
				TEXT("B21: %s must carry the Replicated/ReplicatedUsing UPROPERTY specifier "
					 "(CPF_Net). Without it, GetLifetimeReplicatedProps cannot register the field "
					 "and the value never crosses the wire."),
				PropName),
			(Prop->PropertyFlags & CPF_Net) != 0);
	}

	// .cpp side: GetLifetimeReplicatedProps must reference each of the four
	// fields by name AND call a DOREPLIFETIME-prefixed macro (per-prop
	// registration). A body that only calls Super registers nothing.
	const FString Source = LoadProjectFile(this, PlayerStateRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body =
		ExtractMemberBody(Source, TEXT("ABmrPlayerState::GetLifetimeReplicatedProps"));
	if (!TestTrue(
			TEXT("B21: BmrPlayerState.cpp must define GetLifetimeReplicatedProps with a body. "
				 "The start/ stub provides a Super-only body; the per-property registration is "
				 "what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	TestTrue(
		TEXT("B21: GetLifetimeReplicatedProps body must call a DOREPLIFETIME-prefixed macro "
			 "(DOREPLIFETIME, DOREPLIFETIME_WITH_PARAMS_FAST, etc.) — these are the only "
			 "registration mechanisms the engine inspects for per-property replication. A body "
			 "that only calls Super registers none of ABmrPlayerState's own fields."),
		CodeOnly.Contains(TEXT("DOREPLIFETIME"), ESearchCase::CaseSensitive));

	for (const TCHAR* PropName : RequiredReplicatedNames)
	{
		TestTrue(
			*FString::Printf(
				TEXT("B21: GetLifetimeReplicatedProps body must mention %s — without this "
					 "registration, the field never replicates and the corresponding side of "
					 "the spec (chosen character / end-game tabulation / dead state / round "
					 "stats) silently breaks on remote clients."),
				PropName),
			CodeOnly.Contains(PropName, ESearchCase::CaseSensitive));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Source: ApplyDefaultAttributes uses the engine's FAttributeSetInitter,
//          NOT per-attribute SetNumericAttributeBase writes (B1 failure mode).
//
// The spec's failure mode list, item 1: "Initializing attributes per-attribute
// manually — bypasses the engine initter's tag cascade." The initter's job is
// to run InitAttributeSetDefaults across the registered Curve-Table-driven
// defaults so derived attributes (e.g. MaxHealth → Health) cascade correctly.
// A manual SetNumericAttributeBase per attribute bypasses the cascade and
// leaves derived attributes uninitialised — they hold their UPROPERTY default
// (0), which then trips the death gate on the first PreAttributeChange.
//
// The canonical body:
//   const UAbilitySystemGlobals* AbilityGlobals = ...;
//   const FAttributeSetInitter* AttributeSetInitter = AbilityGlobals
//       ? AbilityGlobals->GetAttributeSetInitter() : nullptr;
//   AttributeSetInitter->InitAttributeSetDefaults(AbilitySystemComponent,
//       TEXT("Default"), 1, true);
//
// Runtime observation of this is impractical: the registered curve-table
// rows are populated by the GAS Globals startup pipeline, and reading the
// post-init attribute values relies on data tables that may not be loaded
// at the time the test runs. Source inspection is the practical
// discriminator for the failure mode.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_ApplyDefaultAttributesUsesEngineInitter,
	"Bomber.PlayerState.ApplyDefaultAttributesUsesEngineInitter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_ApplyDefaultAttributesUsesEngineInitter::RunTest(const FString& Parameters)
{
	using namespace PlayerStateTests;

	const FString Source = LoadProjectFile(this, PlayerStateRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerState::ApplyDefaultAttributes"));
	if (!TestTrue(
			TEXT("B1: BmrPlayerState.cpp must define ApplyDefaultAttributes with a body — the "
				 "start/ stub leaves the body empty; the GAS-initter call is what was stripped."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// Must call into the engine's attribute-set initter path. Both names are
	// acceptable: GetAttributeSetInitter (the canonical accessor on
	// UAbilitySystemGlobals) and InitAttributeSetDefaults (the call the
	// initter exposes). Either reference confirms the initter pathway.
	const bool bUsesInitter =
		CodeOnly.Contains(TEXT("InitAttributeSetDefaults"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("GetAttributeSetInitter"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B1: ApplyDefaultAttributes must initialise via the engine's FAttributeSetInitter. "
			 "The canonical body resolves UAbilitySystemGlobals::GetAttributeSetInitter() then "
			 "calls InitAttributeSetDefaults(ASC, \"Default\", 1, true). The initter walks the "
			 "Curve-Table-driven attribute defaults and respects the tag cascade — derived "
			 "attributes like Health (cascading from MaxHealth) only get the right initial "
			 "value through this path."),
		bUsesInitter);

	// Must NOT do per-attribute SetNumericAttributeBase / ApplyModToAttribute
	// writes — those bypass the initter's cascade and only paper over the
	// failure mode visually.
	const bool bUsesPerAttributeManual =
		CodeOnly.Contains(TEXT("SetNumericAttributeBase"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("ApplyModToAttribute"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B1: ApplyDefaultAttributes must NOT use per-attribute SetNumericAttributeBase / "
			 "ApplyModToAttribute writes. The spec calls this the named failure mode: "
			 "\"Initializing attributes per-attribute manually — bypasses the engine initter's "
			 "tag cascade.\" Derived attributes that depend on the cascade (e.g. Health from "
			 "MaxHealth) silently keep their UPROPERTY default and trip the death gate on the "
			 "first PreAttributeChange."),
		bUsesPerAttributeManual);

	return true;
}

// ---------------------------------------------------------------------------
// Test 7 — Source: ApplyIsPlayerDead defers UpdateEndGameState to next tick
//          (B2 failure mode).
//
// The spec's failure mode list, item 2: "Running end-game tabulation
// synchronously on death — same-frame multi-death race condition." When two
// players die in the same frame (a bomb wipes out a chain), each
// ApplyIsPlayerDead that runs UpdateEndGameState synchronously sees the
// OTHER player's pre-death state and computes a Lose instead of the correct
// Draw. The canonical body defers tabulation:
//
//   if (HasAuthority())
//   {
//       GetWorldTimerManager().SetTimerForNextTick(...
//           OnPostPlayerDead(); // which calls UpdateEndGameState
//       ...);
//   }
//
// Runtime observation of "same-frame multi-death race" requires multiple
// players dying simultaneously inside a hydrated BmrGameState — well beyond
// the scope of a unit-style PIE test. Source inspection is the practical
// discriminator for the failure mode: the body must defer, not call
// synchronously.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_ApplyIsPlayerDeadDefersUpdateEndGameState,
	"Bomber.PlayerState.ApplyIsPlayerDeadDefersUpdateEndGameState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_ApplyIsPlayerDeadDefersUpdateEndGameState::RunTest(const FString& Parameters)
{
	using namespace PlayerStateTests;

	const FString Source = LoadProjectFile(this, PlayerStateRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString ApplyBody = ExtractMemberBody(Source, TEXT("ABmrPlayerState::ApplyIsPlayerDead"));
	if (!TestTrue(
			TEXT("B2 failure mode: BmrPlayerState.cpp must define ApplyIsPlayerDead with a body — "
				 "the start/ stub leaves it empty; the deferral logic is what was stripped."),
			!ApplyBody.IsEmpty()))
	{
		return false;
	}

	const FString ApplyCode = StripComments(ApplyBody);

	// ApplyIsPlayerDead's body must defer via SetTimerForNextTick (the
	// canonical mechanism) OR via OnPostPlayerDead (the deferred callback
	// invoked from the timer). A body that calls UpdateEndGameState directly
	// is the named failure mode.
	const bool bDefers =
		ApplyCode.Contains(TEXT("SetTimerForNextTick"), ESearchCase::CaseSensitive)
		|| ApplyCode.Contains(TEXT("OnPostPlayerDead"), ESearchCase::CaseSensitive);
	TestTrue(
		TEXT("B2 failure mode: ApplyIsPlayerDead must defer end-game tabulation — either via "
			 "GetWorldTimerManager().SetTimerForNextTick(...) (the canonical mechanism) or by "
			 "scheduling OnPostPlayerDead. Calling UpdateEndGameState directly from "
			 "ApplyIsPlayerDead is the spec's named race-condition failure: multiple players "
			 "dying in the same frame see each other's pre-death state and tabulate Lose where "
			 "the correct result is Draw."),
		bDefers);

	// A direct UpdateEndGameState call inside ApplyIsPlayerDead is the
	// failure mode. The canonical body never references UpdateEndGameState
	// directly — it goes through OnPostPlayerDead (which runs on the next
	// tick).
	TestFalse(
		TEXT("B2 failure mode: ApplyIsPlayerDead must NOT call UpdateEndGameState directly. The "
			 "spec is explicit: \"Running end-game tabulation synchronously on death — same-frame "
			 "multi-death race condition.\" The canonical path is via OnPostPlayerDead on the "
			 "next tick, never from inside ApplyIsPlayerDead itself."),
		ApplyCode.Contains(TEXT("UpdateEndGameState"), ESearchCase::CaseSensitive));

	// Also: the dead-state broadcast must fire from ApplyIsPlayerDead — that's
	// part of the dead-state behaviour group (clients react to dead-state
	// changes via the OnPlayerDeadChanged delegate). A body that only schedules
	// the timer but never broadcasts leaves listeners stranded.
	TestTrue(
		TEXT("ApplyIsPlayerDead must broadcast OnPlayerDeadChanged. The canonical body broadcasts "
			 "the new bIsPlayerDead state alongside the deferred tabulation. Without the "
			 "broadcast, listeners (death VFX, scoreboard) miss the transition."),
		ApplyCode.Contains(TEXT("OnPlayerDeadChanged"), ESearchCase::CaseSensitive));

	return true;
}

// ---------------------------------------------------------------------------
// Test 8 — Source: OnDeactivated is suppressed — must NOT chain Super
//          (B3 failure mode).
//
// The spec's failure mode list, item 3: "Destroying the player state on
// disconnect — breaks reconnect and pool reuse." APlayerState::OnDeactivated
// is what the engine calls when a player leaves; the base implementation
// marks the player state for destruction. ABmrPlayerState is reused across
// rounds (and across human-to-bot conversion on disconnect), so the override
// MUST NOT chain Super.
//
// The canonical body is intentionally a no-op (just an early return), which
// the test recognises by the absence of any Super::OnDeactivated() reference.
//
// Runtime observation is impractical: OnDeactivated is invoked from
// engine-internal paths (player-leave / session-teardown) that don't have
// reflection entry points (it's not a UFUNCTION on APlayerState). Source
// inspection is the only practical discriminator.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBmrPlayerState_OnDeactivatedDoesNotChainSuper,
	"Bomber.PlayerState.OnDeactivatedDoesNotChainSuper",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBmrPlayerState_OnDeactivatedDoesNotChainSuper::RunTest(const FString& Parameters)
{
	using namespace PlayerStateTests;

	const FString Source = LoadProjectFile(this, PlayerStateRelPath);
	if (Source.IsEmpty())
	{
		return false;
	}

	const FString Body = ExtractMemberBody(Source, TEXT("ABmrPlayerState::OnDeactivated"));
	if (!TestTrue(
			TEXT("B3 failure mode: BmrPlayerState.cpp must define an OnDeactivated override body — "
				 "the header declares it as an override. A re-impl that deletes the override "
				 "entirely lets APlayerState::OnDeactivated run unmodified and destroys the "
				 "player state."),
			!Body.IsEmpty()))
	{
		return false;
	}

	const FString CodeOnly = StripComments(Body);

	// The contract is precisely "no Super::OnDeactivated()". The body may
	// still do other cleanup (clear timers, notify listeners), but the
	// Super call must not be present — that's the call that marks the player
	// state for destruction.
	const bool bCallsSuper =
		CodeOnly.Contains(TEXT("Super::OnDeactivated"), ESearchCase::CaseSensitive)
		|| CodeOnly.Contains(TEXT("APlayerState::OnDeactivated"), ESearchCase::CaseSensitive);
	TestFalse(
		TEXT("B3 failure mode: OnDeactivated must NOT call Super::OnDeactivated() (or the "
			 "equivalent APlayerState::OnDeactivated()). The spec names this as the "
			 "destruction-on-disconnect failure: APlayerState::OnDeactivated tears the player "
			 "state down, but ABmrPlayerState must survive disconnect because the same instance "
			 "is reused for the reconnecting human or for the bot that takes over their slot. "
			 "The pool manager assumes the player-state instance persists across the leave/"
			 "rejoin boundary; chaining Super breaks that assumption."),
		bCallsSuper);

	return true;
}
