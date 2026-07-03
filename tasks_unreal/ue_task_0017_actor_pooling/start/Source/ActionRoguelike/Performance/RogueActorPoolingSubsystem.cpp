// Fill out your copyright notice in the Description page of Project Settings.


#include "Performance/RogueActorPoolingSubsystem.h"

#include "ActionRoguelike.h"
#include "RogueActorPoolingInterface.h"
#include "Logging/StructuredLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueActorPoolingSubsystem)


static TAutoConsoleVariable CVarActorPoolingEnabled(
	TEXT("game.ActorPooling"),
	false, // Disabled by default in sample project. Has some issues to work out (such as properly resetting all VFX like the black hole projectile)
	TEXT("Enable actor pooling for selected objects."),
	ECVF_Default);


AActor* URogueActorPoolingSubsystem::SpawnActorPooled(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod SpawnHandling)
{
	return nullptr;
}

bool URogueActorPoolingSubsystem::ReleaseToPool(AActor* Actor)
{
	return false;
}


bool URogueActorPoolingSubsystem::IsPoolingEnabled(const UObject* WorldContextObject)
{
	return false;
}

void URogueActorPoolingSubsystem::PrimeActorPool(TSubclassOf<AActor> ActorClass, int32 Amount)
{
}


bool URogueActorPoolingSubsystem::ReleaseToPool_Internal(AActor* Actor)
{
	return false;
}

void URogueActorPoolingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WidgetPool.SetWorld(GetWorld());
}

void URogueActorPoolingSubsystem::Deinitialize()
{
	Super::Deinitialize();

	WidgetPool.ReleaseAll(true);
	WidgetPool.ReleaseAllSlateResources();
}
