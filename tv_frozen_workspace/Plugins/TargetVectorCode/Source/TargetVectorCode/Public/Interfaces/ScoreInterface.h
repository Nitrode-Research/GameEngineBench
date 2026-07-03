#pragma once

#include "ScoreInterface.generated.h"

UINTERFACE(Blueprintable)
class UScoreInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IScoreInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Score")
  bool IsTrue();

};