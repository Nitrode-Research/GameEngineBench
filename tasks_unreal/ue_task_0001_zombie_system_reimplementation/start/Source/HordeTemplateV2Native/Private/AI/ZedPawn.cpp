#include "ZedPawn.h"
#include "Net/UnrealNetwork.h"

AZedPawn::AZedPawn()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
}

void AZedPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZedPawn, Health);
	DOREPLIFETIME(AZedPawn, IsDead);
	DOREPLIFETIME(AZedPawn, PatrolTag);
}
