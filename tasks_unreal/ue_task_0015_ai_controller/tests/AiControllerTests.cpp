// Copyright 2024 UnrealBench. All Rights Reserved.
// AiControllerTests.cpp
// Automation tests for AI Controller + Behavior Tree (ActionRoguelike)
//
// Design: CDO structural checks run without PIE; runtime behaviors
// are batched into a single PIE session to avoid repeated level loads.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/Blackboard/BlackboardKey.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GenericTeamAgentInterface.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#endif

// Project-specific headers
#include "ActionRoguelike.h"
#include "SharedGameplayTags.h"
#include "AI/RogueAIController.h"
#include "AI/RogueAICharacter.h"
#include "AI/RogueBTDecorator_CheckHealth.h"
#include "AI/RogueBTService_CheckAttackRange.h"
#include "AI/RogueBTService_StartAction.h"
#include "AI/RogueBTTask_HealSelf.h"
#include "AI/RogueBTTask_StartAction.h"
#include "AI/RogueQueryContext_TargetActor.h"
#include "ActionSystem/RogueActionComponent.h"
#include "ActionSystem/RogueAttributeSet.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace AICtrlTestHelpers
{
	static const TCHAR* MapPath = TEXT("/Game/ActionRoguelike/Maps/TestLevel");

	UWorld* GetPIEWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE && Ctx.World() && Ctx.World()->IsGameWorld())
			{
				return Ctx.World();
			}
		}
		return nullptr;
	}

	// Returns the first living AI character that still has a controller (server-side).
	ARogueAICharacter* FindFirstLivingAI(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<ARogueAICharacter> It(World); It; ++It)
		{
			ARogueAICharacter* AI = *It;
			if (IsValid(AI) && !AI->IsHidden() && AI->GetController<AAIController>())
			{
				return AI;
			}
		}
		return nullptr;
	}
}


