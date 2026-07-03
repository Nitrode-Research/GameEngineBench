// MIT

#include "Components/Character/AlsxtCharacterCameraEffectsComponent.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtCharacterCameraEffectsComponentInterface.h"
#include "Engine/Scene.h"
#include "Kismet/GameplayStatics.h"
#include "Curves/CurveVector.h"
#include "Math/UnrealMathUtility.h"

// Sets default values for this component's properties
UAlsxtCharacterCameraEffectsComponent::UAlsxtCharacterCameraEffectsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtCharacterCameraEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<AAlsxtCharacter>(GetOwner());
	PostProcessComponent = nullptr;
}


// Called every frame
void UAlsxtCharacterCameraEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAlsxtCharacterCameraEffectsComponent::Initialize()
{
}

void UAlsxtCharacterCameraEffectsComponent::UpdateCameraShake()
{
}

void UAlsxtCharacterCameraEffectsComponent::SetRadialBlur(float Amount)
{
	CurrentRadialBlurAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetRadialBlur()
{
	CurrentRadialBlurAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::SetFocusEffect(bool NewFocus)
{
}

void UAlsxtCharacterCameraEffectsComponent::CameraEffectsTrace()
{
}

void UAlsxtCharacterCameraEffectsComponent::AddSuppression(float NewMagnitude, float RecoveryDelay)
{
	CurrentSuppressionAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetSuppression()
{
	CurrentSuppressionAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutSuppression(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutSuppression()
{
	CurrentSuppressionAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::AddBlindnessEffect(float NewMagnitude, float RecoveryDelay)
{
	CurrentBlindnessEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetBlindnessEffect()
{
	CurrentBlindnessEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutBlindnessEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutBlindnessEffect()
{
	CurrentBlindnessEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::AddDamageEffect(float NewMagnitude, float RecoveryDelay)
{
	CurrentDamageEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetDamageEffect()
{
	CurrentDamageEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutDamageEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutDamageEffect()
{
	CurrentDamageEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::AddConcussionEffect(float NewMagnitude, float RecoveryDelay)
{
	CurrentConcussionEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetConcussionEffect()
{
	CurrentConcussionEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutConcussionEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutConcussionEffect()
{
	CurrentConcussionEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::AddDrunkEffect(float NewMagnitude, float RecoveryDelay)
{
	CurrentDrunkEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetDrunkEffect()
{
	CurrentDrunkEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutDrunkEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutDrunkEffect()
{
	CurrentDrunkEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::AddHighEffect(float NewMagnitude, float RecoveryDelay)
{
	CurrentHighEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::ResetHighEffect()
{
	CurrentHighEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::BeginFadeOutHighEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
}

void UAlsxtCharacterCameraEffectsComponent::FadeOutHighEffect()
{
	CurrentHighEffectAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::UpdateRadialBlur()
{
	CurrentRadialBlurAmount = 0.0f;
}

void UAlsxtCharacterCameraEffectsComponent::SetDepthOfField()
{

}
