#pragma once

#include "QuickSlotsInterface.generated.h"

UINTERFACE(Blueprintable)
class UQuickSlotsInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IQuickSlotsInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Quick Slots")
	void DisplayQuickSlots(bool Persistent, bool DisplayAllSlots);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Quick Slots")
	void GetQuickSlots(TArray<UCommonActivatableWidget*>& QuickSlots);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Quick Slots")
	void FadeOutQuickSlots(float InitialDelay, float FadeOutDuration);

};