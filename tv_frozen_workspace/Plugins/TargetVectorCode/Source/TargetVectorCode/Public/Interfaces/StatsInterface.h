#pragma once

#include "StatsInterface.generated.h"

UINTERFACE(Blueprintable)
class UStatsInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IStatsInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Stats")
  bool IsTrue();

};