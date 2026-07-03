#pragma once

#include "AIInterface.generated.h"

UINTERFACE(Blueprintable)
class UAIInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IAIInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
  bool IsTrue();

};