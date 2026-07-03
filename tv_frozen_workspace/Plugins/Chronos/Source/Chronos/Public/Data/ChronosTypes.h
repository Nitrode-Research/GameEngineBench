// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "ChronosTypes.generated.h"

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosTimeofDay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Hour {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Minute {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Second {0};

	bool operator==(const FChronosTimeofDay& other) const
	{
		return (other.Hour == Hour) && (other.Minute == Minute) && (other.Second == Second);
	}
};

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosTimeofDayDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int HoursPerDay {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int MinutesPerHour {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int SecondsPerMinute {0};

	FChronosTimeofDayDefinition() : HoursPerDay(0), MinutesPerHour(0), SecondsPerMinute(0) {}

	bool operator==(const FChronosTimeofDayDefinition& other) const
	{
		return (other.HoursPerDay == HoursPerDay) && (other.MinutesPerHour == MinutesPerHour) && (other.SecondsPerMinute == SecondsPerMinute);
	}
};

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosDate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Month {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Day {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Year {0};

	bool operator==(const FChronosDate& other) const
	{
		return (other.Month == Month) && (other.Day == Day) && (other.Year == Year);
	}
};

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosWeekDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Days {0};

	bool operator==(const FChronosWeekDefinition& other) const
	{
		return (other.Days == Days);
	}
};

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosMonthDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FChronosWeekDefinition Weeks;

	bool operator==(const FChronosMonthDefinition& other) const
	{
		return (other.Weeks == Weeks);
	}
};

USTRUCT(BlueprintType)
struct CHRONOS_API FChronosYearDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FChronosMonthDefinition> Months;


	bool operator==(const FChronosYearDefinition& other) const
	{
		return (other.Months == Months);
	}
};

UCLASS(Blueprintable, BlueprintType)
class CHRONOS_API UChronosYearDefinitionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	FChronosYearDefinition Years;
};