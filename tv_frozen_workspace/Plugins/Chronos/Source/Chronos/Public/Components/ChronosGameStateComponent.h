// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/ChronosTypes.h"
#include "DaySequenceActor.h"
#include "ChronosGameStateComponent.generated.h"

// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMillenniumChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnCenturyChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnYearChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMonthChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnWeekChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnDayChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnHourChanged, int, OldValue, int, NewValue);
// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMinuteChanged, int, OldValue, int, NewValue);

/**
* @file ChronosGameStateComponent.h
* @brief ChronosGameStateComponent
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHRONOS_API UChronosGameStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChronosGameStateComponent();

	virtual void BeginPlay() override;

	// FOnMillenniumChanged OnMillenniumChanged;
	// FOnCenturyChanged OnCenturyChanged;
	// FOnYearChanged OnYearChanged;
	// FOnMonthChanged OnMonthChanged;
	// FOnWeekChanged OnWeekChanged;
	// FOnDayChanged OnDayChanged;
	// FOnHourChanged OnHourChanged;
	// FOnMinuteChanged OnMinuteChanged;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Time")
	FChronosTimeofDay CurrentGameTime;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Time")
	FChronosDate CurrentGameDate;

	UFUNCTION()
	void OnDayTimeChanged();

private:
	UPROPERTY()
	ADaySequenceActor* DaySequenceActor;
};
