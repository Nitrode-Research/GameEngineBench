// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueMonsterCorpse.h"
#include "ActionRoguelike.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/RogueMonsterData.h"


ARogueMonsterCorpse::ARogueMonsterCorpse()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RagdollMeshComp"));
	MeshComp->SetCollisionProfileName(Collision::Ragdoll_ProfileName);
	RootComponent = MeshComp;
}


void ARogueMonsterCorpse::SetCorpseProperties(USkeletalMeshComponent* ReferenceMeshComp, URogueMonsterData* MonsterData)
{
}

bool ARogueMonsterCorpse::AddImpulseAtLocationCustom(FVector Impulse, FVector Location, FName BoneName)
{
	return false;
}
