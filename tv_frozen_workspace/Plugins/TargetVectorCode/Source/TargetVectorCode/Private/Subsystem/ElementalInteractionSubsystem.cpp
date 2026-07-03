// MIT


#include "Subsystem/ElementalInteractionSubsystem.h"

void UElementalInteractionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UElementalInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UElementalInteractionSubsystem, STATGROUP_Tickables);
}