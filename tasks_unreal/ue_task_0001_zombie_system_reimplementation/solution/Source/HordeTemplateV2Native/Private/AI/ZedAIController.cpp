

#include "ZedAIController.h"
#include "AI/AICorePoint.h"
#include "AI/ZedPawn.h"
#include "HordeTemplateV2Native.h"
#include "Character/HordeBaseCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

/**
 *	Constructor
 *
 * @param
 * @return
 */
AZedAIController::AZedAIController()
{
	PCC = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception Component"));

	UAISenseConfig_Sight* sightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
	sightConfig->DetectionByAffiliation.bDetectEnemies = true;
	sightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	sightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	sightConfig->SightRadius = ZED_SIGHT_RADIUS;
	sightConfig->LoseSightRadius = ZED_LOSE_SIGHT_RADIUS;
	sightConfig->SetMaxAge(10.f);

	PCC->ConfigureSense(*sightConfig);
	PCC->SetDominantSense(sightConfig->GetSenseImplementation());
	PCC->OnTargetPerceptionUpdated.AddDynamic(this, &AZedAIController::EnemyInSight);
}


/**
 *	Tracks perceived enemies and starts a delayed clear when sight is lost.
 *
 * @param The Enemy Actor and AI Stimulus
 * @return void
 */
void AZedAIController::EnemyInSight(AActor* Actor, FAIStimulus Stimulus)
{
	AHordeBaseCharacter* Enemy = Cast<AHordeBaseCharacter>(Actor);
	if (!HasAuthority() || !Enemy)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && !Enemy->GetIsDead())
	{
		if (AZedPawn* Zed = Cast<AZedPawn>(GetPawn()))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[ZedSight] Acquired target=%s zed=%s distXY=%.1f dist3D=%.1f sightRadius=%.1f loseSightRadius=%.1f zedLoc=(%.1f, %.1f, %.1f) targetLoc=(%.1f, %.1f, %.1f)"),
				*GetNameSafe(Enemy),
				*GetNameSafe(Zed),
				FVector::Dist2D(Zed->GetActorLocation(), Enemy->GetActorLocation()),
				FVector::Distance(Zed->GetActorLocation(), Enemy->GetActorLocation()),
				static_cast<float>(ZED_SIGHT_RADIUS),
				static_cast<float>(ZED_LOSE_SIGHT_RADIUS),
				Zed->GetActorLocation().X, Zed->GetActorLocation().Y, Zed->GetActorLocation().Z,
				Enemy->GetActorLocation().X, Enemy->GetActorLocation().Y, Enemy->GetActorLocation().Z);
		}
		CurrentEnemy = Enemy;
		ActiveAction = EZedNativeAction::None;
		GetWorld()->GetTimerManager().ClearTimer(SightClearTimer);
		return;
	}

	if (CurrentEnemy == Enemy)
	{
		if (AZedPawn* Zed = Cast<AZedPawn>(GetPawn()))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[ZedSight] Lost target=%s zed=%s distXY=%.1f dist3D=%.1f zedLoc=(%.1f, %.1f, %.1f) targetLoc=(%.1f, %.1f, %.1f)"),
				*GetNameSafe(Enemy),
				*GetNameSafe(Zed),
				FVector::Dist2D(Zed->GetActorLocation(), Enemy->GetActorLocation()),
				FVector::Distance(Zed->GetActorLocation(), Enemy->GetActorLocation()),
				Zed->GetActorLocation().X, Zed->GetActorLocation().Y, Zed->GetActorLocation().Z,
				Enemy->GetActorLocation().X, Enemy->GetActorLocation().Y, Enemy->GetActorLocation().Z);
		}
		GetWorld()->GetTimerManager().SetTimer(
			SightClearTimer,
			this,
			&AZedAIController::ClearSight,
			FMath::FRandRange(ZED_LOSE_SIGHT_TIME_MIN, ZED_LOSE_SIGHT_TIME_MAX),
			false);
	}
}


/**
 *	Clears the current enemy target.
 *
 * @param
 * @return void
 */
void AZedAIController::ClearSight()
{
	ClearEnemy();
}


