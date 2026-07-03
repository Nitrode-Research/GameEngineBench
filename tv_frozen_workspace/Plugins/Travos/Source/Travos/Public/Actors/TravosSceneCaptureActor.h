// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "TravosSceneCaptureActor.generated.h"

UCLASS()
class TRAVOS_API ATravosSceneCaptureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATravosSceneCaptureActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene Capture")
	USceneCaptureComponent2D* SceneCaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene Capture")
	URuntimeVirtualTextureComponent* RVTComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Capture")
	URuntimeVirtualTexture* TargetRVT;

	void SetCaptureLocationAndBounds(const FVector& Location, const FVector& Extent);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
