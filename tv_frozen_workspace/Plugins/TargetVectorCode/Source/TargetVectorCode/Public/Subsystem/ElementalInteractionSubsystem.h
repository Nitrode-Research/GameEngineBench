// MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ElementalInteractionSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TARGETVECTORCODE_API UElementalInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
};
