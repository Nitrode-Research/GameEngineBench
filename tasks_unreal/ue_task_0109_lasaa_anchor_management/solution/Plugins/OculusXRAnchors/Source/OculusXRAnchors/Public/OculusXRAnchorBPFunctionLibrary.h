#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OculusXRAnchors.h"
#include "OculusXRAnchorBPFunctionLibrary.generated.h"

UCLASS()
class OCULUSXRANCHORS_API UOculusXRAnchorBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="OculusXRAnchors")
	static FOculusXRUUID StringToAnchorUUID(const FString& UUIDString);

	static AActor* SpawnActorWithAnchorQueryResults(UWorld* World, const FOculusXRSpaceQueryResult& QueryResult, UClass* ActorClass, UObject* Owner, UObject* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride);
};