#pragma once

#include "PlayerMarkerInterface.generated.h"

UINTERFACE(Blueprintable)
class UPlayerMarkerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IPlayerMarkerInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player Markers")
  float GetMarkers();

};