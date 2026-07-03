#include "Components/RicochetMagazineDetachable.h"

URicochetMagazineDetachable::URicochetMagazineDetachable()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URicochetMagazineDetachable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void URicochetMagazineDetachable::BeginPlay()
{
	Super::BeginPlay();
}

void URicochetMagazineDetachable::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

float URicochetMagazineDetachable::GetMagazineWeight()
{
	return 0.0f;
}

float URicochetMagazineDetachable::GetTotalMagazineWeight()
{
	return 0.0f;
}
