#pragma once

#include "Utility/TVStructs.h"
#include "UnitPFPLayerInterface.generated.h"

UINTERFACE(Blueprintable)
class UUnitPFPLayerInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IUnitPFPLayerInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Unit PFP")
	void GetImageLayer(FUnitProfilePictureImageLayer& ImageLayer);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Unit PFP")
	void GetPoseLayer(FUnitProfilePicturePoseLayer& PoseLayer);

};