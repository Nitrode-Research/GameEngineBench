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
		SCOPED_NAMED_EVENT(AcquireActorFromPool, FColor::White);

		AActor* AcquiredActor = nullptr;

		FActorPool* ActorPool = &AvailableActorPool.FindOrAdd(ActorClass);
		// Grab first available
		if (ActorPool->FreeActors.IsValidIndex(0))
		{
			UE_LOGFMT(LogGame, Log, "Acquired Actor for {actorclass} from pool", GetNameSafe(ActorClass));

			AcquiredActor = ActorPool->FreeActors[0];

			// Remove from pool
			// @todo: keep in pool but mark as in-use
			ActorPool->FreeActors.RemoveAtSwap(0, 1, EAllowShrinking::No);
		}

		// Failed to find actor
		if (AcquiredActor == nullptr)
		{
			UE_LOGFMT(LogGame, Log, "Actor Pool empty, spawning new Actor for {actorclass}", GetNameSafe(ActorClass));

			// Spawn fresh instance that can eventually be release to the pool
			return GetWorld()->SpawnActor<T>(ActorClass, SpawnTransform, SpawnParams);
		}

		AcquiredActor->SetActorTransform(SpawnTransform);
		AcquiredActor->SetInstigator(SpawnParams.Instigator);
		AcquiredActor->SetOwner(SpawnParams.Owner);

		// These are assumed not used by game code
		AcquiredActor->SetActorEnableCollision(true);
		AcquiredActor->SetActorHiddenInGame(false);

		AcquiredActor->DispatchBeginPlay();

		IRogueActorPoolingInterface::Execute_PoolBeginPlay(AcquiredActor);

		return CastChecked<T>(AcquiredActor);
	}

	bool ReleaseToPool_Internal(AActor* Actor);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

protected:

	/* Holds collection of available Actors, stored per class */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, FActorPool> AvailableActorPool;
};
