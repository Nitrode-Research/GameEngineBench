// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).


#include "AbilitySystem/PlayerState/AlsxtPlayerState.h"
#include "AlsxtCharacter.h"
#include "AbilitySystem/AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "Components/Character/AlsxtCharacterCustomizationComponent.h"
#include "AbilitySystem/AttributeSets/AlsxtMovementAttributeSet.h"

/**
* @file AlsxtPlayerState.cpp
* @brief Base ALSXT Player State class. ASC and Gameplay Abilities/Effect are implemented here.
* AlsxtPlayerState is a template class that contains all shared Logic and Data for Player State Classes.
* Create a Blueprint class based on this class, do not use the C++ class directly in the Editor
*/

/**
* @brief Constructor for AlsxtPlayerState adding the Ability System Component
* @param ObjectInitializer The object initializer for constructing this object.
*/
AAlsxtPlayerState::AAlsxtPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// If the NetUpdateFrequency is too low, there will be a delay on Ability activation / Effect application on the client.
	SetNetUpdateFrequency(100.0f);

	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAlsxtAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AddOwnedComponent(AbilitySystemComponent);

	// Set Replication Mode to Mixed for Players.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	// AbilitySystemInitializationData = FAlsxtAbilitySystemInitializationData();

	// Create the Character Customization Component sub-object.
	CharacterCustomizationComponent = CreateDefaultSubobject<UAlsxtCharacterCustomizationComponent>(TEXT("Character Customization Component"));
	AddOwnedComponent(CharacterCustomizationComponent);
}

void AAlsxtPlayerState::BeginPlay()
{
	Super::BeginPlay();
	 
	// Provide this character as owner and avatar
	// AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
	InitializeAbilitySystem();
}

/**
* @brief Connect Interface Function to provide a pointer to the AlsxtCharacterMovementComponent
*/
UAlsxtCharacterMovementComponent* AAlsxtPlayerState::GetAlsxtCharacterMovementComponent() const
{
	if (Cast<AAlsxtCharacter>(GetPawn()))
	{
		return Cast<UAlsxtCharacterMovementComponent>(Cast<AAlsxtCharacter>(GetPawn())->GetCharacterMovement());
	}
	return nullptr;
}

/**
* @brief Connect Interface Function to provide a pointer to the AbilitySystemComponent
*/
UAbilitySystemComponent* AAlsxtPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAlsxtPlayerState::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		// Shouldn't happen, but if it is, return an error.
		return;
	}
	
	// Call the function on "Custom Ability System Component" to set up references and Init data. (Client)
	AbilitySystemComponent->InitializeAbilitySystemData(AbilitySystemInitializationData.Get(), this, GetPawn());

	// AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAlsxtMovementAttributeSet::GetMovementSpeedMultiplierAttribute()).AddUObject(this, &ThisClass::MovementSpeedMultiplierChanged);
	
	PostInitializeAbilitySystem();
}

void AAlsxtPlayerState::PostInitializeAbilitySystem_Implementation()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
}

void AAlsxtPlayerState::MovementSpeedMultiplierChanged(const FOnAttributeChangeData& OnAttributeChangeData) const
{
	UE_LOG(LogTemp, Warning, TEXT("%f"), OnAttributeChangeData.NewValue);
	GetAlsxtCharacterMovementComponent()->SetMovementSpeedMultiplier(OnAttributeChangeData.NewValue);
}