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

	// if (GetOwner()->GetLocalRole() != ROLE_AutonomousProxy)
	// {
	// 	return;
	// }

	if (GeneralCameraEffectsSettings.bEnableEffects)
	{
		PlayerController = Cast<APlayerController>(GetOwner());
		if (IsValid(PlayerController))
		{
			CameraManager = PlayerController->PlayerCameraManager;
		}
		
		Character = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
		if (IsValid(Character))
		{
			Camera = IAlsxtControllerVFXInterface::Execute_GetCamera(GetOwner());
			// CameraManager = IALSXTControllerVFXInterface::Execute_GetPlayerCameraManager(GetOwner());
			FString PostProcessComponentName = "PostProcess Component";
			PostProcessComponent = NewObject<UPostProcessComponent>(this, UPostProcessComponent::StaticClass(), *PostProcessComponentName);

			if (PostProcessComponent != nullptr)
			{
				PostProcessComponent->AttachToComponent(CameraManager->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				PostProcessComponent->RegisterComponent();

				if (GeneralCameraEffectsSettings.bEnableDepthOfFieldEffect)
				{
					GetWorld()->GetTimerManager().SetTimer(DepthOfFieldTraceTimer, this, &UAlsxtPlayerViewportEffectsComponent::DepthOfFieldTrace, 0.01f, true);

					//Enable Parameters
					PostProcessComponent->Settings.bOverride_DepthOfFieldFocalDistance = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldFocalRegion = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldFarTransitionRegion = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldNearTransitionRegion = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldFstop = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldDepthBlurAmount = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldDepthBlurRadius = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldNearBlurSize = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldFarBlurSize = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldSkyFocusDistance = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldSqueezeFactor = true;
					PostProcessComponent->Settings.bOverride_DepthOfFieldVignetteSize = true;

					// Set some defaults
					PostProcessComponent->Settings.DepthOfFieldNearBlurSize = GeneralCameraEffectsSettings.DepthOfFieldNearBlurSize;
					PostProcessComponent->Settings.DepthOfFieldFarBlurSize = GeneralCameraEffectsSettings.DepthOfFieldFarBlurSize;
					PostProcessComponent->Settings.DepthOfFieldSkyFocusDistance = GeneralCameraEffectsSettings.MaxDOFTraceDistance + .001;
				}

				if (GeneralCameraEffectsSettings.bEnableRadialBlurEffect)
				{
					const FWeightedBlendable RadialBlurBlend{ 0.0f, GeneralCameraEffectsSettings.RadialBlurMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(RadialBlurBlend);
					RadialBlurEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
					GetWorld()->GetTimerManager().SetTimer(RadialBlurEffectTimer, this, &UAlsxtPlayerViewportEffectsComponent::SetRadialBlurEffect, 0.01f, true);
				}

				if (GeneralCameraEffectsSettings.bEnableDrunkEffect)
				{
					const FWeightedBlendable DrunkBlend{ 0.0f, GeneralCameraEffectsSettings.DrunkEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(DrunkBlend);
					DrunkEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
				}

				if (GeneralCameraEffectsSettings.bEnableHighEffect)
				{
					const FWeightedBlendable HighEffectBlend{ 0.0f, GeneralCameraEffectsSettings.HighEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(HighEffectBlend);
					HighEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "High Added");
				}

				if (GeneralCameraEffectsSettings.bEnableSuppressionEffect)
				{
					const FWeightedBlendable SuppressionBlend{ 0.0f, GeneralCameraEffectsSettings.SuppressionEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(SuppressionBlend);
					SuppressionEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
				}

				if (GeneralCameraEffectsSettings.bEnableBlindnessEffect)
				{
					const FWeightedBlendable BlindnessBlend{ 0.0f, GeneralCameraEffectsSettings.BlindnessEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(BlindnessBlend);
					BlindnessEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
				}

				if (GeneralCameraEffectsSettings.bEnableConcussionEffect)
				{
					const FWeightedBlendable ConcussionBlend{ 0.0f, GeneralCameraEffectsSettings.ConcussionEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(ConcussionBlend);
					ConcussionEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
				}

				if (GeneralCameraEffectsSettings.bEnableDamageEffect)
				{
					const FWeightedBlendable DamageBlend{ 0.0f, GeneralCameraEffectsSettings.DamageEffectMaterial };
					PostProcessComponent->Settings.WeightedBlendables.Array.Add(DamageBlend);
					DamageEffectBlendableIndex = PostProcessComponent->Settings.WeightedBlendables.Array.Num() - 1;
				}
			}
		}
	}
	
}

// Called every frame
void UAlsxtPlayerViewportEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAlsxtPlayerViewportEffectsComponent::DepthOfFieldTrace()
{
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	FVector CameraLocation = IAlsxtControllerVFXInterface::Execute_GetCameraLocation(Character);
	FRotator CameraRotation = IAlsxtControllerVFXInterface::Execute_GetCameraRotation(Character);
	FGameplayTag ViewMode = Character->GetViewMode();
	FGameplayTag CombatStance = Character->GetDesiredCombatStance();
	FVector ThirdPersonTraceStartPoint = Character->GetMesh()->GetSocketLocation("head");
	FVector FirstPersonTraceStartPoint = CameraLocation;
	FVector TraceStartPoint = Character->GetViewMode() == AlsViewModeTags::FirstPerson ? FirstPersonTraceStartPoint : ThirdPersonTraceStartPoint;
	FVector TraceEndPoint = TraceStartPoint + (CameraRotation.Vector() * GeneralCameraEffectsSettings.MaxDOFTraceDistance);
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Character);
	FHitResult OutHit;

	float HitDistance;

	if (GeneralCameraEffectsSettings.bFocusOnCombatTarget && IsValid(IAlsxtCombatInterface::Execute_GetCurrentTarget(Character)))
	{
		AActor* CurrentTarget = IAlsxtCombatInterface::Execute_GetCurrentTarget(Character);
		HitDistance = FVector::Dist(Character->GetActorLocation(), CurrentTarget->GetActorLocation());
	}
	else
	{
		bool Hit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TraceStartPoint, TraceEndPoint, 10.0f, GeneralCameraEffectsSettings.TraceType, false, ActorsToIgnore, EDrawDebugTrace::None, OutHit, true, FLinearColor::Blue, FLinearColor::Yellow, 0.01f);
		HitDistance = Hit ? OutHit.Distance : 0.0f;
	}

	float UnaimedFirstPersonFStop = GeneralCameraEffectsSettings.FirstPersonFocalDistanceToFStopCurve->GetVectorValue(HitDistance).X;
	float UnaimedThirdPersonFStop = GeneralCameraEffectsSettings.ThirdPersonFocalDistanceToFStopCurve->GetVectorValue(HitDistance).X;
	float AimedFirstPersonFStop = GeneralCameraEffectsSettings.FirstPersonFocalDistanceToFStopCurve->GetVectorValue(HitDistance).Y;
	float AimedThirdPersonFStop = GeneralCameraEffectsSettings.ThirdPersonFocalDistanceToFStopCurve->GetVectorValue(HitDistance).Y;
	FGameplayTag CurrentCombatStance = Character->GetDesiredCombatStance();
	float FirstPersonFStop = CurrentCombatStance == ALSXTCombatStanceTags::Aiming ? AimedFirstPersonFStop : UnaimedFirstPersonFStop;
	float ThirdPersonFStop = CurrentCombatStance == ALSXTCombatStanceTags::Aiming ? AimedThirdPersonFStop : UnaimedThirdPersonFStop;

	PostProcessComponent->Settings.DepthOfFieldFstop = Character->GetViewMode() == AlsViewModeTags::FirstPerson ? FirstPersonFStop : ThirdPersonFStop;
	PostProcessComponent->Settings.DepthOfFieldFocalDistance = HitDistance;
	PostProcessComponent->Settings.DepthOfFieldFocalRegion = HitDistance;
	// PostProcessComponent->Settings.DepthOfFieldFarTransitionRegion = HitDistance + 100.0f;
	// PostProcessComponent->Settings.DepthOfFieldNearTransitionRegion = FMath::Min(0.0, HitDistance);
}

void UAlsxtPlayerViewportEffectsComponent::SetRadialBlurEffect()
{
	float Velocity = Character->GetVelocity().Size();
	float BlurAmount = FMath::GetMappedRangeValueClamped(FVector2D{ 0.0, GeneralCameraEffectsSettings.RadialBlurMaxVelocity }, FVector2D{ 0.0f, GeneralCameraEffectsSettings.RadialBlurMaxWeight }, Velocity);
	PostProcessComponent->Settings.WeightedBlendables.Array[RadialBlurEffectBlendableIndex].Weight = BlurAmount;
}

// Drunk Effect
void UAlsxtPlayerViewportEffectsComponent::AddDrunkEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableDrunkEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(DrunkEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.DrunkEffectMaxWeight);
			BeginFadeOutDrunkEffect(1.0f, RecoveryDelay);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetDrunkEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutDrunkEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	DrunkEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(DrunkEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutDrunkEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutDrunkEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight - 0.001 * DrunkEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[DrunkEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(DrunkEffectFadeOutTimer);
		}
	}
}


// High Effect
void UAlsxtPlayerViewportEffectsComponent::AddHighEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableHighEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(HighEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.HighEffectMaxWeight);
			BeginFadeOutHighEffect(1.0f, RecoveryDelay);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::SanitizeFloat(PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "PP not valid");
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetHighEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutHighEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	HighEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(HighEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutHighEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutHighEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight - 0.001 * HighEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[HighEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(HighEffectFadeOutTimer);
		}
	}
}

// Suppression Effect
void UAlsxtPlayerViewportEffectsComponent::AddSuppressionEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableSuppressionEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(SuppressionEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.SuppressionEffectMaxWeight);
			BeginFadeOutSuppressionEffect(1.0f, RecoveryDelay);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetSuppressionEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutSuppressionEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	SuppressionEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(SuppressionEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutSuppressionEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutSuppressionEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight - 0.001 * SuppressionEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[SuppressionEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(SuppressionEffectFadeOutTimer);
		}
	}
}

