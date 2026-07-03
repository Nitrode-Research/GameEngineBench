// MIT


#include "AlsxtCharacterAdvanced.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AlsCameraComponent.h"
#include "Components/Character/AlsxtCharacterCameraEffectsComponent.h"
#include "Math/Vector.h"
#include "AlsxtBlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "Net/Core/PushModel/PushModel.h"

AAlsxtCharacterAdvanced::AAlsxtCharacterAdvanced(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	Combat = CreateDefaultSubobject<UAlsxtCombatComponent>(TEXT("Combat"));
	AddOwnedComponent(Combat);

	AcrobaticActions = CreateDefaultSubobject<UAlsxtAcrobaticActionComponent>(TEXT("Acrobatic Actions"));
	AddOwnedComponent(AcrobaticActions);
	
	CameraEffects = CreateDefaultSubobject<UAlsxtCharacterCameraEffectsComponent>(TEXT("Camera Effects"));
	AddOwnedComponent(CameraEffects);
}

void AAlsxtCharacterAdvanced::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDesiredAiming && IAlsxtHeldItemInterface::Execute_IsHoldingAimableItem(this))
	{
		FTransform NewTransform;
		FAlsxtAimState NewAimState = GetAimState();
		NewAimState.CurrentCameraTargetRelativeTransform = NewTransform;
		SetAimState(NewAimState);
	}
}

void AAlsxtCharacterAdvanced::BeginPlay()
{
	Super::BeginPlay();
	HoldBreathTimerDelegate.BindUFunction(this, "HoldBreathTimer");
	BreathRecoveryTimerDelegate.BindUFunction(this, "BreathRecoveryTimer");
}

void AAlsxtCharacterAdvanced::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	auto* EnhancedInput{ Cast<UEnhancedInputComponent>(Input) };
	if (IsValid(EnhancedInput))
	{
		EnhancedInput->BindAction(HoldBreathAction, ETriggerEvent::Triggered, this, &ThisClass::InputHoldBreath);
		EnhancedInput->BindAction(AcrobaticAction, ETriggerEvent::Triggered, this, &ThisClass::InputAcrobaticAction);
		EnhancedInput->BindAction(PrimaryActionAction, ETriggerEvent::Triggered, this, &ThisClass::InputPrimaryAction);
		EnhancedInput->BindAction(SecondaryActionAction, ETriggerEvent::Triggered, this, &ThisClass::InputSecondaryAction);
		EnhancedInput->BindAction(TargetLockAction, ETriggerEvent::Triggered, this, &ThisClass::InputTargetLock);
		EnhancedInput->BindAction(SwitchTargetLeftAction, ETriggerEvent::Triggered, this, &ThisClass::InputSwitchTargetLeft);
		EnhancedInput->BindAction(SwitchTargetRightAction, ETriggerEvent::Triggered, this, &ThisClass::InputSwitchTargetRight);
		// EnhancedInput->BindAction(PrimaryInteractionAction, ETriggerEvent::Triggered, this, &ThisClass::InputPrimaryInteraction);
		// EnhancedInput->BindAction(SecondaryInteractionAction, ETriggerEvent::Triggered, this, &ThisClass::InputSecondaryInteraction);

		OnSetupPlayerInputComponentUpdated.Broadcast();
	}
}

FTransform AAlsxtCharacterAdvanced::GetADSRelativeTransform_Implementation() const
{
	FTransform NewTransform;
	NewTransform.SetLocation(UKismetMathLibrary::InverseTransformDirection(GetCapsuleComponent()->GetComponentTransform(), AimState.CurrentCameraTargetTransform.GetLocation()));
	NewTransform.SetRotation(UKismetMathLibrary::InverseTransformRotation(GetCapsuleComponent()->GetComponentTransform(), AimState.CurrentCameraTargetTransform.GetRotation().Rotator()).Quaternion());
	return NewTransform;
}

