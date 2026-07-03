// Copyright (c) Yevhenii Selivanov

#include "GameFramework/BmrGameState.h"

// Bomber
#include "Actors/BmrGeneratedMap.h"
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "DataAssets/BmrGameStateDataAsset.h"
#include "MyUtilsLibraries/GameplayUtilsLibrary.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// UE
#include "AbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrGameState)

ABmrGameState::ABmrGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GameStateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("GameStateTree"));
}

ABmrGameState& ABmrGameState::Get()
{
	ABmrGameState* MyGameState = UBmrBlueprintFunctionLibrary::GetGameState();
	checkf(MyGameState, TEXT("ERROR: [%i] %s:\n'MyGameState' is null!"), __LINE__, *FString(__FUNCTION__));
	return *MyGameState;
}

/*********************************************************************************************
 * Game State Tree
 ********************************************************************************************* */

bool ABmrGameState::CanStartGameStateTree() const
{
	return false;
}

void ABmrGameState::StartGameStateTree()
{
}

void ABmrGameState::StopGameStateTree()
{
}

void ABmrGameState::BindOnGameStateTagChanged()
{
}

void ABmrGameState::BroadcastGameStateChanged()
{
}

bool ABmrGameState::IsTransitionAllowed(const FGameplayTag& PriorTag, const FGameplayTag& NewTag) const
{
	return true;
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

UAbilitySystemComponent* ABmrGameState::GetAbilitySystemComponent() const
{
	return nullptr;
}

UAbilitySystemComponent& ABmrGameState::GetAbilitySystemComponentChecked() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	checkf(ASC, TEXT("'ASC' is null"));
	return *ASC;
}

void ABmrGameState::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
}

void ABmrGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABmrGameState::BeginPlay()
{
	Super::BeginPlay();
}

void ABmrGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ABmrGameState::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
}
