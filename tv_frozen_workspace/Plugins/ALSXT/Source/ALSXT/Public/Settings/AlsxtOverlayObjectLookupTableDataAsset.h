#pragma once

#include "GameplayTagContainer.h"

#include "AlsxtOverlayObjectLookupTableDataAsset.generated.h"

class UAlsxtOverlayDataAsset;

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtOverlayObjectLookupTableDataAsset: public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Als.OverlayMode"))
	TMap<FGameplayTag, TSoftObjectPtr<UAlsxtOverlayDataAsset> > OverlayObjectDataMap;
};