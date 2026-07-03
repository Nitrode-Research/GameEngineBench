#include "Actors/RicochetBulletPoolManager.h"
#include "Actors/RicochetBullet.h"

ARicochetBulletPoolManager::ARicochetBulletPoolManager()
{
	// This actor is spawned in the world and doesn't need to be replicated
	bReplicates = false; 
}

void ARicochetBulletPoolManager::BeginPlay()
{
	Super::BeginPlay();
}

void ARicochetBulletPoolManager::SpawnNewBullet()
{
}

ARicochetBullet* ARicochetBulletPoolManager::GetAvailableBullet()
{
	return nullptr;
}