/**
 *	Begin Play -> Starts native AI decision loop on the server.
 *
 * @param
 * @return void
 */
void AZedAIController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(NativeDecisionTimer, this, &AZedAIController::TickNativeAI, 0.2f, true);
	}
}

void AZedAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AZedPawn* Zed = Cast<AZedPawn>(InPawn))
	{
		PatrolTag = Zed->PatrolTag;
	}
}

void AZedAIController::SetPlayerInRange(bool bInRange)
{
	bPlayerInRange = bInRange;
}

void AZedAIController::SetPatrolTag(FName NewPatrolTag)
{
	PatrolTag = NewPatrolTag;
}

void AZedAIController::NotifyZedDied()
{
	bZedDead = true;
	ClearEnemy();
	StopMovement();
	GetWorld()->GetTimerManager().ClearTimer(NativeDecisionTimer);
	GetWorld()->GetTimerManager().ClearTimer(SightClearTimer);
}

void AZedAIController::TickNativeAI()
{
	if (!HasAuthority() || bZedDead)
	{
		return;
	}

	AZedPawn* Zed = Cast<AZedPawn>(GetPawn());
	if (!Zed || Zed->GetIsDead())
	{
		NotifyZedDied();
		return;
	}

	if (CurrentEnemy && CurrentEnemy->GetIsDead())
	{
		UE_LOG(LogTemp, Display,
			TEXT("[ZedFallback] DeadEnemyCleared zed=%s enemy=%s patrolTag=%s action=%d moveStatus=%d zedLoc=(%.1f, %.1f, %.1f)"),
			*GetNameSafe(Zed),
			*GetNameSafe(CurrentEnemy),
			*PatrolTag.ToString(),
			static_cast<int32>(ActiveAction),
			static_cast<int32>(GetMoveStatus()),
			Zed->GetActorLocation().X, Zed->GetActorLocation().Y, Zed->GetActorLocation().Z);
		ClearEnemy();
		SetZedWalkSpeed(Zed, 200.f);
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (CurrentEnemy)
	{
		SetZedWalkSpeed(Zed, 500.f);
		SetFocus(CurrentEnemy);
		NextIdleActionTime = 0.f;

		if (bPlayerInRange)
		{
			StopMovement();
			ActiveAction = EZedNativeAction::None;
			if (Now >= NextAttackTime)
			{
				AttackPlayer();
				NextAttackTime = Now + FMath::FRandRange(0.5f, 1.f);
			}
			return;
		}

		if (GetMoveStatus() != EPathFollowingStatus::Moving || ActiveAction != EZedNativeAction::Chase)
		{
			MoveToActor(CurrentEnemy, 80.f, true, true, true);
			ActiveAction = EZedNativeAction::Chase;
		}
		return;
	}

	SetZedWalkSpeed(Zed, 200.f);
	SetFocus(nullptr);

	if (GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	if (ActiveAction == EZedNativeAction::RandomMove)
	{
		ActiveAction = EZedNativeAction::None;
		NextIdleActionTime = Now + FMath::FRandRange(2.84f, 10.99f);
		return;
	}

	if (ActiveAction == EZedNativeAction::Patrol)
	{
		ActiveAction = EZedNativeAction::None;
	}

	if (Now < NextIdleActionTime)
	{
		return;
	}

	if (PatrolTag != NAME_None)
	{
		StartPatrolMove(Zed);
	}
	else
	{
		StartRandomMove(Zed);
	}
}

void AZedAIController::ClearEnemy()
{
	UE_LOG(LogTemp, Display,
		TEXT("[ZedFallback] ClearEnemy zed=%s oldEnemy=%s patrolTag=%s action=%d moveStatus=%d"),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(CurrentEnemy),
		*PatrolTag.ToString(),
		static_cast<int32>(ActiveAction),
		static_cast<int32>(GetMoveStatus()));
	CurrentEnemy = nullptr;
	bPlayerInRange = false;
	ActiveAction = EZedNativeAction::None;
	ClearFocus(EAIFocusPriority::Gameplay);
	StopMovement();
}

void AZedAIController::AttackPlayer()
{
	AZedPawn* Zed = Cast<AZedPawn>(GetPawn());
	if (!Zed || Zed->GetIsDead())
	{
		return;
	}

	TArray<AActor*> ActorsToIgnore = { Zed };
	FHitResult OutResult;
	const FVector TraceStart = Zed->AttackPoint->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (Zed->AttackPoint->GetForwardVector() * 150.f);

	if (UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		16.f,
		ETraceTypeQuery::TraceTypeQuery15,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		OutResult,
		true))
	{
		AHordeBaseCharacter* Char = Cast<AHordeBaseCharacter>(OutResult.GetActor());
		if (Char && !Char->GetIsDead())
		{
			UGameplayStatics::ApplyPointDamage(Char, FMath::RandRange(5.f, 9.f), Zed->GetActorLocation(), OutResult, this, Zed, nullptr);
			Zed->PlayAttackFX();
		}
	}
}

void AZedAIController::StartRandomMove(AZedPawn* Zed)
{
	if (!Zed)
	{
		return;
	}

	FVector LocationToMove;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), Zed->GetActorLocation(), LocationToMove, 500.f, nullptr, nullptr))
	{
		const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(LocationToMove);
		UE_LOG(LogTemp, Display,
			TEXT("[ZedFallback] StartRandomMove zed=%s result=%d dest=(%.1f, %.1f, %.1f)"),
			*GetNameSafe(Zed),
			static_cast<int32>(MoveResult),
			LocationToMove.X, LocationToMove.Y, LocationToMove.Z);
		ActiveAction = EZedNativeAction::RandomMove;
	}
	else
	{
	}
}

