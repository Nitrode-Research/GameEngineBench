// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupActor.h"

#include "ActionRoguelike.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/RoguePlayerCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RoguePickupActor)


ARoguePickupActor::ARoguePickupActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = SphereComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);

	bReplicates = true;
}


void ARoguePickupActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ARoguePickupActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void ARoguePickupActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                     int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}


void ARoguePickupActor::Interact_Implementation(AController* InstigatorController)
{
}


FText ARoguePickupActor::GetInteractText_Implementation(AController* InstigatorController)
{
	return FText::GetEmpty();
}


void ARoguePickupActor::ShowPickup()
{
}


void ARoguePickupActor::HideAndCooldown()
{
}

void ARoguePickupActor::SetPickupState(bool bNewIsActive)
{
}


void ARoguePickupActor::OnRep_IsActive()
{
}


void ARoguePickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARoguePickupActor, bIsActive);
}