FRotator AAlsxtCharacterAdvanced::CalculateRecoilControlRotation_Implementation(FRotator AdditiveControlRotation) const
{
	FRotator CurrentControlRotation = GetWorld()->GetFirstPlayerController()->GetControlRotation();
	FRotator NewControlRotation = CurrentControlRotation + AdditiveControlRotation;
	GetWorld()->GetFirstPlayerController()->SetControlRotation(NewControlRotation);
	return NewControlRotation;
}

void AAlsxtCharacterAdvanced::CalcADSCamera(FMinimalViewInfo& ViewInfo)
{
	if (Camera->IsActive() && GetViewMode() == AlsViewModeTags::FirstPerson && GetDesiredRotationMode() == AlsRotationModeTags::Aiming && GetDesiredCombatStance() == ALSXTCombatStanceTags::Aiming)
	{
		Camera->GetViewInfo(ViewInfo);
		return;
	}

}

void AAlsxtCharacterAdvanced::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredHoldingBreath, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredReloadingType, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredFirearmFingerAction, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredFirearmFingerActionHand, Parameters)
}

// ReloadingType

void AAlsxtCharacterAdvanced::SetDesiredReloadingType(const FGameplayTag& NewReloadingTypeTag)
{
	if (DesiredReloadingType != NewReloadingTypeTag)
	{
		DesiredReloadingType = NewReloadingTypeTag;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredReloadingType, this)

			if (GetLocalRole() == ROLE_AutonomousProxy)
			{
				ServerSetDesiredReloadingType(NewReloadingTypeTag);
			}
	}
}

void AAlsxtCharacterAdvanced::ServerSetDesiredReloadingType_Implementation(const FGameplayTag& NewReloadingTypeTag)
{
	SetDesiredReloadingType(NewReloadingTypeTag);
}

void AAlsxtCharacterAdvanced::SetReloadingType(const FGameplayTag& NewReloadingTypeTag)
{

	if (ReloadingType != NewReloadingTypeTag)
	{
		const auto PreviousReloadingType{ ReloadingType };

		ReloadingType = NewReloadingTypeTag;

		OnReloadingTypeChanged(PreviousReloadingType);
	}
}

void AAlsxtCharacterAdvanced::OnReloadingTypeChanged_Implementation(const FGameplayTag& PreviousReloadingTypeTag) {}

// FirearmFingerAction

void AAlsxtCharacterAdvanced::SetDesiredFirearmFingerAction(const FGameplayTag& NewFirearmFingerActionTag)
{
	if (DesiredFirearmFingerAction != NewFirearmFingerActionTag)
	{
		DesiredFirearmFingerAction = NewFirearmFingerActionTag;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredFirearmFingerAction, this)

			if (GetLocalRole() == ROLE_AutonomousProxy)
			{
				ServerSetDesiredFirearmFingerAction(NewFirearmFingerActionTag);
			}
	}
}

void AAlsxtCharacterAdvanced::ServerSetDesiredFirearmFingerAction_Implementation(const FGameplayTag& NewFirearmFingerActionTag)
{
	SetDesiredFirearmFingerAction(NewFirearmFingerActionTag);
}

void AAlsxtCharacterAdvanced::SetFirearmFingerAction(const FGameplayTag& NewFirearmFingerActionTag)
{

	if (FirearmFingerAction != NewFirearmFingerActionTag)
	{
		const auto PreviousFirearmFingerAction{ FirearmFingerAction };

		FirearmFingerAction = NewFirearmFingerActionTag;

		OnFirearmFingerActionChanged(PreviousFirearmFingerAction);
	}
}

void AAlsxtCharacterAdvanced::OnFirearmFingerActionChanged_Implementation(const FGameplayTag& PreviousFirearmFingerActionTag) {}

// FirearmFingerActionHand

void AAlsxtCharacterAdvanced::SetDesiredFirearmFingerActionHand(const FGameplayTag& NewFirearmFingerActionHandTag)
{
	if (DesiredFirearmFingerActionHand != NewFirearmFingerActionHandTag)
	{
		DesiredFirearmFingerActionHand = NewFirearmFingerActionHandTag;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredFirearmFingerActionHand, this)

			if (GetLocalRole() == ROLE_AutonomousProxy)
			{
				ServerSetDesiredFirearmFingerActionHand(NewFirearmFingerActionHandTag);
			}
	}
}

