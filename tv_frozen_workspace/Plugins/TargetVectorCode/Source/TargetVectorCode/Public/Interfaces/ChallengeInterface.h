#pragma once

#include "ChallengeInterface.generated.h"

UINTERFACE(Blueprintable)
class UChallengeInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IChallengeInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Challenges")
  bool IsTrue();

};