// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueExplosiveBarrel.h"
#include "ActionRoguelike.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueExplosiveBarrel)


ARogueExplosiveBarrel::ARogueExplosiveBarrel()
{
	ActionComp = CreateDefaultSubobject<URogueActionComponent>(TEXT("ActionComp"));
	ActionComp->SetDefaultAttributeSet(URogueHealthAttributeSet::StaticClass());

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	RootComponent = MeshComp;

	ForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("ForceComp"));
	ForceComp->SetupAttachment(MeshComp);

	ExplosionComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ExplosionComp"));
	ExplosionComp->SetupAttachment(MeshComp);

	FlamesFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlamesFXComp"));
	FlamesFXComp->SetupAttachment(MeshComp);
}

void ARogueExplosiveBarrel::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


void ARogueExplosiveBarrel::OnHealthAttributeChanged(float NewValue, const FAttributeModification& AttributeModification)
{
}


void ARogueExplosiveBarrel::Explode()
{
}