void AAlsxtCharacterAdvanced::ServerSetDesiredFirearmFingerActionHand_Implementation(const FGameplayTag& NewFirearmFingerActionHandTag)
{
	SetDesiredFirearmFingerActionHand(NewFirearmFingerActionHandTag);
}

void AAlsxtCharacterAdvanced::SetFirearmFingerActionHand(const FGameplayTag& NewFirearmFingerActionHandTag)
{

	if (FirearmFingerActionHand != NewFirearmFingerActionHandTag)
	{
		const auto PreviousFirearmFingerActionHand{ FirearmFingerActionHand };

		FirearmFingerActionHand = NewFirearmFingerActionHandTag;

		OnFirearmFingerActionHandChanged(PreviousFirearmFingerActionHand);
	}
}

void AAlsxtCharacterAdvanced::OnFirearmFingerActionHandChanged_Implementation(const FGameplayTag& PreviousFirearmFingerActionHandTag) {}

// HoldingBreath
// Pauses the Breath Additive Animation for a period at the cost of stamina

void AAlsxtCharacterAdvanced::SetDesiredHoldingBreath(const FGameplayTag& NewHoldingBreathTag)
{
	if (DesiredHoldingBreath != NewHoldingBreathTag)
	{
		DesiredHoldingBreath = NewHoldingBreathTag;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredHoldingBreath, this)

		FAlsxtBreathState NewBreathState = GetBreathState();
		NewBreathState.HoldingBreath = NewHoldingBreathTag;
		SetBreathState(NewBreathState);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerSetDesiredHoldingBreath(NewHoldingBreathTag);
		}
	}
}

FGameplayTag AAlsxtCharacterAdvanced::CalculateBreathReleaseMode() const
{
	FAlsxtBreathState NewBreathState = GetBreathState();
	FGameplayTag ReleaseBreathMode;
	float RemainingBreath = NewBreathState.CurrentMaxHoldBreathTime - NewBreathState.CurrentHoldBreathTime;

	if (RemainingBreath > (NewBreathState.CurrentMaxHoldBreathTime * 0.6))
	{
		ReleaseBreathMode = ALSXTHoldingBreathTags::Released;
	}
	if ((RemainingBreath >= (NewBreathState.CurrentMaxHoldBreathTime * 0.1)) && (RemainingBreath <= (NewBreathState.CurrentMaxHoldBreathTime * 0.6)))
	{
		ReleaseBreathMode = ALSXTHoldingBreathTags::Gasping;
	}
	if (RemainingBreath < (NewBreathState.CurrentMaxHoldBreathTime * 0.1))
	{
		ReleaseBreathMode = ALSXTHoldingBreathTags::Exhausted;
	}

	return ReleaseBreathMode;
}

void AAlsxtCharacterAdvanced::ServerSetDesiredHoldingBreath_Implementation(const FGameplayTag& NewHoldingBreathTag)
{
	SetDesiredHoldingBreath(NewHoldingBreathTag);
}

void AAlsxtCharacterAdvanced::SetHoldingBreath(const FGameplayTag& NewHoldingBreathTag)
{

	if (HoldingBreath != NewHoldingBreathTag)
	{
		const auto PreviousHoldingBreath{ HoldingBreath };
		HoldingBreath = NewHoldingBreathTag;
		OnHoldingBreathChanged(PreviousHoldingBreath);
	}
}

void AAlsxtCharacterAdvanced::OnHoldingBreathChanged_Implementation(const FGameplayTag& PreviousHoldingBreathTag) 
{
	// CharacterSound->PlayHoldingBreathSound(GetBreathState().HoldingBreath, GetDesiredSex(), CharacterCustomization->VoiceParameters.Variant, IALSXTCharacterInterface::Execute_GetStamina(this));
}

FGameplayTag AAlsxtCharacterAdvanced::GetCharacterHoldingBreath_Implementation() const
{
	return GetDesiredHoldingBreath();
}

