#pragma once

#include "Settings/AlsxtMeshRenderSettings.h"
#include "AlsxtControllerRenderInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtControllerRenderInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtControllerRenderInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Controller Render Interface")
	FAlsxtGeneralMeshRenderSettings GetMeshRenderSettings();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Controller Render Interface")
    bool IsSeparateFirstPersonFOVEnabled();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Controller Render Interface")
	bool IsViewModelOnSkeletalMeshEnabled();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Controller Render Interface")
	bool IsViewModelOnStaticMeshEnabled();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Controller Render Interface")
	bool IsViewModelBlendOnCharacterActionEnabled();
};