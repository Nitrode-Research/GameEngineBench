#pragma once

#include "CharacterInterface.generated.h"

UINTERFACE(Blueprintable)
class UCharacterInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ICharacterInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
  bool IsTrue();

};