void AAlsxtCharacterAdvanced::OnDesiredAimingChanged_Implementation(bool bPreviousDesiredAiming)
{
	if (bDesiredAiming)
	{
		if (Execute_GetTargetableOverlayModes(this).HasTag(IAlsxtCharacterInterface::Execute_GetCharacterOverlayMode(this)) && (IAlsxtCharacterInterface::Execute_GetCharacterCombatStance(this) == ALSXTCombatStanceTags::Aiming || Execute_GetCharacterCombatStance(this) == ALSXTCombatStanceTags::Ready))
		{
			Combat->GetClosestTarget();
		}
	}
	else
	{
		Combat->DisengageAllTargets();
	}
}

bool AAlsxtCharacterAdvanced::CanPerformPrimaryAction_Implementation() const
{
	return true;
}

bool AAlsxtCharacterAdvanced::CanPerformSecondaryAction_Implementation() const
{
	return true;
}

// Acrobatic Action Component
void AAlsxtCharacterAdvanced::InputAcrobaticAction(const FInputActionValue& ActionValue)
{
	if (CanPerformAcrobaticAction() && bMovementEnabled && GetLocomotionAction() != AlsLocomotionActionTags::Acrobatic)
	{
		AcrobaticActions->TryAcrobaticAction();
	}
}

void AAlsxtCharacterAdvanced::InputChargeFirearm()
{
	// 
}

void AAlsxtCharacterAdvanced::InputReload()
{
	// 
}

void AAlsxtCharacterAdvanced::InputReloadWithRetention()
{
	// 
}

void AAlsxtCharacterAdvanced::ClearWeaponMalfunction()
{
	// 
}

//Combat Component
void AAlsxtCharacterAdvanced::InputTargetLock(const FInputActionValue& ActionValue)
{
	if (ActionValue.Get<bool>())
	{
		Combat->GetClosestTarget();
	}
	else
	{
		Combat->DisengageAllTargets();
	}
}

void AAlsxtCharacterAdvanced::InputSwitchTargetLeft()
{
	if (GetDesiredCombatStance() == ALSXTCombatStanceTags::Aiming && Execute_GetTargetableOverlayModes(this).HasTag(GetOverlayMode()))
	{
		Combat->GetTargetLeft();
	}
}

void AAlsxtCharacterAdvanced::InputSwitchTargetRight()
{
	if (GetDesiredCombatStance() == ALSXTCombatStanceTags::Aiming && Execute_GetTargetableOverlayModes(this).HasTag(GetOverlayMode()))
	{
		Combat->GetTargetRight();
	}
}

void AAlsxtCharacterAdvanced::InputHoldBreath(const FInputActionValue& ActionValue)
{
	if (ActionValue.Get<bool>() == true)
	{
		if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::False && CanHoldBreath())
		{
			// SetHoldingBreath(ALSXTHoldingBreathTags::True);
			BeginHoldBreathTimer();
		}
	}
	else
	{
		// SetHoldingBreath(CalculateBreathReleaseMode());
		if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::True)
		{
			EndHoldBreathTimer();
		}
		
	}
}

float AAlsxtCharacterAdvanced::CalculateHoldBreathTimer()
{
	return FMath::FRandRange(ALSXTSettings->BreathEffects.BaseHoldingBreathRange.X, ALSXTSettings->BreathEffects.BaseHoldingBreathRange.Y) * IAlsxtCharacterInterface::Execute_GetStamina(this);
}

void AAlsxtCharacterAdvanced::BeginHoldBreathTimer()
{
	SetDesiredHoldingBreath(ALSXTHoldingBreathTags::True);
	FAlsxtBreathState NewBreathState = GetBreathState();
	NewBreathState.HoldingBreath = ALSXTHoldingBreathTags::True;
	NewBreathState.PreviousBreathRate = NewBreathState.CurrentBreathRate;
	NewBreathState.PreviousBreathAlpha = NewBreathState.CurrentBreathAlpha;
	NewBreathState.CurrentMaxHoldBreathTime = CalculateHoldBreathTimer();
	SetBreathState(NewBreathState);
	GetWorld()->GetTimerManager().SetTimer(HoldBreathTimerHandle, HoldBreathTimerDelegate, 0.1f, true);
	CharacterSound->PlayHoldingBreathSound(ALSXTHoldingBreathTags::True, IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(this).Sex, IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(this).Variant, IAlsxtCharacterInterface::Execute_GetStamina(this));
}

