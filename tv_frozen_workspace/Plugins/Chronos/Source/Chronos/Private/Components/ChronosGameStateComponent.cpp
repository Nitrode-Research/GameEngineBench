// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#include "Components/ChronosGameStateComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DaySequenceActor.h"
#include "Net/UnrealNetwork.h"

/**
* @file ChronosGameStateComponent.cpp
* @brief ChronosGameStateComponent
*/
UChronosGameStateComponent::UChronosGameStateComponent()
{
	SetIsReplicatedByDefault(true);
}

void UChronosGameStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UChronosGameStateComponent, CurrentGameTime);
}

void UChronosGameStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority()) // Only run this on the server
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADaySequenceActor::StaticClass(), FoundActors);

		if (FoundActors.Num() > 0)
		{
			DaySequenceActor = Cast<ADaySequenceActor>(FoundActors[0]);
			if (DaySequenceActor)
			{
				// Get the Day Sequence Actor and bind the delegate
				DaySequenceActor->GetOnDaySequenceUpdate().AddUObject(this, &ThisClass::OnDayTimeChanged);
			}
		}
	}
}

void UChronosGameStateComponent::OnDayTimeChanged()
{
	if (DaySequenceActor)
	{
		// The DaySequence is a looping sequence, so when it reaches the end and
		// loops, we can increment our day. A safe way to do this is to check
		// if the sequence is near its maximum playback time.
		float CurrentTime = DaySequenceActor->GetApparentTimeOfDay();
		float DayLength = DaySequenceActor->GetDayLength(); // DayLength is in hours.

		// Use a small epsilon to catch the wrap-around. For example, if the sequence
		// completes at 24.0, we can check if it's very close to 24.0 and
		// wasn't there last tick. A simpler approach is to check if it has passed a
		// certain threshold that indicates the day has ended (e.g., midnight).
		const float MidnightThreshold = 0.1f; // Check for the very start of a new day.

		if (CurrentTime < MidnightThreshold)
		{
			// You might also need a previous time check to avoid
			// incrementing multiple times on a single update tick near zero.
			// A simple boolean flag can prevent multiple increments.

			// GameDate.IncrementDay();

			// Log the update to see it working.
			UE_LOG(LogTemp, Warning, TEXT("New day! Current day is: %d"), CurrentGameDate.Day);
		}
	}
}
