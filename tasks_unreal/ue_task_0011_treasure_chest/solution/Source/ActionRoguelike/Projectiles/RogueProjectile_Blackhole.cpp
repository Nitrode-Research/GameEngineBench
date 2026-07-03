// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueProjectile_Blackhole.h"

#include "Animation/RogueCurveAnimSubsystem.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"


ARogueProjectile_Blackhole::ARogueProjectile_Blackhole()
{
	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->RemoveObjectTypeToAffect(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
}

void ARogueProjectile_Blackhole::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


void ARogueProjectile_Blackhole::OnOverlappedPhysicsActor(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}


void ARogueProjectile_Blackhole::BeginPlay()
{
	Super::BeginPlay();
}
