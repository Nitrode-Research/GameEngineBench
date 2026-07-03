// MIT


#include "AlsxtCameraAnimationInstance.h"
#include "AlsCameraComponent.h"
#include "AlsCharacter.h"
#include "AlsxtCharacter.h"

void UAlsxtCameraAnimationInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	// Call RepeatingFunction once per second, starting two seconds from now.
	GetWorld()->GetTimerManager().SetTimer(FirstPersonOverrideHandle, this, &UAlsxtCameraAnimationInstance::OnFirstPersonOverrideChangedEvent, 0.01f, true, 0.0f);
}

void UAlsxtCameraAnimationInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ALSXTCharacter = Cast<AAlsxtCharacter>(GetOwningActor());
	ALSXTCamera = Cast<UAlsCameraComponent>(GetSkelMeshComponent());

#if WITH_EDITOR
	if (!GetWorld()->IsGameWorld())
	{
		// Use default objects for editor preview.

		if (!IsValid(ALSXTCharacter))
		{
			ALSXTCharacter = GetMutableDefault<AAlsxtCharacter>();
		}

		if (!IsValid(ALSXTCamera))
		{
			ALSXTCamera = GetMutableDefault<UAlsCameraComponent>();
		}
	}
#endif
}

void UAlsxtCameraAnimationInstance::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!IsValid(ALSXTCharacter) || !IsValid(ALSXTCamera))
	{
		return;
	}

	Overlay = ALSXTCharacter->GetOverlayMode();
	Freelooking = ALSXTCharacter->GetDesiredFreelooking();
	Sex = ALSXTCharacter->GetDesiredSex();
	LocomotionVariant = ALSXTCharacter->GetDesiredLocomotionVariant();
	Injury = ALSXTCharacter->GetDesiredInjury();
	CombatStance = ALSXTCharacter->GetDesiredCombatStance();
	WeaponFirearmStance = ALSXTCharacter->GetDesiredWeaponFirearmStance();
	WeaponReadyPosition = ALSXTCharacter->GetDesiredWeaponReadyPosition();
	CameraZoom = ALSXTCharacter->GetCameraZoom();

	if (GetOwningActor()->Implements<UAlsxtCharacterInterface>())
	{
		bool NewIsIdleCameraRotationActive{ IAlsxtIdleAnimationComponentInterface::Execute_IsIdleCameraRotationActive(GetOwningActor()) };
		if (NewIsIdleCameraRotationActive != bIsIdleCameraRotationActive)
		{
			bIsIdleCameraRotationActive = NewIsIdleCameraRotationActive;

			float NewIdleCameraRotation{ IAlsxtIdleAnimationComponentInterface::Execute_GetIdleCameraRotation(GetOwningActor()) };
			if (NewIdleCameraRotation != IdleCameraRotation)
			{
				IdleCameraRotation = NewIdleCameraRotation;
			}
		}	
		
	}
}

void UAlsxtCameraAnimationInstance::OnFirstPersonOverrideChangedEvent()
{
	FirstPersonOverride = GetCurveValue("FirstPersonOverride");
	if (FirstPersonOverride != PreviousFirstPersonOverride)
	{
		OnFirstPersonOverrideChanged.Broadcast(GetCurveValue("FirstPersonOverride"));
		PreviousFirstPersonOverride = GetCurveValue("FirstPersonOverride");
	}
	else
	{
		PreviousFirstPersonOverride = GetCurveValue("FirstPersonOverride");
	}
}
