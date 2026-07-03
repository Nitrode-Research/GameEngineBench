#include "Components/RicochetBarrel.h"

URicochetBarrel::URicochetBarrel()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URicochetBarrel::BeginPlay()
{
	Super::BeginPlay();
}

void URicochetBarrel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void URicochetBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

UAbilitySystemComponent* URicochetBarrel::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
