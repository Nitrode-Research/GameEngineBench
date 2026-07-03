// MIT


#include "Components/PlayerController/AlsxtPlayerViewportEffectsComponent.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtControllerVFXInterface.h"
#include "Engine/Scene.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Curves/CurveVector.h"

// Sets default values for this component's properties
UAlsxtPlayerViewportEffectsComponent::UAlsxtPlayerViewportEffectsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtPlayerViewportEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerController = Cast<APlayerController>(GetOwner());
	Character = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
	PostProcessComponent = nullptr;
}

// Called every frame
void UAlsxtPlayerViewportEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAlsxtPlayerViewportEffectsComponent::DepthOfFieldTrace()
{
}

void UAlsxtPlayerViewportEffectsComponent::SetRadialBlurEffect()
{
}

// Drunk Effect
void UAlsxtPlayerViewportEffectsComponent::AddDrunkEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetDrunkEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutDrunkEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutDrunkEffect()
{
}


// High Effect
void UAlsxtPlayerViewportEffectsComponent::AddHighEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetHighEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutHighEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutHighEffect()
{
}

// Suppression Effect
void UAlsxtPlayerViewportEffectsComponent::AddSuppressionEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetSuppressionEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutSuppressionEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutSuppressionEffect()
{
}

// Blindness Effect
void UAlsxtPlayerViewportEffectsComponent::AddBlindnessEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetBlindnessEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutBlindnessEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutBlindnessEffect()
{
}

// Concussion Effect
void UAlsxtPlayerViewportEffectsComponent::AddConcussionEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetConcussionEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutConcussionEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutConcussionEffect()
{
}

// Damage Effect
void UAlsxtPlayerViewportEffectsComponent::AddDamageEffect(float Amount, float RecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetDamageEffect()
{
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutDamageEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutDamageEffect()
{
}

// Death Effect
void UAlsxtPlayerViewportEffectsComponent::AddDeathEffect(float Amount)
{
}

void UAlsxtPlayerViewportEffectsComponent::ResetDeathEffect()
{
}
