#pragma once

#include "MissionInterface.generated.h"

UINTERFACE(Blueprintable)
class UMissionInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IMissionInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Missions")
  bool IsTrue();

};