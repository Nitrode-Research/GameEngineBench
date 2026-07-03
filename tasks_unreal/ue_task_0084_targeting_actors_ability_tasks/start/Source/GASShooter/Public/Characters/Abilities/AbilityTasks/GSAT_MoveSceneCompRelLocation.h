// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GSAT_MoveSceneCompRelLocation.generated.h"

class UCurveFloat;
class UCurveVector;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveSceneCompRelLocationDelegate);

UCLASS()
class GASSHOOTER_API UGSAT_MoveSceneCompRelLocation : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FMoveSceneCompRelLocationDelegate OnFinishMove;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UGSAT_MoveSceneCompRelLocation* MoveSceneComponentRelativeLocation(UGameplayAbility* OwningAbility, FName TaskInstanceName, USceneComponent* SceneComponent, FVector Location, float Duration, UCurveFloat* OptionalInterpolationCurve, UCurveVector* OptionalVectorInterpolationCurve);

	virtual void Activate() override;

	virtual void TickTask(float DeltaTime) override;

	virtual void OnDestroy(bool AbilityIsEnding) override;

protected:
	bool bIsFinished;

	FVector StartLocation;

	FVector TargetLocation;

	float DurationOfMovement;

	float TimeMoveStarted;

	float TimeMoveWillEnd;

	UPROPERTY()
	USceneComponent* Component;

	UPROPERTY()
	UCurveFloat* LerpCurve;

	UPROPERTY()
	UCurveVector* LerpCurveVector;
};
