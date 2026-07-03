// MIT


#include "Components/AxosGameStateComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UAxosGameStateComponent::UAxosGameStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	NorthYaw = 0.0f;
	UseOverrideYaw  = false;
}

void UAxosGameStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, UseOverrideYaw, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, NorthYaw, Parameters)
}


void UAxosGameStateComponent::OnRep_UseOverrideYaw()
{
	//
}

void UAxosGameStateComponent::OnRep_NorthYaw()
{
	AxosLevelActor.Get()->Server_SetYaw(NorthYaw);
}

float UAxosGameStateComponent::GetNorthYaw()
{
	return NorthYaw;
}

void UAxosGameStateComponent::SetNorthYaw(const float NewNorthYaw)
{
	OnNorthYawChanged.Broadcast(NorthYaw, NewNorthYaw);
	NorthYaw = NewNorthYaw;
}

// Called when the game starts
void UAxosGameStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAxosGameStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

