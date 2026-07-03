// MIT


#include "Components/Character/AlsxtCharacterCustomizationComponent.h"
#include "Components/Mesh/AlsxtPaintableSkeletalMeshComponent.h"

// Sets default values for this component's properties
UAlsxtCharacterCustomizationComponent::UAlsxtCharacterCustomizationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtCharacterCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();

}


// Called every frame
void UAlsxtCharacterCustomizationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FALSXTCharacterVoiceParameters UAlsxtCharacterCustomizationComponent::GetVoiceParameters()
{
	return VoiceParameters;
}

TArray<UAlsxtPaintableSkeletalMeshComponent*> UAlsxtCharacterCustomizationComponent::GetAllComponents()
{
	TArray<UAlsxtPaintableSkeletalMeshComponent*> AllComponents;
	return AllComponents;
}

TArray<UAlsxtPaintableSkeletalMeshComponent*> UAlsxtCharacterCustomizationComponent::GetHighlightableComponents()
{
	TArray<UAlsxtPaintableSkeletalMeshComponent*> AllComponents {GetAllComponents()};
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

