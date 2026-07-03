// MIT


#include "Components/TravosGameStateComponent.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "VT/RuntimeVirtualTextureVolume.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "Subsystems/TravosWorldPartitionSubsystem.h"
#include "EngineUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/LevelStreaming.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"
#include "Streaming/LevelStreamingDelegates.h"

void UTravosGameStateComponent::OnLevelStreamingStateChanged(UWorld* World, const ULevelStreaming* LevelStreaming, ULevel* Level,
	ELevelStreamingState PreviousState, ELevelStreamingState NewState)
{
	if (NewState == ELevelStreamingState::LoadedVisible || NewState == ELevelStreamingState::MakingVisible)
	{
		OnCellLoaded(LevelStreaming);
	}
	else
	{
		OnCellUnloaded(LevelStreaming);
	}
}

// Sets default values for this component's properties
UTravosGameStateComponent::UTravosGameStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	FLevelStreamingDelegates::OnLevelStreamingStateChanged.AddUObject(this, &UTravosGameStateComponent::OnLevelStreamingStateChanged);
}

// Called when the game starts
void UTravosGameStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get the World Partition Subsystem
	WorldPartitionSubsystem = UWorld::GetSubsystem<UTravosWorldPartitionSubsystem>(GetWorld());
	if (WorldPartitionSubsystem)
	{
		// Bind to the loading and unloading delegates.
		// WorldPartitionSubsystem->OnCellLoaded().AddUObject(this, &UTravosGameStateComponent::OnCellLoaded);
		// WorldPartitionSubsystem->OnCellUnloaded().AddUObject(this, &UTravosGameStateComponent::OnCellUnloaded);
	}
    
	// Find the landscape in the world
	for (TActorIterator<ALandscape> It(GetWorld()); It; ++It)
	{
		WorldLandscape = *It;
		break;
	}
    
	// Create the master render target to combine captures
	MasterRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	// Configure MasterRenderTarget properties
	
}

void UTravosGameStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WorldPartitionSubsystem)
	{
		// Unbind the delegates to prevent crashes
		// WorldPartitionSubsystem->OnCellLoaded().RemoveAll(this);
		// WorldPartitionSubsystem->OnCellUnloaded().RemoveAll(this);
	}

	// Clean up all spawned components
	for (auto& Elem : ActiveCaptureComponents)
	{
		if (Elem.Value.CaptureComponent)
		{
			Elem.Value.CaptureComponent->DestroyComponent();
		}
		if (Elem.Value.RenderTarget)
		{
			Elem.Value.RenderTarget = nullptr;
			Elem.Value.RenderTarget->MarkAsGarbage();
		}
		if (Elem.Value.RVTVolume)
		{
			Elem.Value.RVTVolume->Destroy();
		}
	}
	ActiveCaptureComponents.Empty();

	Super::EndPlay(EndPlayReason);
}

void UTravosGameStateComponent::OnCellLoaded(const ULevelStreaming* LevelStreaming)
{
	if (LevelStreaming)
	{
		FString CellKey = FString::Printf(TEXT("%s_%s"), *LevelStreaming->GetName(), *LevelStreaming->GetWorldPartitionCell()->GetDebugName());
		if (ActiveCaptureComponents.Contains(CellKey))
		{
			CreateCaptureForCell(LevelStreaming);
		}
	}
}

void UTravosGameStateComponent::OnCellUnloaded(const ULevelStreaming* LevelStreaming)
{
	if (LevelStreaming)
	{
		
		FString CellKey = FString::Printf(TEXT("%s_%s"), *LevelStreaming->GetName(), *LevelStreaming->GetWorldPartitionCell()->GetDebugName());
		if (ActiveCaptureComponents.Contains(CellKey))
		{
			DestroyCaptureForCell(LevelStreaming);
		}
	}
}

void UTravosGameStateComponent::CreateCaptureForCell(const ULevelStreaming* LevelStreaming)
{
    if (!WorldLandscape) return;

    FBox CellBounds = const_cast<ULevelStreaming*>(LevelStreaming)->GetStreamingVolumeBounds();	
    FVector CellCenter = CellBounds.GetCenter();

    // Spawn a SceneCaptureComponent
    USceneCaptureComponent2D* NewCaptureComponent = NewObject<USceneCaptureComponent2D>(this);
    if (NewCaptureComponent)
    {
        NewCaptureComponent->RegisterComponent();
        NewCaptureComponent->SetWorldLocationAndRotation(CellCenter, FRotator(-90.f, 0.f, 0.f)); // Top-down view
        NewCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
        NewCaptureComponent->OrthoWidth = FMath::Max(CellBounds.GetExtent().X, CellBounds.GetExtent().Y) * 2.0f;
        NewCaptureComponent->bCaptureEveryFrame = true;
        NewCaptureComponent->bCaptureOnMovement = false;
        // NewCaptureComponent->ShowOnlyActors.Add(ACharacter::StaticClass()); // Example: capture only characters

        // Create a render target for this cell's capture
        UTextureRenderTarget2D* NewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
        NewRenderTarget->InitAutoFormat(512, 512); // Adjust size as needed
        NewCaptureComponent->TextureTarget = NewRenderTarget;

        // Create and configure a Runtime Virtual Texture Volume
        ARuntimeVirtualTextureVolume* NewRVTVolume = GetWorld()->SpawnActor<ARuntimeVirtualTextureVolume>(CellCenter, FRotator::ZeroRotator);
        URuntimeVirtualTextureComponent* RVTComponent = NewRVTVolume->VirtualTextureComponent;
    	
        if (RVTComponent)
        {
             // TODO: Create a URuntimeVirtualTexture asset and assign it to RVTComponent->VirtualTexture
             // TODO: Configure RVTComponent properties (bounds, etc.)
        }

        // Store the new components
        FString CellKey = FString::Printf(TEXT("%s_%s"), *LevelStreaming->GetName(), *LevelStreaming->GetWorldPartitionCell()->GetDebugName());
        ActiveCaptureComponents.Add(CellKey, FCaptureComponentInfo());
        ActiveCaptureComponents[CellKey].CaptureComponent = NewCaptureComponent;
        ActiveCaptureComponents[CellKey].RenderTarget = NewRenderTarget;
        ActiveCaptureComponents[CellKey].RVTVolume = NewRVTVolume;
    }
}

void UTravosGameStateComponent::DestroyCaptureForCell(const ULevelStreaming* LevelStreaming)
{
    FString CellKey = FString::Printf(TEXT("%s_%s"), *LevelStreaming->GetName(), *LevelStreaming->GetWorldPartitionCell()->GetDebugName());
    if (FCaptureComponentInfo* FoundInfo = ActiveCaptureComponents.Find(CellKey))
    {
        if (FoundInfo->CaptureComponent)
        {
            FoundInfo->CaptureComponent->DestroyComponent();
        }
        if (FoundInfo->RenderTarget)
        {
            FoundInfo->RenderTarget = nullptr;
            FoundInfo->RenderTarget->MarkAsGarbage();
        }
        if (FoundInfo->RVTVolume)
        {
            FoundInfo->RVTVolume->Destroy();
        }
        ActiveCaptureComponents.Remove(CellKey);
    }
}

// TODO: Implement RVT combination and material logic
void UTravosGameStateComponent::CombineRuntimeVirtualTextures()
{
    // This requires a custom material that samples each RVT based on landscape UVs.
    // An alternative is to manually draw the RVT textures onto a single master render target.
}

// Called every frame
void UTravosGameStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
