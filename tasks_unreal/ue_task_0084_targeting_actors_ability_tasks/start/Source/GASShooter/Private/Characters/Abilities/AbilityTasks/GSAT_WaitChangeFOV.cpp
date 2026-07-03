// Copyright 2020 Dan Kestranek.

#include "Characters/Abilities/AbilityTasks/GSAT_WaitChangeFOV.h"

UGSAT_WaitChangeFOV::UGSAT_WaitChangeFOV(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

UGSAT_WaitChangeFOV* UGSAT_WaitChangeFOV::WaitChangeFOV(UGameplayAbility* OwningAbility, FName TaskInstanceName, UCameraComponent* CameraComponent, float TargetFOV, float Duration, UCurveFloat* OptionalInterpolationCurve)
{
	return NewAbilityTask<UGSAT_WaitChangeFOV>(OwningAbility, TaskInstanceName);
}

void UGSAT_WaitChangeFOV::Activate()
{
	EndTask();
}

void UGSAT_WaitChangeFOV::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UGSAT_WaitChangeFOV::OnDestroy(bool AbilityIsEnding)
{
	Super::OnDestroy(AbilityIsEnding);
}
