#pragma once

#include "LocationControlInterface.generated.h"

UINTERFACE(Blueprintable)
class ULocationControlInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ILocationControlInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Location Control")
  bool IsTrue();

};