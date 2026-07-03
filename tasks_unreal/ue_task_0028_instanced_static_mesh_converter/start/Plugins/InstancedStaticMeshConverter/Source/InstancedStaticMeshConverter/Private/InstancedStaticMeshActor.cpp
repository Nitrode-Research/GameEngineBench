// Copyright (c) Yevhenii Selivanov

#include "InstancedStaticMeshActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InstancedStaticMeshActor)

AInstancedStaticMeshActor::AInstancedStaticMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AInstancedStaticMeshActor::SpawnInstanceByActor(const FTransform& Transform, TSubclassOf<AActor> ActorBlueprint)
{
}

void AInstancedStaticMeshActor::SpawnInstanceByMesh(const FTransform& Transform, const UStaticMesh* Mesh)
{
}

void AInstancedStaticMeshActor::ResetAllInstances()
{
}

void AInstancedStaticMeshActor::Destroyed()
{
	Super::Destroyed();
}

FCachedActorMeshInstances* AInstancedStaticMeshActor::FindCachedActorMeshInstances(TSubclassOf<AActor> ActorBlueprint)
{
	return nullptr;
}

FCachedActorMeshInstances* AInstancedStaticMeshActor::FindOrCreateInstancedMeshes(TSubclassOf<AActor> ActorClass)
{
	return nullptr;
}