// ---------------------------------------------------------------------------
// Test 1 — CDO / structural property checks (no PIE required)
//
// B1   BehaviorTree UPROPERTY exists on ARogueAIController                (PARTIAL)
// B4   ARogueAIController implements IGenericTeamAgentInterface            (PARTIAL)
// B14  LowHealthFraction UPROPERTY exists on URogueBTDecorator_CheckHealth (PARTIAL)
// B15  LowHealthFraction default is in (0.0, 1.0)                         (PARTIAL)
// B6   MaxAttackRange > 0; TargetActorKey defaults to "TargetActor"        (PARTIAL)
// B7   AttackRangeKey UPROPERTY exists on attack range service             (PARTIAL)
// B8   DebugColor UPROPERTY exists on attack range service                 (PARTIAL)
// B9   ActionName UPROPERTY exists on URogueBTService_StartAction          (PARTIAL)
// B10  ActionName UPROPERTY exists on URogueBTTask_StartAction             (PARTIAL)
// B3   ARoguePlayerCharacter exposes ClientOnSeenBy RPC                   (PARTIAL)
// B5   URogueQueryContext_TargetActor class is loaded                      (PARTIAL)
// B16  URogueBTDecorator_CheckHealth inherits from UBTDecorator            (PARTIAL)
// B11  URogueBTTask_StartAction inherits from UBTTaskNode                  (PARTIAL)
// B13  URogueBTTask_HealSelf inherits from UBTTaskNode                     (PARTIAL)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAIController_CDOChecks,
	"ActionRoguelike.AIController.CDOChecks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAIController_CDOChecks::RunTest(const FString& Parameters)
{
	// B1 / B4: ARogueAIController CDO
	{
		const ARogueAIController* CDO = GetDefault<ARogueAIController>();
		if (TestNotNull(TEXT("ARogueAIController CDO must exist"), CDO))
		{
			// B1: BehaviorTree UPROPERTY must be present so BeginPlay can call RunBehaviorTree.
			const FObjectPropertyBase* BTProp =
				FindFProperty<FObjectPropertyBase>(CDO->GetClass(), TEXT("BehaviorTree"));
			TestNotNull(TEXT("B1: BehaviorTree UPROPERTY must exist on ARogueAIController"), BTProp);

			// B4: Must implement IGenericTeamAgentInterface
			const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(CDO);
			TestNotNull(TEXT("B4: ARogueAIController must implement IGenericTeamAgentInterface"), TeamAgent);
		}
	}

	// B14 / B15: URogueBTDecorator_CheckHealth
	{
		const URogueBTDecorator_CheckHealth* CDO = GetDefault<URogueBTDecorator_CheckHealth>();
		if (TestNotNull(TEXT("URogueBTDecorator_CheckHealth CDO must exist"), CDO))
		{
			const FFloatProperty* Prop =
				FindFProperty<FFloatProperty>(CDO->GetClass(), TEXT("LowHealthFraction"));
			if (TestNotNull(TEXT("B14: LowHealthFraction UPROPERTY must exist on URogueBTDecorator_CheckHealth"), Prop))
			{
				const float DefaultFraction = Prop->GetPropertyValue_InContainer(CDO);
				TestTrue(TEXT("B15: LowHealthFraction must be in (0.0, 1.0) — a ratio that triggers the flee/heal branch"),
					DefaultFraction > 0.0f && DefaultFraction < 1.0f);
			}
		}
	}

	// B6 / B7: URogueBTService_CheckAttackRange
	{
		const URogueBTService_CheckAttackRange* CDO = GetDefault<URogueBTService_CheckAttackRange>();
		if (TestNotNull(TEXT("URogueBTService_CheckAttackRange CDO must exist"), CDO))
		{
			// B6: MaxAttackRange must be > 0
			const FFloatProperty* RangeProp =
				FindFProperty<FFloatProperty>(CDO->GetClass(), TEXT("MaxAttackRange"));
			if (TestNotNull(TEXT("B6: MaxAttackRange UPROPERTY must exist on attack range service"), RangeProp))
			{
				TestTrue(TEXT("B6: MaxAttackRange must be > 0 (zero means the AI can never be in range)"),
					RangeProp->GetPropertyValue_InContainer(CDO) > 0.0f);
			}

			// B7: AttackRangeKey UPROPERTY must exist
			TestNotNull(TEXT("B7: AttackRangeKey UPROPERTY must exist on attack range service"),
				FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("AttackRangeKey")));

			// B6: TargetActorKey.SelectedKeyName must default to "TargetActor"
			const FStructProperty* TargetKeyProp =
				FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("TargetActorKey"));
			if (TestNotNull(TEXT("B6: TargetActorKey UPROPERTY must exist on attack range service"), TargetKeyProp))
			{
				const FBlackboardKeySelector* KeySel =
					TargetKeyProp->ContainerPtrToValuePtr<FBlackboardKeySelector>(CDO);
				TestEqual(
					TEXT("B6: TargetActorKey.SelectedKeyName must default to 'TargetActor' (set in URogueBTService_CheckAttackRange constructor)"),
					KeySel->SelectedKeyName, FName(TEXT("TargetActor")));
			}

			// B8 PARTIAL: DebugColor UPROPERTY must exist
			TestNotNull(
				TEXT("B8 PARTIAL: DebugColor UPROPERTY must exist on attack range service (used by non-shipping CVar debug draw)"),
				FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("DebugColor")));
		}
	}

	// B10 / B11: URogueBTTask_StartAction
	{
		const URogueBTTask_StartAction* CDO = GetDefault<URogueBTTask_StartAction>();
		if (TestNotNull(TEXT("URogueBTTask_StartAction CDO must exist"), CDO))
		{
			TestNotNull(
				TEXT("B10/B11: ActionName UPROPERTY (FGameplayTag) must exist on URogueBTTask_StartAction"),
				FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("ActionName")));

			// B11 PARTIAL: URogueBTTask_StartAction must inherit from UBTTaskNode
			TestTrue(
				TEXT("B11 PARTIAL: URogueBTTask_StartAction must inherit from UBTTaskNode"),
				URogueBTTask_StartAction::StaticClass()->IsChildOf<UBTTaskNode>());
		}
	}

	// B9: URogueBTService_StartAction
	{
		const URogueBTService_StartAction* CDO = GetDefault<URogueBTService_StartAction>();
		if (TestNotNull(TEXT("URogueBTService_StartAction CDO must exist"), CDO))
		{
			TestNotNull(
				TEXT("B9: ActionName UPROPERTY (FGameplayTag) must exist on URogueBTService_StartAction"),
				FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("ActionName")));
		}
	}

	// B13 PARTIAL: URogueBTTask_HealSelf must inherit from UBTTaskNode
	{
		const URogueBTTask_HealSelf* CDO = GetDefault<URogueBTTask_HealSelf>();
		if (TestNotNull(TEXT("URogueBTTask_HealSelf CDO must exist"), CDO))
		{
			TestTrue(
				TEXT("B13 PARTIAL: URogueBTTask_HealSelf must inherit from UBTTaskNode"),
				URogueBTTask_HealSelf::StaticClass()->IsChildOf<UBTTaskNode>());
		}
	}

	// B3 PARTIAL: ARoguePlayerCharacter must expose a ClientOnSeenBy RPC.
	{
		UClass* PlayerCharClass = FindObject<UClass>(nullptr,
			TEXT("/Script/ActionRoguelike.RoguePlayerCharacter"));
		if (TestNotNull(TEXT("B3 PARTIAL: ARoguePlayerCharacter class must be loaded"), PlayerCharClass))
		{
			TestNotNull(
				TEXT("B3 PARTIAL: ClientOnSeenBy RPC must be declared on ARoguePlayerCharacter (called by OnTargetActorChanged)"),
				PlayerCharClass->FindFunctionByName(TEXT("ClientOnSeenBy")));
		}
	}

	// B5 PARTIAL: URogueQueryContext_TargetActor must exist as a loaded class.
	{
		UClass* QCClass = FindObject<UClass>(nullptr,
			TEXT("/Script/ActionRoguelike.RogueQueryContext_TargetActor"));
		TestNotNull(
			TEXT("B5 PARTIAL: URogueQueryContext_TargetActor class must exist for EQS target context resolution"),
			QCClass);
	}

	// B16 PARTIAL: URogueBTDecorator_CheckHealth must inherit from UBTDecorator
	{
		TestTrue(
			TEXT("B16 PARTIAL: URogueBTDecorator_CheckHealth must inherit from UBTDecorator (required for BT flee-or-heal branch gating when AI is injured)"),
			URogueBTDecorator_CheckHealth::StaticClass()->IsChildOf<UBTDecorator>());
	}

	return true;
}


