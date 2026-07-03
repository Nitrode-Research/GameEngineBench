#pragma once

#include "ServerLogInterface.generated.h"

UINTERFACE(Blueprintable)
class UServerLogInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IServerLogInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
  bool IsTrue();

};