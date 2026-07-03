#include "SpudRoamingActorSubsystem.h"

#include "SpudSubsystem.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

// Console variable to toggle debug drawing of WP cell cache bounds at runtime
// Usage: RoamingActorSubsystem.DebugDrawCells 1
static TAutoConsoleVariable<bool> CVarDebugDrawCells(
    TEXT("RoamingActorSubsystem.DebugDrawCells"),
    false,
    TEXT("Draw debug boxes for WP cell cache")
);

void USpudRoamingActorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USpudRoamingActorSubsystem::Deinitialize()
{
    CachedSpudSubsystem = nullptr;
    TrackedActors.Empty();
    CellCache.Empty();

    Super::Deinitialize();
}

// Only create this subsystem in game worlds
bool USpudRoamingActorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) return false;

    // Only create on server / standalone
    return World->GetNetMode() != NM_Client;
}

void USpudRoamingActorSubsystem::RegisterActor(AActor* Actor)
{
}

void USpudRoamingActorSubsystem::UnregisterActor(AActor* Actor)
{
}

// Called by SPUD when it is about to save a specific streaming level.
// We store each tracked actor into whichever cell it physically occupies,
// falling back to LastValidCellName if no cell is found at the current location.
void USpudRoamingActorSubsystem::OnLevelStore(const FString& LevelName)
{
}

void USpudRoamingActorSubsystem::OnPreUnloadStreamingLevel(const FName& LevelName)
{
}

void USpudRoamingActorSubsystem::OnPostUnloadStreamingLevel(const FName& LevelName)
{
}

void USpudRoamingActorSubsystem::OnPostLoadStreamingLevel(const FName& LevelName)
{
}

// Keeps the cell state cache in sync with the current WP streaming state.
// If the number of valid cells has changed, performs a full rebuild.
// Otherwise just refreshes the State field of each cached entry.
void USpudRoamingActorSubsystem::OnStreamingStateUpdated()
{
}

// Rebuilds the cell cache from scratch by iterating all WP streaming cells.
// Cells without valid content bounds are skipped
void USpudRoamingActorSubsystem::RebuildCellCache()
{
    CellCache.Empty();
}

// Finds the most specific WP cell that contains the given location.
// "Most specific" = smallest XY area (e.g. a house cell inside a landscape cell).
bool USpudRoamingActorSubsystem::FindCellForLocation(
    const FVector& Location,
    FString& OutCellName,
    bool& OutIsActivated) const
{
    OutCellName.Reset();
    OutIsActivated = false;
    return false;
}

// Clamps the actor's location to within the bounds of the given cell (with a small inset).
// This ensures the actor restores strictly inside its cell and doesn't immediately
// trigger the unload logic due to a boundary position.
void USpudRoamingActorSubsystem::ClampActorToCell(AActor* Actor, const FString& CellName) const
{
}

// Clamps the actor into its target cell, stores it in SPUD, then queues it for destruction.
// Actual Destroy() is deferred to after the Tick loop to avoid invalidating iterators.
void USpudRoamingActorSubsystem::SaveAndDestroyActor(
    FTrackedActor& Tracked,
    const FString& CellName,
    USpudSubsystem* Spud,
    TArray<AActor*>& OutActorsToDestroy)
{
}

void USpudRoamingActorSubsystem::Tick(float DeltaTime)
{
}
