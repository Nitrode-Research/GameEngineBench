#pragma once

#include "Utility/TVSpectatingStructs.h"
#include "SpectatingInterface.generated.h"

UINTERFACE(Blueprintable)
class USpectatingInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ISpectatingInterface {
	GENERATED_BODY()

public:
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectating")
  FSpectatingSettings GetSpectatingSettings();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectating")
  bool IsSpectatingEnabled();

};