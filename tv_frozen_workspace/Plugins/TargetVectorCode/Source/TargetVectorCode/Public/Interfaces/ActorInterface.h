#pragma once

#include "ActorInterface.generated.h"

UINTERFACE(Blueprintable)
class UActorInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IActorInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
  bool IsTrue();

};