#pragma once

#include "BattleZoneInterface.generated.h"

UINTERFACE(Blueprintable)
class UBattleZoneInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IBattleZoneInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Battle Zone")
  bool IsTrue();

};