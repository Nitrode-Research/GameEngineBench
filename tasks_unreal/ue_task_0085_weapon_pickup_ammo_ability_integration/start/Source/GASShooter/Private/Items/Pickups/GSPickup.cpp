// Copyright 2020 Dan Kestranek.

#include "Items/Pickups/GSPickup.h"

#include "Components/CapsuleComponent.h"
#include "GASShooter/GASShooter.h"
#include "Net/UnrealNetwork.h"

AGSPickup::AGSPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bIsActive = true;
	bCanRespawn = true;
	RespawnTime = 5.0f;
	PickedUpBy = nullptr;

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionComp"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.KnockedDown"));
}

void AGSPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSPickup, bIsActive);
	DOREPLIFETIME(AGSPickup, PickedUpBy);
}

void AGSPickup::NotifyActorBeginOverlap(AActor* Other)
{
	Super::NotifyActorBeginOverlap(Other);
}

bool AGSPickup::CanBePickedUp(AGSCharacterBase* TestCharacter) const
{
	return false;
}

bool AGSPickup::K2_CanBePickedUp_Implementation(AGSCharacterBase* TestCharacter) const
{
	return false;
}

void AGSPickup::PickupOnTouch(AGSCharacterBase* Pawn)
{
}

void AGSPickup::GivePickupTo(AGSCharacterBase* Pawn)
{
}

void AGSPickup::OnPickedUp()
{
}

void AGSPickup::RespawnPickup()
{
}

void AGSPickup::OnRespawned()
{
}

void AGSPickup::OnRep_IsActive()
{
}
