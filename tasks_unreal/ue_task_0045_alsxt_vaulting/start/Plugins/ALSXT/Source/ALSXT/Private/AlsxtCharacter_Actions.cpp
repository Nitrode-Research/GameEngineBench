#include "AlsCharacter.h"

#include "AlsAnimationInstance.h"
#include "AlsCharacterMovementComponent.h"
#include "AlsxtCharacter.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/Core/PushModel/PushModel.h"
#include "RootMotionSources/AlsRootMotionSource_Mantling.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsxtCharacterSettings.h"
#include "Utility/AlsConstants.h"
#include "Utility/AlsMacros.h"
#include "Utility/AlsMath.h"
#include "Utility/AlsxtGameplayTags.h"
#include "Utility/AlsUtility.h"
#include "Utility/AlsDebugUtility.h"
#include "Utility/AlsVector.h"
#include "Utility/AlsRotation.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RootMotionSources/AlsxtRootMotionSource_Vaulting.h"

void AAlsxtCharacter::TryStartSliding(const float PlayRate)
{
	if (GetLocomotionMode() == AlsLocomotionModeTags::Grounded)
	{
		StartSliding(PlayRate, ALSXTSettings->Sliding.bRotateToInputOnStart && GetLocomotionState().bHasInput
			? GetLocomotionState().InputYawAngle
			: UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(GetActorRotation().Yaw)));
	}
}

bool AAlsxtCharacter::IsSlidingAllowedToStart(const UAnimMontage* Montage) const
{
	return !GetLocomotionAction().IsValid() ||
		// ReSharper disable once CppRedundantParentheses
		(GetLocomotionAction() == AlsLocomotionActionTags::Sliding &&
			!GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage));
}

void AAlsxtCharacter::StartSliding(const float PlayRate, const float TargetYawAngle)
{
	if (GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	auto* Montage{ SelectSlideMontage() };

	if (!ALS_ENSURE(IsValid(Montage)) || !IsSlidingAllowedToStart(Montage))
	{
		return;
	}

	const auto StartYawAngle{ UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(GetActorRotation().Yaw)) };

	if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastStartSliding(Montage, PlayRate, StartYawAngle, TargetYawAngle);
		// OnSlidingStarted();
	}
	else
	{
		GetCharacterMovement()->FlushServerMoves();

		StartSlidingImplementation(Montage, PlayRate, StartYawAngle, TargetYawAngle);
		ServerStartSliding(Montage, PlayRate, StartYawAngle, TargetYawAngle);
		// OnSlidingStarted();
	}
}

UAnimMontage* AAlsxtCharacter::SelectSlideMontage_Implementation()
{
	return ALSXTSettings->Sliding.Montage;
}

void AAlsxtCharacter::ServerStartSliding_Implementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	if (IsSlidingAllowedToStart(Montage))
	{
		MulticastStartSliding(Montage, PlayRate, StartYawAngle, TargetYawAngle);
		ForceNetUpdate();
	}
}

void AAlsxtCharacter::MulticastStartSliding_Implementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	StartSlidingImplementation(Montage, PlayRate, StartYawAngle, TargetYawAngle);
}

void AAlsxtCharacter::StartSlidingImplementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	FAlsxtSlidingState NewSlidingState;
	NewSlidingState.Montage = Montage;
	SetSlidingState(NewSlidingState);
	
	if (IsSlidingAllowedToStart(Montage) && GetMesh()->GetAnimInstance()->Montage_Play(Montage, PlayRate))
	{
		SlidingState.TargetYawAngle = TargetYawAngle;

		SetRotationInstant(StartYawAngle);

		SetLocomotionAction(AlsLocomotionActionTags::Sliding);
		OnSlidingStarted();
		LaunchCharacter(GetActorForwardVector() * 1000, false, false);
		// GetMesh()->AddImpulse(GetActorForwardVector() * 100);
		// Crouch(); //Hack

		if (GetMesh()->GetAnimInstance())
		{
			OnSlidingStartedBlendOutDelegate.BindUObject(this, &AAlsxtCharacter::OnSlidingStartedBlendOut);
			GetMesh()->GetAnimInstance()->Montage_SetBlendingOutDelegate(OnSlidingStartedBlendOutDelegate);
		}
	}
}

void AAlsxtCharacter::RefreshSliding(const float DeltaTime)
{
	if (GetLocalRole() <= ROLE_SimulatedProxy ||
		GetMesh()->GetAnimInstance()->RootMotionMode <= ERootMotionMode::IgnoreRootMotion)
	{
		// Refresh Sliding physics here because AALSXTCharacter::PhysicsRotation()
		// won't be called on simulated proxies or with ignored root motion.

		RefreshSlidingPhysics(DeltaTime);
	}
}

void AAlsxtCharacter::RefreshSlidingPhysics(const float DeltaTime)
{
	if (GetLocomotionAction() != AlsLocomotionActionTags::Sliding)
	{
		return;
	}

	if (GetVelocity().Length() <= GetCharacterMovementComponent()->MaxWalkSpeed)
	{
		StopSliding();
		return;
	}

	auto TargetRotation{ GetCharacterMovement()->UpdatedComponent->GetComponentRotation() };

	if (ALSXTSettings->Sliding.RotationInterpolationSpeed <= 0.0f)
	{
		TargetRotation.Yaw = SlidingState.TargetYawAngle;

		GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		TargetRotation.Yaw = UAlsRotation::DamperExactAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(TargetRotation.Yaw)),
			SlidingState.TargetYawAngle, DeltaTime,
			ALSXTSettings->Sliding.RotationInterpolationSpeed);

		GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false);
	}
}

