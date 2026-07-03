// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_MoveSceneCompRelLocation.h"

UGSAT_MoveSceneCompRelLocation::UGSAT_MoveSceneCompRelLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

UGSAT_MoveSceneCompRelLocation* UGSAT_MoveSceneCompRelLocation::MoveSceneComponentRelativeLocation(UGameplayAbility* OwningAbility, FName TaskInstanceName, USceneComponent* SceneComponent, FVector Location, float Duration, UCurveFloat* OptionalInterpolationCurve, UCurveVector* OptionalVectorInterpolationCurve)
{
	return NewAbilityTask<UGSAT_MoveSceneCompRelLocation>(OwningAbility, TaskInstanceName);
}

void UGSAT_MoveSceneCompRelLocation::Activate()
{
	EndTask();
}

void UGSAT_MoveSceneCompRelLocation::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UGSAT_MoveSceneCompRelLocation::OnDestroy(bool AbilityIsEnding)
{
	Super::OnDestroy(AbilityIsEnding);
}
