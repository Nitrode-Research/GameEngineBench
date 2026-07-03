#include "EndGameTrigger.h"

AEndGameTrigger::AEndGameTrigger()
{
	SetReplicates(true);
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Player Collision"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetRelativeScale3D(FVector(2.f, 2.f, 2.f));

	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AEndGameTrigger::OnColide);
}

void AEndGameTrigger::OnColide(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {}
