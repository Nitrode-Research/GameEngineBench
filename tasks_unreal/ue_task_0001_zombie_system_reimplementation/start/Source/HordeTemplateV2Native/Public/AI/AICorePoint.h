#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AICorePoint.generated.h"

UCLASS()
class HORDETEMPLATEV2NATIVE_API AAICorePoint : public AActor
{
	GENERATED_BODY()

public:
	AAICorePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Target")
	FName PatrolTag = NAME_None;
};