void AZedAIController::StartPatrolMove(AZedPawn* Zed)
{
	if (!Zed)
	{
		return;
	}

	const FName TargetPatrolTag = PatrolTag;
	PatrolTag = NAME_None;

	const FVector LocationToMove = GetPatrolLocation(TargetPatrolTag);
	if (!LocationToMove.IsZero())
	{
		const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(LocationToMove);
		UE_LOG(LogTemp, Display,
			TEXT("[ZedFallback] StartPatrolMove zed=%s tag=%s result=%d dest=(%.1f, %.1f, %.1f)"),
			*GetNameSafe(Zed),
			*TargetPatrolTag.ToString(),
			static_cast<int32>(MoveResult),
			LocationToMove.X, LocationToMove.Y, LocationToMove.Z);
		ActiveAction = EZedNativeAction::Patrol;
	}
	else
	{
		UE_LOG(LogTemp, Display,
			TEXT("[ZedFallback] StartPatrolMoveFailed zed=%s tag=%s reason=no_core_point"),
			*GetNameSafe(Zed),
			*TargetPatrolTag.ToString());
	}
}

FVector AZedAIController::GetPatrolLocation(FName TargetPatrolTag) const
{
	TArray<FVector> PatrolLocations;
	for (TActorIterator<AAICorePoint> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		AAICorePoint* CorePoint = *ActorItr;
		if (CorePoint && CorePoint->PatrolTag == TargetPatrolTag)
		{
			PatrolLocations.Add(CorePoint->GetActorLocation());
		}
	}

	if (PatrolLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[ZedFallback] GetPatrolLocation tag=%s matches=0"),
			*TargetPatrolTag.ToString());
		return FVector::ZeroVector;
	}

	const FVector SelectedLocation = PatrolLocations[FMath::RandRange(0, PatrolLocations.Num() - 1)];
	UE_LOG(LogTemp, Display,
		TEXT("[ZedFallback] GetPatrolLocation tag=%s matches=%d selected=(%.1f, %.1f, %.1f)"),
		*TargetPatrolTag.ToString(),
		PatrolLocations.Num(),
		SelectedLocation.X, SelectedLocation.Y, SelectedLocation.Z);
	return SelectedLocation;
}

void AZedAIController::SetZedWalkSpeed(AZedPawn* Zed, float MaxWalkSpeed)
{
	if (Zed && Zed->GetCharacterMovement()->MaxWalkSpeed != MaxWalkSpeed)
	{
		Zed->ModifyWalkSpeed(MaxWalkSpeed);
	}
}
