// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AxosBlueprintFunctionLibrary.generated.h"

UCLASS()
class AXOS_API UAxosBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Actor Bearing")
	static float GetActorBearing(AActor* TargetActor, AGameState* GameState);
};