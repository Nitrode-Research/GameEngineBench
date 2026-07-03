#include "Med_VAC.h"

void AMed_VAC::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMed_VAC, IsHealing);
}

AMed_VAC::AMed_VAC()
{
	Weapon->SetSkeletalMesh(nullptr);
	VACMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Vac Mesh"));
	VACMesh->SetupAttachment(RootComponent);
}

void AMed_VAC::FireFirearm()
{
}

void AMed_VAC::FinishHealing_Implementation()
{
}

bool AMed_VAC::FinishHealing_Validate()
{
	return true;
}

void AMed_VAC::StartHealing_Implementation()
{
}

bool AMed_VAC::StartHealing_Validate()
{
	return true;
}