void AAlsxtCharacter::StopSliding()
{
	GetMesh()->GetAnimInstance()->Montage_JumpToSection("Exit", GetSlidingState().Montage);
}

bool AAlsxtCharacter::TryStartVaultingGrounded()
{
	return false;
}

bool AAlsxtCharacter::TryStartVaultingInAir()
{
	return false;
}

bool AAlsxtCharacter::IsVaultingAllowedToStart_Implementation(FVaultAnimation VaultAnimation) const
{
	// return !LocomotionAction.IsValid();

	return !GetLocomotionAction().IsValid() ||
		// ReSharper disable once CppRedundantParentheses
		(GetLocomotionAction() == AlsLocomotionActionTags::Vaulting &&
			!GetMesh()->GetAnimInstance()->Montage_IsPlaying(VaultAnimation.Montage.Montage));
}

bool AAlsxtCharacter::TryStartVaulting(const FAlsxtVaultingTraceSettings& TraceSettings)
{
	return false;
}

void AAlsxtCharacter::ServerStartVaulting_Implementation(const FAlsxtVaultingParameters& Parameters)
{
}

void AAlsxtCharacter::MulticastStartVaulting_Implementation(const FAlsxtVaultingParameters& Parameters)
{
}

void AAlsxtCharacter::StartVaultingImplementation(const FAlsxtVaultingParameters& Parameters)
{
}

UAlsxtMantlingSettings* AAlsxtCharacter::SelectMantlingSettingsXT_Implementation()
{
	return nullptr;
}

UAlsxtSlidingSettings* AAlsxtCharacter::SelectSlidingSettings_Implementation()
{
	return nullptr;
}

UAlsxtVaultingSettings* AAlsxtCharacter::SelectVaultingSettings_Implementation(const FGameplayTag& VaultingType)
{
	return nullptr;
}

FVaultAnimation AAlsxtCharacter::SelectVaultingMontage_Implementation(const FGameplayTag& CurrentGait, const FGameplayTag& VaultingType)
{
	TArray<FVaultAnimation> VaultingAnimations = SelectVaultingSettings(VaultingType)->VaultAnimations;
	TArray<FGameplayTag> TagsArray = { CurrentGait, VaultingType };
	FGameplayTagContainer TagsContainer = FGameplayTagContainer::CreateFromArray(TagsArray);
	TArray<FVaultAnimation> FilteredVaultingAnimations;
	FVaultAnimation SelectedVaultingAnimation;

	// Return is there are no sounds
	if (VaultingAnimations.Num() < 1 || !VaultingAnimations[0].Montage.Montage)
	{
		return SelectedVaultingAnimation;
	}

	// Filter sounds based on Tag parameters
	for (auto VaultingAnimation : VaultingAnimations)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(VaultingAnimation.Gait);
		CurrentTagsContainer.AppendTags(VaultingAnimation.VaultType);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredVaultingAnimations.Add(VaultingAnimation);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredVaultingAnimations.Num() < 1 || !FilteredVaultingAnimations[0].Montage.Montage)
	{
		return SelectedVaultingAnimation;
	}

	// If more than one result, avoid duplicates
	if (FilteredVaultingAnimations.Num() > 1)
	{
		//Shuffle Array
		for (int m = FilteredVaultingAnimations.Max(); m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredVaultingAnimations.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = rand() % FilteredVaultingAnimations.Max();
		SelectedVaultingAnimation = FilteredVaultingAnimations[RandIndex];
		return SelectedVaultingAnimation;
	}
	else
	{
		SelectedVaultingAnimation = FilteredVaultingAnimations[0];
		return SelectedVaultingAnimation;
	}

	return SelectedVaultingAnimation;
}

void AAlsxtCharacter::OnVaultingStarted_Implementation(const FAlsxtVaultingParameters& Parameters) 
{
	FALSXTCharacterVoiceParameters CharacterVoiceParams = IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(this);

	if (Parameters.VaultingType == ALSXTVaultTypeTags::Low)
	{
		ClearOverlayObject();
	}

	CharacterSound->PlayActionSound(true, true, true, ALSXTCharacterMovementSoundTags::Vaulting, CharacterVoiceParams.Sex, CharacterVoiceParams.Variant, IAlsxtCharacterInterface::Execute_GetCharacterOverlayMode(this), ALSXTActionStrengthTags::Medium, IAlsxtCharacterInterface::Execute_GetStamina(this));
}

void AAlsxtCharacter::RefreshVaulting()
{
	StopVaulting();
}

void AAlsxtCharacter::StopVaulting()
{
	VaultingRootMotionSourceId = 0;
	AlsCharacterMovement->SetMovementModeLocked(false);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	OnVaultingEnded();
}

void AAlsxtCharacter::OnVaultingEnded_Implementation() {}
