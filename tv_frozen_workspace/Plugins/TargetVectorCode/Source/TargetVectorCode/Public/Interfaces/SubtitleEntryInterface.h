#pragma once

#include "Utility/TVStructs.h"
#include "Misc/DateTime.h"
#include "SubtitleEntryInterface.generated.h"

UINTERFACE(Blueprintable)
class USubtitleEntryInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ISubtitleEntryInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Entry")
  void GetSubtitle(FSubtitleEntry& Subtitle);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Entry")
  void GetTimestamp(FDateTime& Timestamp);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Entry")
  void HasMinimumDisplayDuration(bool& HasMinimumDisplayDuration);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Subtitle Entry")
  void GetDisplayDurationPercentage(float& Percentage);
};