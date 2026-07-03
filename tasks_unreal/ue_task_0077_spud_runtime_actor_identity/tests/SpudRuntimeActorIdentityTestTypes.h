#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISpudObject.h"
#include "SpudRuntimeActorIdentityTestTypes.generated.h"

UCLASS()
class ASpudRuntimeIdentityTestActor : public AActor, public ISpudObject
{
	GENERATED_BODY()

public:
	ASpudRuntimeIdentityTestActor()
	{
		PrimaryActorTick.bCanEverTick = false;
	}

	UPROPERTY(SaveGame)
	FGuid SpudGuid;

	UPROPERTY(SaveGame)
	int32 SavedValue = 0;

	UPROPERTY(SaveGame)
	TObjectPtr<ASpudRuntimeIdentityTestActor> LinkedActor = nullptr;

	UPROPERTY(SaveGame)
	TWeakObjectPtr<ASpudRuntimeIdentityTestActor> WeakLinkedActor;
};

UCLASS()
class ASpudRuntimeIdentityNamedActor : public ASpudRuntimeIdentityTestActor
{
	GENERATED_BODY()

public:
	virtual ESpudRespawnMode GetSpudRespawnMode_Implementation() const override
	{
		return ESpudRespawnMode::NeverRespawn;
	}

	virtual FString OverrideName_Implementation() const override
	{
		return TEXT("StableAutoCreatedRuntimeActor");
	}
};

UCLASS()
class ASpudRuntimeIdentityRoamingActor : public ASpudRuntimeIdentityTestActor, public ISpudRoamingActor
{
	GENERATED_BODY()
};
