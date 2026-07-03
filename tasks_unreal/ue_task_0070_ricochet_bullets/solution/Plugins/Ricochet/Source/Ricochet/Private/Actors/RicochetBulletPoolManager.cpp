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
	if (!HasAuthority()) return;

	/**
	 * @todo Replace with only spawning very few at first, and then adding to the Array on demand, until MaxPoolSize is met, and then recycle from then on out
	 */
	for (int32 i = 0; i < InitialPoolSize; ++i)
	{
		ARicochetBullet* NewBullet = GetWorld()->SpawnActor<ARicochetBullet>(BulletClass, FVector::ZeroVector, FRotator::ZeroRotator);
		if (NewBullet)
		{
			NewBullet->DeactivateBullet();
			BulletPool.Add(NewBullet);
		}
	}
}

void ARicochetBulletPoolManager::SpawnNewBullet()
{
	ARicochetBullet* NewBullet = GetWorld()->SpawnActor<ARicochetBullet>(BulletClass, FVector::ZeroVector, FRotator::ZeroRotator);
	if (NewBullet)
	{
		if (BulletPool.Num() < MaximumPoolSize)
		{
			NewBullet->DeactivateBullet();
			BulletPool.Add(NewBullet);
		}
		else
		{
			// Get existing Bullet
		}
		
	}
}

ARicochetBullet* ARicochetBulletPoolManager::GetAvailableBullet()
{
	if (!HasAuthority()) return nullptr;

	for (ARicochetBullet* Bullet : BulletPool)
	{
		if (!Bullet->bIsActive)
		{
			return Bullet;
		}
	}
	return nullptr;
}
