// Copyright (c) Valerii Rotermel and Yevhenii Selivanov

#include "LevelActors/PSStarActor.h"

// PS
#include "Components/PSSpotComponent.h"
#include "Data/PSDataAsset.h"
#include "Data/PSTypes.h"
#include "Data/PSWorldSubsystem.h"

// Bomber
#include "Actors/BmrPawn.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "DalRegistrySubsystem.h"
#include "DataRegistries/BmrBombRow.h"
#include "MyUtilsLibraries/GameplayUtilsLibrary.h"
#include "PoolManagerSubsystem.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PSStarActor)

// Sets default values
APSStarActor::APSStarActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	StarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
	RootComponent = StarMeshComponent;
}

// Called when the game starts or when spawned
void APSStarActor::BeginPlay()
{
	Super::BeginPlay();

	// Listen to hande when local character is ready
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);

	// Listen to handle input for each game state
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Called when this actor is explicitly being destroyed during gameplay or in the editor, not called during level streaming or gameplay ending
void APSStarActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	Super::EndPlay(EndPlayReason);
}

// Function called every frame on this Actor
void APSStarActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// play only if cinematic is started
	// else play menu animation
	if (StartTimeHideStarsInternal)
	{
		TryPlayHideStarAnimation();
	}
	else if (StartTimeMenuStarsInternal)
	{
		TryPlayMenuStarAnimation();
	}
}

// When a local character load finished
void APSStarActor::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	const ABmrPawn* Character = Cast<ABmrPawn>(Payload.Instigator);
	ABmrPlayerController* LocalPC = Character ? Character->GetController<ABmrPlayerController>() : nullptr;
	if (ensureMsgf(LocalPC, TEXT("ASSERT: [%i] %hs:\n'LocalPC' is null!"), __LINE__, __FUNCTION__))
	{
		LocalPC->OnAnyCinematicStarted.AddUniqueDynamic(this, &APSStarActor::OnAnyCinematicStarted);
	}
}

// Called when the current game state was changed
void APSStarActor::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::Menu))
	{
		SetStartTimeMenuStars();
		TryPlayMenuStarAnimation();
	}
	else
	{
		StartTimeMenuStarsInternal = 0.f;
	}
}

// Is called when any cinematic started
void APSStarActor::OnAnyCinematicStarted_Implementation(const UObject* LevelSequence, const UObject* FromInstigator)
{
	SetStartTimeHideStars();
	TryPlayHideStarAnimation();
}

// Hiding stars with animation in main menu when cinematic is start to play
void APSStarActor::TryPlayHideStarAnimation()
{
}

// Menu stars with animation in main menu idle
void APSStarActor::TryPlayMenuStarAnimation()
{
}

// Helper function that plays any given star animation from various places
bool APSStarActor::TryPlayStarAnimation(float& StartTimeRef, UCurveTable* AnimationCurveTable)
{
	return false;
}

// Set the start time for hiding stars
void APSStarActor::SetStartTimeHideStars()
{
	const UWorld* World = GetWorld();
	check(World);

	StartTimeHideStarsInternal = World->GetTimeSeconds();
}

// Set the start time for main menu stars animation
void APSStarActor::SetStartTimeMenuStars()
{
	const UWorld* World = GetWorld();
	check(World);

	StartTimeMenuStarsInternal = World->GetTimeSeconds();
}

//  Is get called when a Star actor is initialized
void APSStarActor::OnInitialized(const FVector& PreviousActorLocation)
{
}

//  Updates star actors Mesh material to the Locked Star, Unlocked or partially achieved
void APSStarActor::UpdateStarActorProgressMeshMaterial(float AmountOfStars, EPSStarActorState StarActorState)
{
}

// Applies the star dynamic material
void APSStarActor::SetStarActorProgressMeshMaterial(class UMaterialInstanceDynamic* StarDynamicMaterial, float StarProgressionAmount)
{
}

// Changes current bomb mesh to current spot bomb mesh
void APSStarActor::ChangeStarMesh(const UPSSpotComponent* SpotComponent)
{
	if (!ensureMsgf(SpotComponent, TEXT("ASSERT: [%i] %hs:\n'SpotComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return; // Early return if pointers are invalid
	}

	const FBmrPlayerTag& PlayerTag = SpotComponent->GetMeshChecked().GetPlayerTag();
	const FName BombRowName = FBmrBombRow::GetRowNameByPredicate([&PlayerTag](const FBmrBombRow& Row)
	{
		return Row.PlayerTag == PlayerTag;
	});
	const FBmrBombRow* BombRow = !BombRowName.IsNone() ? FBmrBombRow::GetRowByName(BombRowName) : nullptr;
	UStaticMesh* BombMesh = BombRow ? Cast<UStaticMesh>(BombRow->Mesh.Get()) : nullptr;
	if (!BombMesh
	    && ensureMsgf(!BombRowName.IsNone(), TEXT("ASSERT: [%i] %hs:\n'BombMesh' is not valid for BombRowName=%s!"), __LINE__, __FUNCTION__, *BombRowName.ToString()))
	{
		// Soft mesh not loaded yet, wait until loaded
		UDalRegistrySubsystem::Get().ListenForDataRegistryRow<FBmrBombRow>(this, BombRowName, [this, WeakSpot = TWeakObjectPtr(SpotComponent)](const FBmrBombRow&)
		{
			if (const UPSSpotComponent* StillValidSpot = WeakSpot.Get())
			{
				ChangeStarMesh(StillValidSpot);
			}
		});
		return;
	}

	StarMeshComponent->SetMaterial(0, nullptr);
	StarMeshComponent->SetStaticMesh(BombMesh);
}
