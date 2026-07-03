#include "Components/RicochetMuzzleAttachment.h"

URicochetMuzzleAttachment::URicochetMuzzleAttachment()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URicochetMuzzleAttachment::BeginPlay()
{
	Super::BeginPlay();
}

void URicochetMuzzleAttachment::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void URicochetMuzzleAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

float URicochetMuzzleAttachment::GetMuzzleAttachmentWeight()
{
	return 0.0f;
}

UAbilitySystemComponent* URicochetMuzzleAttachment::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
