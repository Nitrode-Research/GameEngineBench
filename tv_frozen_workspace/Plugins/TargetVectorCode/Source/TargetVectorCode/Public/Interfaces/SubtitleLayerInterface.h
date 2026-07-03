#pragma once

#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Utility/TVStructs.h"
#include "SubtitleLayerInterface.generated.h"

UINTERFACE(Blueprintable)
class USubtitleLayerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ISubtitleLayerInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Layer")
  void GetMostEligibleStack(UCommonActivatableWidgetStack*& Stack);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Layer")
  void FindSubtitleStack(FSubtitleEntry Subtitle, UCommonActivatableWidgetStack*& Stack);
};