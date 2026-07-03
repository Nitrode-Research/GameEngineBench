// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "BlueprintFunctionLibrary/ChronosTimeConversionLibrary.h"

FChronosTimeofDay UChronosTimeConversionLibrary::Convert24HrFloatToCustomTime(float TimeOfDayFloat, FChronosTimeofDayDefinition TimeOfDayDefinition)
{
	FChronosTimeofDay CustomTime;

	// Ensure inputs are valid
	TimeOfDayDefinition.HoursPerDay = FMath::Max(1, TimeOfDayDefinition.HoursPerDay);
	TimeOfDayDefinition.MinutesPerHour = FMath::Max(1, TimeOfDayDefinition.MinutesPerHour);
	TimeOfDayDefinition.SecondsPerMinute = FMath::Max(1, TimeOfDayDefinition.SecondsPerMinute);

	// Calculate total seconds in the custom day
	float TotalSecondsInCustomDay = static_cast<float>(TimeOfDayDefinition.HoursPerDay) * TimeOfDayDefinition.MinutesPerHour * TimeOfDayDefinition.SecondsPerMinute;

	// Convert the 24-hour TimeOfDayFloat to a proportion of the custom day
	// Assuming TimeOfDayFloat is a value between 0.0 (midnight) and 24.0 (end of day)
	float ProportionOfDay = TimeOfDayFloat / 24.0f;

	// Calculate the total seconds into the custom day
	float TotalSecondsElapsed = ProportionOfDay * TotalSecondsInCustomDay;

	// Extract hours, minutes, and seconds
	CustomTime.Hour = FMath::FloorToInt(TotalSecondsElapsed / (TimeOfDayDefinition.MinutesPerHour * TimeOfDayDefinition.SecondsPerMinute));
	TotalSecondsElapsed -= CustomTime.Hour * (TimeOfDayDefinition.MinutesPerHour * TimeOfDayDefinition.SecondsPerMinute);

	CustomTime.Minute = FMath::FloorToInt(TotalSecondsElapsed / TimeOfDayDefinition.SecondsPerMinute);
	TotalSecondsElapsed -= CustomTime.Minute * TimeOfDayDefinition.SecondsPerMinute;

	CustomTime.Second = FMath::FloorToInt(TotalSecondsElapsed);

	return CustomTime;
}