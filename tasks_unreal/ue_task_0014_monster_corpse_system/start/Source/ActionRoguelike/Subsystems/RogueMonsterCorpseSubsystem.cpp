// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueMonsterCorpseSubsystem.h"

#include "Performance/RogueActorPoolingSubsystem.h"
#include "World/RogueMonsterCorpse.h"


ARogueMonsterCorpse* URogueMonsterCorpseSubsystem::FetchCorpse(AActor* InActor, URogueMonsterData* MonsterData)
{
	return nullptr;
}


void URogueMonsterCorpseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


void URogueMonsterCorpseSubsystem::CleanupNextAvailableCorpse()
{
}


void URogueMonsterCorpseSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
