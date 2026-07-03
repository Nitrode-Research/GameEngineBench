// MIT


#include "Components/Character/AlsxtCharacterEquipmentComponent.h"
#include "Components/Mesh/AlsxtPaintableSkeletalMeshComponent.h"

// Sets default values for this component's properties
UAlsxtCharacterEquipmentComponent::UAlsxtCharacterEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtCharacterEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAlsxtCharacterEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

TArray<UAlsxtPaintableSkeletalMeshComponent*> UAlsxtCharacterEquipmentComponent::GetAllComponents()
{
	TArray<UAlsxtPaintableSkeletalMeshComponent*> AllComponents;
	return AllComponents;
}

TArray<UAlsxtPaintableSkeletalMeshComponent*> UAlsxtCharacterEquipmentComponent::GetHighlightableComponents()
{
	TArray<UAlsxtPaintableSkeletalMeshComponent*> AllComponents{ GetAllComponents() };
	TArray<UAlsxtPaintableSkeletalMeshComponent*> HighlightableComponents;

	for (UAlsxtPaintableSkeletalMeshComponent* Component : AllComponents)
	{
		if (IsValid(Component))
		{
			HighlightableComponents.Add(Component);
		}
	}
	return HighlightableComponents;
}
