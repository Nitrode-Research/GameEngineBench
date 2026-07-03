#pragma once

#include "AIController.h"
#include "AlsxtAIController.generated.h"

UCLASS(DisplayName = "Als AI Controller")
class ALSXT_API AAlsxtAIController : public AAIController
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings", Meta = (AllowPrivateAccess))
	TObjectPtr<UBehaviorTree> BehaviourTree;

public:
	AAlsxtAIController();

protected:
	virtual void OnPossess(APawn* NewPawn) override;

public:
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;
};
