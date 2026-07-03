#pragma once

#include "TicketInterface.generated.h"

UINTERFACE(Blueprintable)
class UTicketInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ITicketInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Tickets")
  bool IsTickets();

};