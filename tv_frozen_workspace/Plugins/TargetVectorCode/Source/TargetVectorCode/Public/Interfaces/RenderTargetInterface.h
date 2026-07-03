#pragma once

#include "Math/Rotator.h"
#include "RenderTargetInterface.generated.h"

UINTERFACE(Blueprintable)
class URenderTargetInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IRenderTargetInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Render Target")
	void AddRotation(FRotator Rotation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Render Target")
	void AddZoom(float Zoom);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Render Target")
	void SetAutoRotate(bool AutoRotate);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Render Target")
	void SetAutoRotateVelocity(float Velocity);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Render Target")
	void Reset();

};