#pragma once

#include "MarkerInterface.generated.h"

UINTERFACE(Blueprintable)
class UMarkerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IMarkerInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Markers")
  bool IsTrue();

};