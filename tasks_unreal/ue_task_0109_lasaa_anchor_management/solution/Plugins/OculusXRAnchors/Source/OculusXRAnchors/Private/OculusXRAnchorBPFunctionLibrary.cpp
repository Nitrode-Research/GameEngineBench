#include "OculusXRAnchorBPFunctionLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

FOculusXRUUID UOculusXRAnchorBPFunctionLibrary::StringToAnchorUUID(const FString& UUIDString)
{
	return FOculusXRUUID(UUIDString);
}

AActor* UOculusXRAnchorBPFunctionLibrary::SpawnActorWithAnchorQueryResults(UWorld* World, const FOculusXRSpaceQueryResult& QueryResult, UClass* ActorClass, UObject*, UObject*, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride)
{
	if (!World || !ActorClass)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = CollisionHandlingOverride;
	return World->SpawnActor<AActor>(ActorClass, QueryResult.Transform, Params);
}