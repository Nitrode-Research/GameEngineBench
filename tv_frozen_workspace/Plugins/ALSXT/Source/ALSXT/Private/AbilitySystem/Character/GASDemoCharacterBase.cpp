// MIT


#include "AbilitySystem/Character/GASDemoCharacterBase.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Character/GASDemoCharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystem/AbilitySystemComponent/AlsxtAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AlsxtMovementAttributeSet.h"


AGASDemoCharacterBase::AGASDemoCharacterBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer
.SetDefaultSubobjectClass<UGASDemoCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// If the NetUpdateFrequency is too low, there will be a delay on Ability activation / Effect application on the client.
	//SetNetUpdateFrequency(100.0f);
	
	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAlsxtAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UGASDemoCharacterMovementComponent* AGASDemoCharacterBase::GetGASDemoCharacterMovementComponent() const
{
	return Cast<UGASDemoCharacterMovementComponent>(GetCharacterMovement());
}

UAbilitySystemComponent* AGASDemoCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

UAlsxtAbilitySystemComponent* AGASDemoCharacterBase::GetALSXTAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void AGASDemoCharacterBase::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		// Shouldn't happen, but if it is, return an error.
		return;
	}
	
	// Call the function on "Custom Ability System Component" to set up references and Init data. (Client)
	// AbilitySystemComponent->InitializeAbilitySystemData(AbilitySystemInitializationData, this, this);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAlsxtMovementAttributeSet::GetMovementSpeedMultiplierAttribute()).AddUObject(this, &ThisClass::MovementSpeedMultiplierChanged);
	
	PostInitializeAbilitySystem();
}

void AGASDemoCharacterBase::PostInitializeAbilitySystem_Implementation()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
}

void AGASDemoCharacterBase::MovementSpeedMultiplierChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	UE_LOG(LogTemp, Warning, TEXT("%f"), OnAttributeChangeData.NewValue);
	GetGASDemoCharacterMovementComponent()->MovementSpeedMultiplier = OnAttributeChangeData.NewValue;
}

void AGASDemoCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AGASDemoCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilitySystem();
}