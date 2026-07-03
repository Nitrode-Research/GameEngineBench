#pragma once

#include "InteractionInterface.generated.h"

UINTERFACE(Blueprintable)
class UInteractionInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IInteractionInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
  bool IsTrue();

};