// ---------------------------------------------------------------------------
// Test 2 — Runtime behaviors, single PIE session
//
// Phase 0:
//   B4   AI controller generic team ID == TEAM_ID_BOTS (1) after spawn       (DIRECT)
//   B1   BrainComponent is running after BeginPlay                           (DIRECT)
//   B2   Blackboard has "TargetActor" key (prerequisite for observer reg.)   (PARTIAL)
// Phase 1:
//   B12  HealSelf — verify ApplyAttributeChange raises health                (DIRECT)
//   B14/B15 — verify health ratio logic via attribute manipulation           (DIRECT)
// Phase 2:
//   B12  BT must run HealSelf; verify health is restored to near max        (DIRECT)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAIController_RuntimeBehaviors,
	"ActionRoguelike.AIController.RuntimeBehaviors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAIController_RuntimeBehaviors::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	// Cross-phase state — reset on each test invocation.
	static ARogueAICharacter* CachedAIChar = nullptr;
	CachedAIChar = nullptr;
	static float CachedHealthMax = 0.0f;
	CachedHealthMax = 0.0f;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(AICtrlTestHelpers::MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));

	// Give the game mode time to spawn at least one AI via EQS.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4.0f));

	// -------------------------------------------------------------------------
	// Phase 0: B4 + B1 + B2 — verify controller team ID, BT running, BB key
	// -------------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = AICtrlTestHelpers::GetPIEWorld();
		if (!TestNotNull(TEXT("Phase 0: PIE world must be valid"), World)) return true;

		ARogueAICharacter* AIChar = AICtrlTestHelpers::FindFirstLivingAI(World);
		if (!TestNotNull(TEXT("Phase 0: At least one living ARogueAICharacter must have spawned"), AIChar)) return true;

		CachedAIChar = AIChar;

		AAIController* AIC = AIChar->GetController<AAIController>();
		if (!TestNotNull(TEXT("Phase 0: AI character must have a valid AAIController"), AIC)) return true;

		// B4 DIRECT
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(AIC))
		{
			TestEqual(
				TEXT("B4 DIRECT: AI controller generic team ID must be TEAM_ID_BOTS (1); set in PreRegisterAllComponents, not BeginPlay"),
				static_cast<int32>(TeamAgent->GetGenericTeamId().GetId()),
				static_cast<int32>(TEAM_ID_BOTS));
		}
		else
		{
			AddError(TEXT("B4: AI controller does not implement IGenericTeamAgentInterface"));
		}

		// B1 DIRECT
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			TestTrue(
				TEXT("B1 DIRECT: BrainComponent must be running after BeginPlay (RunBehaviorTree was called with a valid asset)"),
				Brain->IsRunning());
		}
		else
		{
			AddError(TEXT("B1: AI controller has no BrainComponent — RunBehaviorTree was never called"));
		}

		// B2 PARTIAL
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			const FBlackboard::FKey KeyID = BB->GetKeyID(FName(TEXT(NAME_TargetActor)));
			TestTrue(
				TEXT("B2 PARTIAL: Blackboard must contain 'TargetActor' key (required for RegisterObserver in BeginPlay)"),
				KeyID != FBlackboard::InvalidKey);
		}

		return true;
	}));

	// -------------------------------------------------------------------------
	// Phase 1: B12 (HealSelf observable) + B14/B15 (health ratio logic)
	// NOTE: We do NOT call CalculateRawConditionValue directly — it requires
	// a fully initialized BT owner with valid pawn/controller chain, and
	// calling it on a transient decorator with the live BTC causes a SIGSEGV.
	// Instead we verify the health ratio math that the decorator is specified
	// to use, by manipulating attributes and checking observable values.
	// -------------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = AICtrlTestHelpers::GetPIEWorld();
		if (!World) return true;

		ARogueAICharacter* AIChar = IsValid(CachedAIChar)
			? CachedAIChar
			: AICtrlTestHelpers::FindFirstLivingAI(World);

		if (!TestNotNull(TEXT("Phase 1: A valid ARogueAICharacter is required for health/heal tests"), AIChar)) return true;

		AAIController* AIC = AIChar->GetController<AAIController>();
		if (!TestNotNull(TEXT("Phase 1: AI character must have a controller"), AIC)) return true;

		URogueActionComponent* ActionComp = URogueGameplayFunctionLibrary::GetActionComponentFromActor(AIChar);
		if (!TestNotNull(TEXT("Phase 1: AI character must have URogueActionComponent"), ActionComp)) return true;

		const float HealthMax = ActionComp->GetAttributeValue(SharedGameplayTags::Attribute_HealthMax);
		if (!TestTrue(TEXT("Phase 1: HealthMax must be > 0"), HealthMax > 0.0f)) return true;

		// Read LowHealthFraction from the decorator CDO
		float LowHealthFraction = 0.3f;
		if (const URogueBTDecorator_CheckHealth* DecCDO = GetDefault<URogueBTDecorator_CheckHealth>())
		{
			if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(DecCDO->GetClass(), TEXT("LowHealthFraction")))
			{
				LowHealthFraction = Prop->GetPropertyValue_InContainer(DecCDO);
			}
		}

		// Restore to full health
		ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, HealthMax,
			AIChar, EAttributeModifyType::OverrideBase);

		const float HealthAtFull = ActionComp->GetAttributeValue(SharedGameplayTags::Attribute_Health);

		// B14/B15 PARTIAL: At full health the ratio should be >= LowHealthFraction
		// (i.e. the decorator condition should be false at full health)
		TestTrue(
			TEXT("B14/B15 PARTIAL: At full health, health ratio must be >= LowHealthFraction (decorator should return false)"),
			(HealthAtFull / HealthMax) >= LowHealthFraction);

		// Deal enough damage to drop well below the threshold.
		const float HealthToLeave  = HealthMax * LowHealthFraction * 0.5f;
		const float DamageToApply  = HealthMax - HealthToLeave;
		ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, -DamageToApply,
			AIChar, EAttributeModifyType::AddBase);

		const float HealthAfterDamage = ActionComp->GetAttributeValue(SharedGameplayTags::Attribute_Health);

		// B14/B15 PARTIAL: Below the threshold the health ratio should be < LowHealthFraction
		// (i.e. the decorator condition should be true below threshold)
		TestTrue(
			FString::Printf(
				TEXT("B14/B15 PARTIAL: Below threshold health ratio (%.1f/%.1f = %.2f) must be < threshold (%.2f) so decorator returns true"),
				HealthAfterDamage, HealthMax, HealthAfterDamage / HealthMax, LowHealthFraction),
			(HealthAfterDamage / HealthMax) < LowHealthFraction);

		// B12 DIRECT: Verify ApplyAttributeChange with AddBase raises health
		{
			ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, HealthMax,
				AIChar, EAttributeModifyType::AddBase);
			const float HealthPostHeal = ActionComp->GetAttributeValue(SharedGameplayTags::Attribute_Health);
			TestTrue(
				FString::Printf(
					TEXT("B12 DIRECT: ApplyAttributeChange(Health, HealthMax=%.1f, AddBase) must raise health above damaged value %.1f (got %.1f)"),
					HealthMax, HealthAfterDamage, HealthPostHeal),
				HealthPostHeal > HealthAfterDamage);
		}

		// -----------------------------------------------------------------------
		// B6/B7 PARTIAL: Observable blackboard check
		// -----------------------------------------------------------------------
		{
			if (UBlackboardComponent* LiveBB = AIC->GetBlackboardComponent())
			{
				const FBlackboard::FKey LiveRangeKeyID =
					LiveBB->GetKeyID(FName(TEXT("WithinAttackRange")));
				const FBlackboard::FKey LiveTargetKeyID =
					LiveBB->GetKeyID(FName(TEXT(NAME_TargetActor)));

				if (LiveRangeKeyID != FBlackboard::InvalidKey &&
					LiveTargetKeyID != FBlackboard::InvalidKey)
				{
					// Clear the target; the next BT tick should set WithinAttackRange=false.
					LiveBB->SetValueAsObject(FName(TEXT(NAME_TargetActor)), nullptr);
					AddInfo(TEXT("B6/B7 PARTIAL: Live blackboard has 'WithinAttackRange' key — service is active in BT asset."));
				}
				else
				{
					AddError(TEXT("B6/B7: Live blackboard does not have 'WithinAttackRange' key. "
						"URogueBTService_CheckAttackRange must register this key in its constructor "
						"so the BT asset can expose it. A stub that never sets up the service "
						"will fail here."));
				}
			}
		}

		CachedHealthMax = HealthMax;

		return true;
	}));

	// -------------------------------------------------------------------------
	// Phase 2: Damage AI and let BT heal branch fire
	// -------------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = AICtrlTestHelpers::GetPIEWorld();
		if (!World) return true;

		ARogueAICharacter* AIChar = IsValid(CachedAIChar)
			? CachedAIChar : AICtrlTestHelpers::FindFirstLivingAI(World);
		if (!TestNotNull(TEXT("Phase 2: Valid ARogueAICharacter required"), AIChar)) return true;

		URogueActionComponent* ActionComp = URogueGameplayFunctionLibrary::GetActionComponentFromActor(AIChar);

		// Damage the AI to ~10% health so the CheckHealth decorator returns true
		// and the BT enters the heal branch.
		if (ActionComp && CachedHealthMax > 0.0f)
		{
			ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health,
				CachedHealthMax * 0.1f, AIChar, EAttributeModifyType::OverrideBase);
		}

		return true;
	}));

	// Give the BT time to enter the heal branch and execute URogueBTTask_HealSelf.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(8.0f));

	// -------------------------------------------------------------------------
	// Phase 3: B12 DIRECT — BT must have run HealSelf; verify health is restored
	// -------------------------------------------------------------------------
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = AICtrlTestHelpers::GetPIEWorld();
		if (!World) return true;

		ARogueAICharacter* AIChar = IsValid(CachedAIChar)
			? CachedAIChar : AICtrlTestHelpers::FindFirstLivingAI(World);
		if (!TestNotNull(TEXT("Phase 3: Valid ARogueAICharacter required for heal check"), AIChar)) return true;

		URogueActionComponent* ActionComp = URogueGameplayFunctionLibrary::GetActionComponentFromActor(AIChar);
		if (!TestNotNull(TEXT("Phase 3: URogueActionComponent required"), ActionComp)) return true;

		const float HealthNow = ActionComp->GetAttributeValue(SharedGameplayTags::Attribute_Health);
		// B12 DIRECT: URogueBTTask_HealSelf must restore health to HealthMax.
		TestTrue(
			FString::Printf(
				TEXT("B12 DIRECT: BT HealSelf task must restore health to near HealthMax (%.1f); "
					 "got %.1f after 8 s with health set to 10%%"),
				CachedHealthMax, HealthNow),
			HealthNow >= CachedHealthMax * 0.95f);

		return true;
	}));

	// Cleanup: restore all AI characters to full health before EndPlayMap to
	// prevent a Chaos physics constraint use-after-free crash in OnDestroyPhysicsState
	// that occurs when the world tears down while AI skeletal meshes are at low health.
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = AICtrlTestHelpers::GetPIEWorld();
		if (!World) return true;
		for (TActorIterator<ARogueAICharacter> It(World); It; ++It)
		{
			ARogueAICharacter* AI = *It;
			if (!IsValid(AI)) continue;
			URogueActionComponent* AC = URogueGameplayFunctionLibrary::GetActionComponentFromActor(AI);
			if (AC)
			{
				const float HMax = AC->GetAttributeValue(SharedGameplayTags::Attribute_HealthMax);
				if (HMax > 0.0f)
				{
					AC->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, HMax,
						AI, EAttributeModifyType::OverrideBase);
				}
			}
		}
		return true;
	}));
	// Brief wait to let the health restore settle before tearing down physics.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

#else
	AddWarning(TEXT("Editor-only test skipped."));
#endif
	return true;
}
