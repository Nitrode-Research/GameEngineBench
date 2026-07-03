#pragma once

#include "AlsxtTargetLockInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtTargetLockInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtTargetLockInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Target Lock")
  bool IsTrue();

};