void AAlsxtCharacterAdvanced::HoldBreathTimer()
{
	FAlsxtBreathState NewBreathState = GetBreathState();
	NewBreathState.CurrentHoldBreathTime = NewBreathState.CurrentHoldBreathTime + 0.1f;
	SetBreathState(NewBreathState);

	if (BreathState.CurrentHoldBreathTime >= BreathState.CurrentMaxHoldBreathTime)
	{
		SetDesiredHoldingBreath(CalculateBreathReleaseMode());
		EndHoldBreathTimer();
	}
}

void AAlsxtCharacterAdvanced::EndHoldBreathTimer()
{
	FAlsxtBreathState NewBreathState = GetBreathState();
	FGameplayTag BreathReleaseMode;
	BreathReleaseMode = CalculateBreathReleaseMode();
	NewBreathState.CurrentHoldBreathTime = 0.0f;
	NewBreathState.CurrentMaxHoldBreathTime = 0.0f;
	NewBreathState.HoldingBreath = BreathReleaseMode;
	NewBreathState.BreathRecoveryTime = CalculateBreathRecoveryTime();
	SetDesiredHoldingBreath(BreathReleaseMode);
	SetBreathState(NewBreathState);
	CharacterSound->PlayHoldingBreathSound(BreathReleaseMode, IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(this).Sex, IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(this).Variant, IAlsxtCharacterInterface::Execute_GetStamina(this));
	GetWorld()->GetTimerManager().ClearTimer(HoldBreathTimerHandle);

	BeginBreathRecoveryTimer();

}

float AAlsxtCharacterAdvanced::CalculateBreathRecoveryTime()
{
	if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::Released)
	{
		return FMath::FRandRange(ALSXTSettings->BreathEffects.BaseReleasedBreathRange.X, ALSXTSettings->BreathEffects.BaseReleasedBreathRange.Y);
	}
	if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::Gasping)
	{
		return FMath::FRandRange(ALSXTSettings->BreathEffects.BaseGaspingBreathRange.X, ALSXTSettings->BreathEffects.BaseGaspingBreathRange.Y);
	}
	if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::Exhausted)
	{
		return FMath::FRandRange(ALSXTSettings->BreathEffects.BaseExhaustedBreathRange.X, ALSXTSettings->BreathEffects.BaseExhaustedBreathRange.Y);
	}
	else
	{
		return FMath::FRandRange(ALSXTSettings->BreathEffects.BaseReleasedBreathRange.X, ALSXTSettings->BreathEffects.BaseReleasedBreathRange.Y);
	}
}

void AAlsxtCharacterAdvanced::BeginBreathRecoveryTimer()
{
	if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::Gasping)
	{
		IAlsxtCharacterInterface::Execute_SubtractStamina(this, FMath::FRandRange(ALSXTSettings->BreathEffects.BaseGaspingBreathStaminaCost.X, ALSXTSettings->BreathEffects.BaseGaspingBreathStaminaCost.Y));
	}
	if (GetBreathState().HoldingBreath == ALSXTHoldingBreathTags::Exhausted)
	{
		IAlsxtCharacterInterface::Execute_SubtractStamina(this, FMath::FRandRange(ALSXTSettings->BreathEffects.BaseExhaustedBreathStaminaCost.X, ALSXTSettings->BreathEffects.BaseExhaustedBreathStaminaCost.Y));
	}

	FAlsxtBreathState NewBreathState = GetBreathState();
	NewBreathState.BreathRecoveryTime = CalculateBreathRecoveryTime();
	SetBreathState(NewBreathState);
	GetWorld()->GetTimerManager().SetTimer(BreathRecoveryTimerHandle, BreathRecoveryTimerDelegate, 0.1f, true);
}

