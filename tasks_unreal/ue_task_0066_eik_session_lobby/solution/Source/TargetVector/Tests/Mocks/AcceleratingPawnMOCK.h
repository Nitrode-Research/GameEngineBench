// MIT

#pragma once

#include "CoreMinimal.h"
#include "AcceleratingPawn.h"
#include "AcceleratingPawnMOCK.generated.h"

/**
 *
 */
UCLASS()
class TARGETVECTOR_API AAcceleratingPawnMOCK : public AAcceleratingPawn
{
	GENERATED_BODY()

public:

	bool isMeshTheRootComponent();

	bool hasGravityEnabled();

	bool isAffectingNavigation();

	bool hasAStaticMeshAssigned();
};
