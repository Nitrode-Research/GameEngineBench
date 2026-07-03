// MIT


#include "Components/Character/AlsxtStationaryModeComponent.h"

// Sets default values for this component's properties
UAlsxtStationaryModeComponent::UAlsxtStationaryModeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtStationaryModeComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAlsxtStationaryModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UAlsxtStationaryModeComponent::TryTraceForSeat(){}

void UAlsxtStationaryModeComponent::GetObjectStationaryModeInfo() {}

void UAlsxtStationaryModeComponent::GetEnterStationaryModeAnimation() {}

void UAlsxtStationaryModeComponent::EnterStationaryMode() {}

void UAlsxtStationaryModeComponent::GetExitStationaryModeAnimation() {}

void UAlsxtStationaryModeComponent::ExitStationaryMode() {}
