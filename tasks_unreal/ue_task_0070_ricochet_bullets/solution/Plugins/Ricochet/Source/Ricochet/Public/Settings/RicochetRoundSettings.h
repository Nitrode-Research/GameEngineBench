#pragma once

#include "Engine/DataAsset.h"
#include "RicochetRoundSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class RICOCHET_API URicochetRoundSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	// A curve table or data asset is best for data-driven design
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere", Meta = (AllowPrivateAccess)) 
	class UCurveFloat* MachDragCurve;	

};