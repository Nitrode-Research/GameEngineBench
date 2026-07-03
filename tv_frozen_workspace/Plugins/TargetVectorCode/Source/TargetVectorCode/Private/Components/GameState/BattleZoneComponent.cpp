// MIT


#include "Components/GameState/BattleZoneComponent.h"

// Sets default values for this component's properties
UBattleZoneComponent::UBattleZoneComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBattleZoneComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void UBattleZoneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}