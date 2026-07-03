// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionRoguelike.h"
#include "RogueActorPoolingInterface.h"
#include "Blueprint/UserWidgetPool.h"
#include "Logging/StructuredLog.h"
#include "Subsystems/WorldSubsystem.h"
#include "RogueActorPoolingSubsystem.generated.h"

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> FreeActors;
};

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API URogueActorPoolingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category= "Actor Pooling", meta = (WorldContext="WorldContextObject"))
	static AActor* SpawnActorPooled(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod SpawnHandling);

	static bool ReleaseToPool(AActor* Actor);

	template <class T>
	static T* AcquireFromPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, FActorSpawnParameters SpawnParams = FActorSpawnParameters())
	{
		if (IsPoolingEnabled(WorldContextObject))
		{
			URogueActorPoolingSubsystem* PoolingSystem = WorldContextObject->GetWorld()->GetSubsystem<URogueActorPoolingSubsystem>();
			return PoolingSystem->AcquireFromPool_Internal<T>(ActorClass, SpawnTransform, SpawnParams);
		}

		SCOPED_NAMED_EVENT(SpawnActorNoPool, FColor::Red);
		// Fallback to standard spawning when not enabled
		return WorldContextObject->GetWorld()->SpawnActor<T>(ActorClass, SpawnTransform, SpawnParams);
	}

	static bool IsPoolingEnabled(const UObject* WorldContextObject);

	void PrimeActorPool(TSubclassOf<AActor> ActorClass, int32 Amount);
	
	UPROPERTY(Transient)
	FUserWidgetPool WidgetPool;

protected:

	template <class T>
	T* AcquireFromPool_Internal(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, FActorSpawnParameters SpawnParams = FActorSpawnParameters())
	{
		return nullptr;
	}

	bool ReleaseToPool_Internal(AActor* Actor);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

protected:

	/* Holds collection of available Actors, stored per class */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, FActorPool> AvailableActorPool;
};
