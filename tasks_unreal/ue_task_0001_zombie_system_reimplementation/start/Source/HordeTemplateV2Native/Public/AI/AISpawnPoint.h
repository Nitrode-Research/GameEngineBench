#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AISpawnPoint.generated.h"

UCLASS(ClassGroup="Horde AI")
class HORDETEMPLATEV2NATIVE_API AAISpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AAISpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName PatrolTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI")
	bool SpawnNotFree = false;
};
