#pragma once

#include "ThreatLevelInterface.generated.h"

UINTERFACE(Blueprintable)
class UThreatLevelInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IThreatLevelInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Threat Level")
  bool IsTrue();

};