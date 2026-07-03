#pragma once

#include "MultimediaInterface.generated.h"

UINTERFACE(Blueprintable)
class UMultimediaInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IMultimediaInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Multimedia")
  bool IsTrue();

};