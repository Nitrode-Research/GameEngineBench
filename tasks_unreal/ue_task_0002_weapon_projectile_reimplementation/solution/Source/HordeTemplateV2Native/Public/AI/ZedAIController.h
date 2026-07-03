

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIModule/Classes/Perception/AISenseConfig_Sight.h"
#include "AIModule/Classes/Perception/AIPerceptionComponent.h"
#include "ZedAIController.generated.h"

class AHordeBaseCharacter;
class AZedPawn;

/**
 * 
 */
UCLASS()
class HORDETEMPLATEV2NATIVE_API AZedAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZedAIController();

	UFUNCTION()
		void EnemyInSight(AActor* Actor, FAIStimulus Stimulus);

	void SetPlayerInRange(bool bInRange);
	void SetPatrolTag(FName NewPatrolTag);
	void NotifyZedDied();
protected:

	UPROPERTY()
		FTimerHandle SightClearTimer;

	UPROPERTY()
		FTimerHandle NativeDecisionTimer;

	UPROPERTY()
		AHordeBaseCharacter* CurrentEnemy = nullptr;

	UPROPERTY()
		FName PatrolTag = NAME_None;

	bool bPlayerInRange = false;
	bool bZedDead = false;
	float NextAttackTime = 0.f;
	float NextIdleActionTime = 0.f;

	enum class EZedNativeAction : uint8
	{
		None,
		Patrol,
		RandomMove,
		Chase
	};

	EZedNativeAction ActiveAction = EZedNativeAction::None;

	UFUNCTION()
		void ClearSight();

	void TickNativeAI();
	void ClearEnemy();
	void AttackPlayer();
	void StartRandomMove(AZedPawn* Zed);
	void StartPatrolMove(AZedPawn* Zed);
	FVector GetPatrolLocation(FName TargetPatrolTag) const;
	void SetZedWalkSpeed(AZedPawn* Zed, float MaxWalkSpeed);

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Target")
		class UAIPerceptionComponent* PCC;
};
