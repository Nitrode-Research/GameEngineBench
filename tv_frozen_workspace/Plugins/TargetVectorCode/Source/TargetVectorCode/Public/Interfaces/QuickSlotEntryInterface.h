#pragma once

#include "QuickSlotEntryInterface.generated.h"

UINTERFACE(Blueprintable)
class UQuickSlotEntryInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IQuickSlotEntryInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Quick Slot Entry")
	void DisplayQuickSlotEntry(bool Persistent);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Quick Slot Entry")
	void FadeOutQuickSlotEntry(float InitialDelay, float FadeOutDuration);

};