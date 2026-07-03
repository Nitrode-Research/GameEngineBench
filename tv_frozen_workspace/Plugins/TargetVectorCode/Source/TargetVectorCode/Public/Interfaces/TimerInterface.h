#pragma once

#include "TimerInterface.generated.h"

UINTERFACE(Blueprintable)
class UTimerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ITimerInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Timers")
  bool IsTimers();

};