void AAlsxtCharacterAdvanced::BreathRecoveryTimer()
{
	FAlsxtBreathState NewBreathState = GetBreathState();
	NewBreathState.CurrentBreathRecoveryTime = NewBreathState.CurrentBreathRecoveryTime + 0.1f;
	SetBreathState(NewBreathState);

	if (BreathState.CurrentBreathRecoveryTime >= BreathState.BreathRecoveryTime)
	{
		EndBreathRecoveryTimer();
	}
}

void AAlsxtCharacterAdvanced::EndBreathRecoveryTimer()
{
	FAlsxtBreathState NewBreathState = GetBreathState();
	FGameplayTag BreathReleaseMode;
	NewBreathState.HoldingBreath = ALSXTHoldingBreathTags::False;
	NewBreathState.TargetState = CalculateTargetBreathState();
	NewBreathState.BreathRecoveryTime = 0.0f;
	NewBreathState.CurrentBreathRecoveryTime = 0.0f;
	FAlsxtTargetBreathState NewTargetBreathState;
	NewTargetBreathState.Alpha = 0.75;
	NewTargetBreathState.Rate = 1.0;
	NewBreathState.TargetState = NewTargetBreathState;
	SetBreathState(NewBreathState);
	SetDesiredHoldingBreath(ALSXTHoldingBreathTags::False);
	GetWorld()->GetTimerManager().ClearTimer(BreathRecoveryTimerHandle);
}

void AAlsxtCharacterAdvanced::OnAttackCollision_Implementation(FAttackDoubleHitResult Hit)
{
	// Combat->OnAttackCollisionDelegate.Add()
	// Combat->OnAttackCollisionDelegate.BindUObject(this, &ThisClass::OnAttackCollision_Implementation);
	// Combat->OnAttackCollision(Hit);
}

// Interface Functions

FGameplayTag AAlsxtCharacterAdvanced::GetCharacterReloadingType_Implementation() const
{
	return GetDesiredReloadingType();
}

FGameplayTag AAlsxtCharacterAdvanced::GetCharacterFirearmFingerAction_Implementation() const
{
	return GetDesiredFirearmFingerAction();
}

FGameplayTag AAlsxtCharacterAdvanced::GetCharacterFirearmFingerActionHand_Implementation() const
{
	return GetDesiredFirearmFingerActionHand();
}

//Camera Effects Component Interface
UAlsCameraComponent* AAlsxtCharacterAdvanced::GetCameraComponent_Implementation() const
{
	return Camera;
}

UAlsxtCharacterCameraEffectsComponent* AAlsxtCharacterAdvanced::GetCameraEffectsComponent_Implementation() const
{
	return CameraEffects;
}

FAlsxtGeneralCameraEffectsSettings AAlsxtCharacterAdvanced::GetCameraEffectsSettings_Implementation() const
{
	return CameraEffects->GeneralCameraEffectsSettings;
}

FAlsxtFirearmAimState AAlsxtCharacterAdvanced::GetFirearmAimState_Implementation() const
{
	return FirearmAimState;
}

void AAlsxtCharacterAdvanced::InitializeFirearm_Implementation()
{
	// 
}

void AAlsxtCharacterAdvanced::OnFirearmDischarge_Implementation()
{
	// 
}

void AAlsxtCharacterAdvanced::OnFirearmDischargeEnd_Implementation()
{
	// 
}


void AAlsxtCharacterAdvanced::PlayAttackSound_Implementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& SoundSex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float Stamina)
{
	CharacterSound->PlayAttackSound(MovementSound, AccentSound, WeaponSound, SoundSex, Variant, Overlay, Strength, AttackMode, Stamina);
}

void AAlsxtCharacterAdvanced::SetRadialBlurEffect_Implementation(float Amount)
{
	CameraEffects->SetRadialBlur(Amount);
}

void AAlsxtCharacterAdvanced::ResetRadialBlurEffect_Implementation()
{
	CameraEffects->ResetRadialBlur();
}

