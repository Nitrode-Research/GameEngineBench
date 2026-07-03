// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Factories/PoolFactory_Actor.h"

#include "BmrPoolFactory_Pawn.generated.h"

/**
 * Handles Pawn-inherited objects with project-specific pooling behavior.
 */
UCLASS()
class BOMBER_API UBmrPoolFactory_Pawn : public UPoolFactory_Actor
{
	GENERATED_BODY()

public:
	/** Is overridden to handle Pawn-inherited classes */
	virtual const UClass* GetObjectClass_Implementation() const override;

	/** Is overridden to prevent SetActorTransform call that causes Mover simulation desyncs and location disagreements */
	virtual void OnTakeFromPool_Implementation(UObject* Object, const FTakeFromPoolPayload& Payload) override;

	/** Is overridden to reset transform to the actor before returning the object to its pool. */
	virtual void OnReturnToPool_Implementation(UObject* Object) override;
};