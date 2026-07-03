// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "RicochetBullet.h"
#include "GameFramework/Actor.h"
#include "RicochetBulletPoolManager.generated.h"

UCLASS()
class RICOCHET_API ARicochetBulletPoolManager : public AActor
{
	GENERATED_BODY()

public:    
	ARicochetBulletPoolManager();

protected:
	virtual void BeginPlay() override;

	void SpawnNewBullet();

public:    
	// Function to get an available bullet from the pool
	ARicochetBullet* GetAvailableBullet();

protected:
	// The class of the bullets to spawn
	UPROPERTY(EditAnywhere, Category="BulletPool")
	TSubclassOf<ARicochetBullet> BulletClass;

	// The initial size of the bullet pool
	UPROPERTY(EditAnywhere, Category="BulletPool")
	int32 InitialPoolSize = 10;

	// Maximum size of the bullet pool
	UPROPERTY(EditAnywhere, Category="BulletPool")
	int32 MaximumPoolSize = 100;

private:
	// The array holding the bullet pool
	TArray<ARicochetBullet*> BulletPool;
};
