#include "AlsxtAIController.h"

AAlsxtAIController::AAlsxtAIController()
{
	bAttachToPawn = true;
}

void AAlsxtAIController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	RunBehaviorTree(BehaviourTree);
}

FVector AAlsxtAIController::GetFocalPointOnActor(const AActor* Actor) const
{
	const auto* FocusedPawn{Cast<APawn>(Actor)};
	if (IsValid(FocusedPawn))
	{
		return FocusedPawn->GetPawnViewLocation();
	}

	return Super::GetFocalPointOnActor(Actor);
}
