#pragma once

#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Utility/AlsxtStructs.h"
#include "AlsxtElementalInteractionSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtElementalInteractionSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (TitleProperty = "{ElementalInteractions}", ForceInlineRow))
	TMap<TEnumAsByte<EPhysicalSurface>, FALSXTElementalInteractionSet> Interactions;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGeneralElementalInteractionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool EnableElementalInteractions{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DebugMode{ false };
};