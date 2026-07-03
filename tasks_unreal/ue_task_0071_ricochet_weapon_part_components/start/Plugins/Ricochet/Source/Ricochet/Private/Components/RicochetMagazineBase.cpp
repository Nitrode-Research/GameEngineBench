#include "Components/RicochetMagazineBase.h"

URicochetMagazineBase::URicochetMagazineBase()
{
}

void URicochetMagazineBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

UAbilitySystemComponent* URicochetMagazineBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
