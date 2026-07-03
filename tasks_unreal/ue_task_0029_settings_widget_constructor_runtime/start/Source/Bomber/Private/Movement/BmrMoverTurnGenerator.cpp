// Copyright (c) Yevhenii Selivanov

#include "Movement/BmrMoverTurnGenerator.h"

// Bomber
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrMoverTurnGenerator)

// Is overridden to handle orientation-related movement
FVector UBmrMoverTurnGenerator::GetTurn_Implementation(FRotator TargetOrientation, const FMoverTickStartData& FullStartState, const FMoverDefaultSyncState& MoverState, const FMoverTimeStep& TimeStep, const FProposedMove& ProposedMove, UMoverBlackboard* SimBlackboard)
{
	FRotator FinalTargetOrientation = TargetOrientation;

	// Override target if we should orient to movement
	if (bOrientRotationToMovement)
	{
		const FCharacterDefaultInputs* CharacterInputs = FullStartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
		if (CharacterInputs && !CharacterInputs->GetMoveInput_WorldSpace().IsNearlyZero())
		{
			FinalTargetOrientation = CharacterInputs->GetMoveInput_WorldSpace().ToOrientationRotator();
		}
	}

	return Super::GetTurn_Implementation(FinalTargetOrientation, FullStartState, MoverState, TimeStep, ProposedMove, SimBlackboard);
}