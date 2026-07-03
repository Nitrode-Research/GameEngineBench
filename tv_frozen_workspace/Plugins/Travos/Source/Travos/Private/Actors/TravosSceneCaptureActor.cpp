// MIT


#include "Actors/TravosSceneCaptureActor.h"


// Sets default values
ATravosSceneCaptureActor::ATravosSceneCaptureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	RootComponent = SceneCaptureComponent;

	RVTComponent = CreateDefaultSubobject<URuntimeVirtualTextureComponent>(TEXT("RVTComponent"));
	RVTComponent->SetupAttachment(RootComponent);

	SceneCaptureComponent->bCaptureEveryFrame = false; // Capture on demand
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Look down

	// Configure RVT component (properties like bounds, material, etc., should be set at runtime)

}

void ATravosSceneCaptureActor::SetCaptureLocationAndBounds(const FVector& Location, const FVector& Extent)
{
	SetActorLocation(Location);
	SceneCaptureComponent->OrthoWidth = Extent.X * 2.0f; // Assuming square capture area
	// RVTComponent->SetWorldBounds(FBox(Location - Extent, Location + Extent));
	RVTComponent->Bounds = FBox(Location - Extent, Location + Extent);
	// RVTComponent->UpdateVirtualTextureContents(); // Update RVT after setting bounds
	RVTComponent->UpdateBounds();

	// Add actors to show only
	TArray<AActor*> ActorsToShow;
	// Populate ActorsToShow with relevant moving actors in this region
	SceneCaptureComponent->ShowOnlyActors = ActorsToShow;
	// SceneCaptureComponent->CaptureScene(); // Manually trigger capture when needed
}

// Called when the game starts or when spawned
void ATravosSceneCaptureActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATravosSceneCaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (TargetRVT)
	{
		// RVTComponent->SetRuntimeVirtualTexture(TargetRVT);
		RVTComponent->SetVirtualTexture(TargetRVT);
	}

}