void AAlsxtCharacterAdvanced::SetFocusEffect_Implementation(bool NewFocus)
{
	CameraEffects->SetFocusEffect(NewFocus);
}

void AAlsxtCharacterAdvanced::AddDrunkEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength)
{
	CameraEffects->AddDrunkEffect(Magnitude, Length);
}

void AAlsxtCharacterAdvanced::ResetDrunkEffect_Implementation()
{
	CameraEffects->ResetDrunkEffect();
}

void AAlsxtCharacterAdvanced::AddHighEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength)
{
	CameraEffects->AddHighEffect(Magnitude, Length);
}

void AAlsxtCharacterAdvanced::ResetHighEffect_Implementation()
{
	CameraEffects->ResetHighEffect();
}

void AAlsxtCharacterAdvanced::AddSuppressionEffect_Implementation(float Magnitude, float PostDelay)
{
	CameraEffects->AddSuppression(Magnitude, PostDelay);
}

void AAlsxtCharacterAdvanced::ResetSuppressionEffect_Implementation()
{
	CameraEffects->ResetSuppression();
}

void AAlsxtCharacterAdvanced::AddBlindnessEffect_Implementation(float Magnitude, float Length, float FadeOutLength)
{
	CameraEffects->AddBlindnessEffect(Magnitude, Length);
}

void AAlsxtCharacterAdvanced::ResetBlindnessEffect_Implementation()
{
	CameraEffects->ResetBlindnessEffect();
}

void AAlsxtCharacterAdvanced::AddProjectileFlyByEffect_Implementation(USoundBase* Sound, FVector Location, FRotator Rotation)
{
	//
}

void AAlsxtCharacterAdvanced::AddConcussionEffect_Implementation(float Magnitude, float Length, float FadeInLength, float FadeOutLength)
{
	CameraEffects->AddConcussionEffect(Magnitude, Length);
}

void AAlsxtCharacterAdvanced::ResetConcussionEffect_Implementation()
{
	CameraEffects->ResetConcussionEffect();
}

void AAlsxtCharacterAdvanced::AddDamageEffect_Implementation(float Damage, const FGameplayTag& DamageType, const FHitResult& HitResult, float PostDelay)
{
	CameraEffects->AddDamageEffect(Damage, PostDelay);
}

void AAlsxtCharacterAdvanced::ResetDamageEffect_Implementation()
{
	CameraEffects->ResetDamageEffect();
}

void AAlsxtCharacterAdvanced::AddNearDeathEffect_Implementation(float Damage, const FGameplayTag& DamageType)
{
	//
}

void AAlsxtCharacterAdvanced::ResetNearDeathEffect_Implementation()
{
	//
}

void AAlsxtCharacterAdvanced::AddDeathEffect_Implementation(float Damage, const FGameplayTag& DamageType, const FHitResult& HitResult, float PostDelay)
{
	//
}

void AAlsxtCharacterAdvanced::ResetDeathEffect_Implementation()
{
	//
}

//Head Look At Interface
void AAlsxtCharacterAdvanced::IsInFrontOf_Implementation(bool& IsInFrontOf, FVector LookAtActorLocation) const
{
	IsInFrontOf = FVector::DotProduct(LookAtActorLocation, ALSXTMesh->GetForwardVector()) < 0.0f;
}

//Combat Interface
FAlsxtGeneralCombatSettings AAlsxtCharacterAdvanced::GetGeneralCombatSettings_Implementation()
{
	return ALSXTAdvancedSettings->Combat;
}

FAlsxtCombatAttackTraceSettings AAlsxtCharacterAdvanced::GetCombatAttackTraceSettings_Implementation()
{
	return Combat->CurrentAttackTraceSettings;
}

void AAlsxtCharacterAdvanced::BeginCombatAttackCollisionTrace_Implementation(FAlsxtCombatAttackTraceSettings TraceSettings)
{
	Combat->BeginAttackCollisionTrace(TraceSettings);
}

void AAlsxtCharacterAdvanced::EndCombatAttackCollisionTrace_Implementation()
{
	Combat->EndAttackCollisionTrace();
}