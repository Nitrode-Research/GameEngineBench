// MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AuroraFunctionLibrary.generated.h"

class UAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class AURORA_API UAuroraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	bool SerializePCGActor(AActor* PCGActor, FString& OutJsonString);
	bool SerializeASCActor(AActor* PCGActor, FString& OutJsonString);
	
};