// Blindness Effect
void UAlsxtPlayerViewportEffectsComponent::AddBlindnessEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableBlindnessEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(BlindnessEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.BlindnessEffectMaxWeight);
			BeginFadeOutBlindnessEffect(1.0f, RecoveryDelay);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetBlindnessEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutBlindnessEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	BlindnessEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(BlindnessEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutBlindnessEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutBlindnessEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight - 0.001 * BlindnessEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[BlindnessEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(BlindnessEffectFadeOutTimer);
		}
	}
}

// Concussion Effect
void UAlsxtPlayerViewportEffectsComponent::AddConcussionEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableConcussionEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(ConcussionEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.ConcussionEffectMaxWeight);
			BeginFadeOutConcussionEffect(1.0f, RecoveryDelay);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetConcussionEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutConcussionEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	ConcussionEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(ConcussionEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutConcussionEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutConcussionEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight - 0.001 * ConcussionEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[ConcussionEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(ConcussionEffectFadeOutTimer);
		}
	}
}

// Damage Effect
void UAlsxtPlayerViewportEffectsComponent::AddDamageEffect(float Amount, float RecoveryDelay)
{
	if (GeneralCameraEffectsSettings.bEnableDamageEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(DamageEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.DamageEffectMaxWeight);
			BeginFadeOutDamageEffect(1.0f, RecoveryDelay);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetDamageEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight = 0.0f;
}

void UAlsxtPlayerViewportEffectsComponent::BeginFadeOutDamageEffect(float NewRecoveryScale, float NewRecoveryDelay)
{
	DamageEffectRecoveryScale = NewRecoveryScale;
	GetWorld()->GetTimerManager().SetTimer(DamageEffectFadeOutTimer, this, &UAlsxtPlayerViewportEffectsComponent::FadeOutDamageEffect, 0.01f, true, NewRecoveryDelay);
}

void UAlsxtPlayerViewportEffectsComponent::FadeOutDamageEffect()
{
	if (IsValid(PostProcessComponent))
	{
		PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight - 0.001 * DamageEffectRecoveryScale), 0.0f, 1.0);

		if (PostProcessComponent->Settings.WeightedBlendables.Array[DamageEffectBlendableIndex].Weight <= 0.0)
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(DamageEffectFadeOutTimer);
		}
	}
}

// Death Effect
void UAlsxtPlayerViewportEffectsComponent::AddDeathEffect(float Amount)
{
	if (GeneralCameraEffectsSettings.bEnableDeathEffect)
	{
		if (IsValid(PostProcessComponent) && PostProcessComponent->Settings.WeightedBlendables.Array.IsValidIndex(DeathEffectBlendableIndex))
		{
			PostProcessComponent->Settings.WeightedBlendables.Array[DeathEffectBlendableIndex].Weight = FMath::Clamp((PostProcessComponent->Settings.WeightedBlendables.Array[DeathEffectBlendableIndex].Weight + Amount), 0.0, GeneralCameraEffectsSettings.DeathEffectMaxWeight);
		}
	}
}

void UAlsxtPlayerViewportEffectsComponent::ResetDeathEffect()
{
	PostProcessComponent->Settings.WeightedBlendables.Array[DeathEffectBlendableIndex].Weight = 0.0f;
}
