// MIT

#include "Components/Character/AlsxtCharacterSoundComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/AlsxtStructs.h"
#include "Settings/AlsxtCharacterSoundSettings.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtCharacterSoundComponentInterface.h"
#include "Interfaces/AlsxtCharacterCustomizationComponentInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UAlsxtCharacterSoundComponent::UAlsxtCharacterSoundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UAlsxtCharacterSoundComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentStamina, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentStaminaTag, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentBreathType, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, VocalizationMixerAudioComponent, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CharacterMovementSoundMixer, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, WeaponMovementAudioComponent, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, WeaponActionAudioComponent, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, WeaponActionAudioComponent, Parameters)
}


void UAlsxtCharacterSoundComponent::BeginPlay()
{
	Super::BeginPlay();
	IAlsxtCharacterInterface::Execute_GetStaminaThresholds(GetOuter(), StaminaOptimalThreshold, StaminaLowThreshold);
	BreathParticleSettings = IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner());
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	CurrentBreathType = IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner());

	if (IsValid(Settings))
	{
			
		MulticastSpawnAudioComponent(VocalizationMixerAudioComponent, Settings->VocalizationMixer, IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner()), IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), 1.0f, GeneralCharacterSoundSettings.VoiceSocketName);

		// SpawnAudioComponent(VocalizationMixerAudioComponent, Settings->VocalizationMixer, IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), 1.0f, GeneralCharacterSoundSettings.VoiceSocketName);

		 // if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
		 // {
		 // 	// Server
		 // 	ServerSpawnAudioComponent(VocalizationMixerAudioComponent, Settings->VocalizationMixer, IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), // IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), 1.0f, GeneralCharacterSoundSettings.VoiceSocketName);
		 // }
		 // else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
		 // {
		 // 	// Reg
		 // 	SpawnAudioComponent(VocalizationMixerAudioComponent, Settings->VocalizationMixer, IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner()), IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), // IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), 1.0f, GeneralCharacterSoundSettings.VoiceSocketName);
		 // }


		

		// Setup Delegate for each time a vocalization plays
		// VocalizationMixerAudioComponent->OnAudioPlayStateChanged
	}
}


void UAlsxtCharacterSoundComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CurrentBreathType = IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner());

}

void UAlsxtCharacterSoundComponent::UpdateStaminaThresholds()
{
	float NewStaminaOptimalThreshold;
	float NewStaminaLowThreshold;
	IAlsxtCharacterInterface::Execute_GetStaminaThresholds(GetOwner(), NewStaminaOptimalThreshold, NewStaminaLowThreshold);
	(NewStaminaOptimalThreshold != StaminaOptimalThreshold) ? StaminaOptimalThreshold = NewStaminaOptimalThreshold : StaminaOptimalThreshold;
	(NewStaminaLowThreshold != StaminaLowThreshold) ? StaminaLowThreshold = NewStaminaLowThreshold : StaminaLowThreshold;
}

void UAlsxtCharacterSoundComponent::UpdateStamina(bool& StaminaTagChanged)
{
	float NewStamina = IAlsxtCharacterInterface::Execute_GetStamina(GetOwner());
	if ((CurrentStamina != NewStamina) && IsValid(VocalizationMixerAudioComponent))
	{
		CurrentStamina = NewStamina;
		FGameplayTag NewStaminaTag{ ConvertStaminaToStaminaTag(IAlsxtCharacterInterface::Execute_GetStamina(GetOwner())) };
		CurrentStaminaTag = NewStaminaTag;
		VocalizationMixerAudioComponent->SetFloatParameter("Stamina", CurrentStamina);
	}
	else
	{
		if ( !IsValid(VocalizationMixerAudioComponent))
		{
			FString Objectname = GetOwner()->GetDebugName(GetOwner());
			FString FunctionName = ": UpdateStamina: VocalizationMixerAudioComponent not valid";
			Objectname.Append(FunctionName);
			// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, Objectname);
		}
	}
}

void UAlsxtCharacterSoundComponent::UpdateVoiceSocketLocation()
{
	VoiceSocketLocation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName);
}

void UAlsxtCharacterSoundComponent::UpdateVoiceSocketRotation()
{
	VoiceSocketRotation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName);
}

void UAlsxtCharacterSoundComponent::DetermineNewSound(TArray<FSound> Sounds, TArray<UObject*> PreviousAssetsReferences, FSound& ResultSound)
{
	FSound NewSoundResult;
	UObject* NewAssetResult {nullptr};
	TArray<FSound> FilteredSounds;
	
	// 	// Return if Return is there are no filtered sounds
	if (Sounds.Num() < 1)
	{
		return;
	}

	// If more than one result, avoid duplicates
	if (Sounds.Num() > 1)
	{
		
		for (FSound Sound : Sounds)
		{		
			if (!PreviousAssetsReferences.Contains(Sound.Sound))
			{
				FilteredSounds.Add(Sound);
			}
		}
		
		//Shuffle Array
		for (int m = FilteredSounds.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredSounds.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredSounds.Num() - 1));
		NewSoundResult = FilteredSounds[RandIndex];
		NewAssetResult = NewSoundResult.Sound;
		ResultSound = NewSoundResult;
	}
	else
	{
		NewSoundResult = FilteredSounds[0];
		NewAssetResult = NewSoundResult.Sound;
		ResultSound = NewSoundResult;
	}
}

void UAlsxtCharacterSoundComponent::SetNewSound(UObject* Sound, TArray<UObject*> PreviousAssetsReferences, int NoRepeats)
{
	if (PreviousAssetsReferences.Num() >= abs(NoRepeats))
	{
		PreviousAssetsReferences.Add(Sound);
		PreviousAssetsReferences.RemoveAt(0, 1, EAllowShrinking::Yes);
	}
	else
	{
		PreviousAssetsReferences.Add(Sound);
	}
}

void UAlsxtCharacterSoundComponent::PlayCharacterBreathEffects(const FGameplayTag& StaminaOverride)
{	
	// FString LocalRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetLocalRole());
	// FString RemoteRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetRemoteRole());
	// FString DebugString = GetOwner()->GetDebugName(GetOwner());
	// DebugString.Append(" Local Role : ");
	// DebugString.Append(LocalRoleString);
	// DebugString.Append(" Remote Role: ");
	// DebugString.Append(RemoteRoleString);
	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, DebugString);
	
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Player
		MulticastPlayCharacterBreathEffects(StaminaOverride);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// AI
		PlayCharacterBreathEffectsImplementation(StaminaOverride);
	}
}

void UAlsxtCharacterSoundComponent::MulticastPlayCharacterBreathEffects_Implementation(const FGameplayTag& StaminaOverride)
{
	PlayCharacterBreathEffectsImplementation(StaminaOverride);
}

void UAlsxtCharacterSoundComponent::PlayCharacterBreathEffectsImplementation(const FGameplayTag& StaminaOverride)
{
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayBreathSound(GetOwner()) && ShouldPlayBreathSound())
	{
		UpdateStaminaThresholds();
		bool StaminaTagChanged;
		UpdateStamina(StaminaTagChanged);
		FGameplayTag StaminaToUse{ (StaminaOverride != FGameplayTag::EmptyTag || !StaminaOverride.IsValid()) ? StaminaOverride : CurrentStaminaTag };
		UpdateVoiceSocketLocation();
		FMotionSounds NewMotionSounds;

		// If New
		if (StaminaTagChanged || StaminaToUse != CurrentStaminaTag || CurrentBreathSounds.IsEmpty())
		{
			// Get and Set New Sounds
			UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
			CurrentBreathSounds = SelectBreathSoundsNew(Settings, IAlsxtCharacterInterface::Execute_GetCharacterSex(GetOwner()), IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(GetOwner()).Variant, IAlsxtCharacterInterface::Execute_GetCharacterBreathState(GetOwner()).BreathType, StaminaToUse);
			TArray<FSound> Sounds;

			for (FALSXTBreathSound BS : CurrentBreathSounds)
			{
				Sounds.Append(BS.Sounds);
			}

			if (Sounds.IsValidIndex(0))
			{
				FSound NewBreathSound;
				DetermineNewSound(Sounds, PreviousBreathsAssets, NewBreathSound);
				float Pitch = FMath::RandRange(NewBreathSound.PitchRange.X, NewBreathSound.PitchRange.Y);
				CurrentBreathSound = NewBreathSound;
				SetNewSound(NewBreathSound.Sound, PreviousBreathsAssets, GeneralCharacterSoundSettings.BreathNoRepeats);
			}
			NewMotionSounds.BreathSounds = Sounds;
		}
		else // If Same
		{
			TArray<FSound> Sounds;

			for (FALSXTBreathSound BS : CurrentBreathSounds)
			{
				Sounds.Append(BS.Sounds);
			}

			if (Sounds.IsValidIndex(0))
			{
				FSound NewBreathSound;
				DetermineNewSound(Sounds, PreviousBreathsAssets, NewBreathSound);
				float Pitch = FMath::RandRange(NewBreathSound.PitchRange.X, NewBreathSound.PitchRange.Y);
				CurrentBreathSound = NewBreathSound;
				SetNewSound(NewBreathSound.Sound, PreviousBreathsAssets, GeneralCharacterSoundSettings.BreathNoRepeats);
			}
			NewMotionSounds.BreathSounds = Sounds;
		}
		// V2
		if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
		{
			// Server
			ServerPlaySound(NewMotionSounds);
		}
		else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
		{
			// Reg
			PlaySound(NewMotionSounds);
		}
	}
}

void UAlsxtCharacterSoundComponent::ServerPlayBreathParticle_Implementation(UNiagaraSystem* NiagaraSystem)
{
	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner()), GeneralCharacterSoundSettings.VoiceSocketName, IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), EAttachLocation::KeepWorldPosition, true, true, ENCPoolMethod::None, true);
}

void UAlsxtCharacterSoundComponent::StartTimeSinceLastCharacterMovementSoundTimer(const float Delay)
{
	ResetTimeSinceLastCharacterMovementSoundTimer();
	TimeSinceLastCharacterMovementSound = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(TimeSinceLastCharacterMovementSoundTimer, this, &UAlsxtCharacterSoundComponent::IncrementTimeSinceLastCharacterMovementSound, Delay, false);
}

void UAlsxtCharacterSoundComponent::IncrementTimeSinceLastCharacterMovementSound()
{
	TimeSinceLastCharacterMovementSound = TimeSinceLastCharacterMovementSound + 0.1f;

	if (TimeSinceLastCharacterMovementSound >= CurrentCharacterMovementSoundDelay)
	{
		ResetTimeSinceLastCharacterMovementSoundTimer();
	}
}

void UAlsxtCharacterSoundComponent::ResetTimeSinceLastCharacterMovementSoundTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeSinceLastCharacterMovementSoundTimer);
	CurrentCharacterMovementSoundDelay = 0.0f;
}

void UAlsxtCharacterSoundComponent::StartTimeSinceLastWeaponMovementSoundTimer(const float Delay)
{
	ResetTimeSinceLastWeaponMovementSoundTimer();
	TimeSinceLastWeaponMovementSound = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(TimeSinceLastWeaponMovementSoundTimer, this, &UAlsxtCharacterSoundComponent::IncrementTimeSinceLastWeaponMovementSound, Delay, false);
}

void UAlsxtCharacterSoundComponent::IncrementTimeSinceLastWeaponMovementSound()
{
	TimeSinceLastWeaponMovementSound = TimeSinceLastWeaponMovementSound + 0.1f;

	if (TimeSinceLastWeaponMovementSound >= CurrentWeaponMovementSoundDelay)
	{
		ResetTimeSinceLastWeaponMovementSoundTimer();
	}
}

void UAlsxtCharacterSoundComponent::ResetTimeSinceLastWeaponMovementSoundTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeSinceLastWeaponMovementSoundTimer);
	CurrentWeaponMovementSoundDelay = 0.0f;
}

void UAlsxtCharacterSoundComponent::StartTimeSinceLastActionSoundTimer(const float Delay)
{
	ResetTimeSinceLastActionSoundTimer();
	TargetActionSoundDelay = FMath::RandRange(GeneralCharacterSoundSettings.ActionSoundDelay.X, GeneralCharacterSoundSettings.ActionSoundDelay.Y);
	GetWorld()->GetTimerManager().SetTimer(TimeSinceLastActionSoundTimer, this, &UAlsxtCharacterSoundComponent::IncrementTimeSinceLastActionSound, Delay, false);
}

void UAlsxtCharacterSoundComponent::IncrementTimeSinceLastActionSound()
{
	TimeSinceLastActionSound = TimeSinceLastActionSound + 0.1f;
	
	if (TimeSinceLastActionSound >= TargetActionSoundDelay)
	{
		ResetTimeSinceLastActionSoundTimer();
	}
}

void UAlsxtCharacterSoundComponent::ResetTimeSinceLastActionSoundTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeSinceLastActionSoundTimer);
	TimeSinceLastActionSound = 0.0f;
}

void UAlsxtCharacterSoundComponent::StartTimeSinceLastAttackSoundTimer(const float Delay)
{
	ResetTimeSinceLastAttackSoundTimer();
	TimeSinceLastAttackSound = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(TimeSinceLastAttackSoundTimer, this, &UAlsxtCharacterSoundComponent::IncrementTimeSinceLastAttackSound, Delay, false);
}

void UAlsxtCharacterSoundComponent::IncrementTimeSinceLastAttackSound()
{
	TimeSinceLastAttackSound = TimeSinceLastAttackSound + 0.1f;

	if (TimeSinceLastAttackSound >= CurrentAttackSoundDelay)
	{
		ResetTimeSinceLastAttackSoundTimer();
	}
}

void UAlsxtCharacterSoundComponent::ResetTimeSinceLastAttackSoundTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeSinceLastAttackSoundTimer);
	CurrentAttackSoundDelay = 0.0f;
}

void UAlsxtCharacterSoundComponent::StartTimeSinceLastDamageSoundTimer(const float Delay)
{
	ResetTimeSinceLastDamageSoundTimer();
	TimeSinceLastDamageSound = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(TimeSinceLastDamageSoundTimer, this, &UAlsxtCharacterSoundComponent::IncrementTimeSinceLastDamageSound, Delay, false);
}

void UAlsxtCharacterSoundComponent::IncrementTimeSinceLastDamageSound()
{
	TimeSinceLastDamageSound = TimeSinceLastDamageSound + 0.1f;

	if (TimeSinceLastDamageSound >= CurrentDamageSoundDelay)
	{
		ResetTimeSinceLastDamageSoundTimer();
	}
}

void UAlsxtCharacterSoundComponent::ResetTimeSinceLastDamageSoundTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeSinceLastDamageSoundTimer);
	CurrentDamageSoundDelay = 0.0f;
}

FName UAlsxtCharacterSoundComponent::GetSocketForMovement(const FGameplayTag MovementType)
{
	FName FoundSocketName;
	for (FMotionSoundAreaMap SoundSourcesForMotionsEntry : GeneralCharacterSoundSettings.SoundSourcesForMotions)
	{
		if (SoundSourcesForMotionsEntry.Motions.HasTag(MovementType))
		{
			FoundSocketName = SoundSourcesForMotionsEntry.MotionSoundBone;
		}
	}
	return FoundSocketName;
}

FGameplayTag UAlsxtCharacterSoundComponent::ConvertWeightTagToStrengthTag(const FGameplayTag Weight)
{
	if (Weight == ALSXTObjectWeightTags::Stealth || Weight == ALSXTObjectWeightTags::VeryLight || Weight == ALSXTObjectWeightTags::Light)
	{
		return ALSXTActionStrengthTags::Light;
	}
	else if (Weight == ALSXTObjectWeightTags::Default)
	{
		return ALSXTActionStrengthTags::Medium;
	}
	else if (Weight == ALSXTObjectWeightTags::Heavy || Weight == ALSXTObjectWeightTags::VeryHeavy || Weight == ALSXTObjectWeightTags::MassivelyHeavy)
	{
		return ALSXTActionStrengthTags::Heavy;
	}
	else
	{
		return ALSXTActionStrengthTags::Medium;
	}
}

FGameplayTag UAlsxtCharacterSoundComponent::ConvertStaminaToStaminaTag(const float Stamina)
{
	if (Stamina == 1.0)
	{
		return ALSXTStaminaTags::Full;
	}
	else if (Stamina < 1.0 && Stamina >= GeneralCharacterSoundSettings.StaminaOptimalThreshold)
	{
		return ALSXTStaminaTags::Optimal;
	}
	else if (( Stamina >= GeneralCharacterSoundSettings.StaminaHalfPoint && Stamina < GeneralCharacterSoundSettings.StaminaOptimalThreshold))
	{
		return ALSXTStaminaTags::Half;
	}
	else if ((Stamina < GeneralCharacterSoundSettings.StaminaHalfPoint && Stamina > GeneralCharacterSoundSettings.StaminaLowThreshold))
	{
		return ALSXTStaminaTags::Low;
	}
	else if ((Stamina < GeneralCharacterSoundSettings.StaminaLowThreshold))
	{
		return ALSXTStaminaTags::Empty;
	}
	else
	{
		return ALSXTStaminaTags::Empty;
	}
}

TArray<FALSXTBreathSound> UAlsxtCharacterSoundComponent::SelectBreathSoundsNew(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& BreathType, const FGameplayTag& Stamina)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTBreathSound> BreathSounds = Settings->BreathSounds;
	TArray<FALSXTBreathSound> FilteredBreathSounds;
	TagsContainer.AddTag(Sex);
	TagsContainer.AddTag(Variant);
	TagsContainer.AddTag(BreathType);
	TagsContainer.AddTag(Stamina);

	// Return if there are no sounds
	if (BreathSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Breath Sounds");
		}
		return FilteredBreathSounds;
	}

	// Filter sounds based on Tag parameters
	for (FALSXTBreathSound BreathSound : BreathSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(BreathSound.Sex);
		CurrentTagsContainer.AppendTags(BreathSound.Variant);
		CurrentTagsContainer.AppendTags(BreathSound.BreathType);
		CurrentTagsContainer.AppendTags(BreathSound.Stamina);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredBreathSounds.Add(BreathSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredBreathSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Filtered Breath Sounds");
		}
		return FilteredBreathSounds;
	}

	return FilteredBreathSounds;
}

TArray<UNiagaraSystem*> UAlsxtCharacterSoundComponent::SelectBreathParticles(const FGameplayTag& BreathType, const FGameplayTag& Stamina)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTBreathParticle> BreathParticles = IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner()).BreathParticles;
	TArray<UNiagaraSystem*> FilteredBreathParticles;
	TagsContainer.AddTag(BreathType);
	TagsContainer.AddTag(Stamina);

	// Return if there are no sounds
	if (BreathParticles.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Breath Particles");
		}
		return FilteredBreathParticles;
	}

	// Filter sounds based on Tag parameters
	for (FALSXTBreathParticle BreathParticle : BreathParticles)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(BreathParticle.BreathType);
		CurrentTagsContainer.AppendTags(BreathParticle.Stamina);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredBreathParticles.Append(BreathParticle.Particles);
		}
	}

	// Return if Return is there are no filtered Particles
	if (FilteredBreathParticles.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Filtered Breath Particles");
		}
		return FilteredBreathParticles;
	}

	return FilteredBreathParticles;
}

UNiagaraSystem* UAlsxtCharacterSoundComponent::DetermineNewBreathParticle()
{
	if (CurrentBreathParticles.IsValidIndex(0))
	{
		if (CurrentBreathParticles.Num() < 1)
		{
			return CurrentBreathParticles[0];
		}
		else
		{
			TArray<UNiagaraSystem*> FilteredParticles = CurrentBreathParticles;
			UNiagaraSystem* SelectedParticle{ nullptr };
			if (FilteredParticles.Contains(LastBreathParticle))
			{
				int IndexToRemove = FilteredParticles.Find(LastBreathParticle);
				FilteredParticles.RemoveAt(IndexToRemove, 1, EAllowShrinking::Yes);
			}

			//Shuffle Array
			for (int m = FilteredParticles.Num() - 1; m >= 0; --m)
			{
				int n = FMath::Rand() % (m + 1);
				if (m != n) FilteredParticles.Swap(m, n);
			}

			// Select Random Array Entry
			int RandIndex = FMath::RandRange(0, (FilteredParticles.Num() - 1));
			SelectedParticle = FilteredParticles[RandIndex];
			LastBreathParticle = SelectedParticle;
			return SelectedParticle;
		}
	}
	else
	{
		return nullptr;
	}
}

TArray<FALSXTBreathSound> UAlsxtCharacterSoundComponent::SelectBreathSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& BreathType, const float Stamina)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTBreathSound> BreathSounds = Settings->BreathSounds;
	TArray<FALSXTBreathSound> FilteredBreathSounds;
	FGameplayTag StaminaTag = ConvertStaminaToStaminaTag(Stamina);
	TagsContainer.AddTag(Sex);
	TagsContainer.AddTag(Variant);
	TagsContainer.AddTag(BreathType);
	TagsContainer.AddTag(StaminaTag);

	// Return if there are no sounds
	if (BreathSounds.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Breath Sounds");
		return FilteredBreathSounds;
	}

	// Filter sounds based on Tag parameters
	for (FALSXTBreathSound BreathSound : BreathSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(BreathSound.Sex);
		CurrentTagsContainer.AppendTags(BreathSound.Variant);
		CurrentTagsContainer.AppendTags(BreathSound.BreathType);
		CurrentTagsContainer.AppendTags(BreathSound.Stamina);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredBreathSounds.Add(BreathSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredBreathSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No Filtered Breath Sounds");
		}
		return FilteredBreathSounds;
	}

	return FilteredBreathSounds;
}

TArray<FALSXTCharacterMovementSound> UAlsxtCharacterSoundComponent::SelectCharacterMovementSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Type, const FGameplayTag& Weight)
{
	TMap<TEnumAsByte<EPhysicalSurface>, FALSXTCharacterMovementSounds> MovementSoundsMap = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner())->MovementSounds;
	TEnumAsByte<EPhysicalSurface> FoundSurface;
	IAlsxtCharacterInterface::Execute_GetClothingSurfaceForMovement(GetOwner(), FoundSurface, Type);
	TArray<FALSXTCharacterMovementSound> Sounds = MovementSoundsMap.FindRef(FoundSurface).Sounds;
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, UKismetSystemLibrary::GetDisplayName(Sounds[0].Sound.Sound.Sound));
	FGameplayTagContainer TagsContainer;
	TagsContainer.AddTag(Type);
	TagsContainer.AddTag(Weight);

	TArray<FALSXTCharacterMovementSound> FilteredSounds;

	// Return if there are no sounds
	if (Sounds.Num() < 1)
	{
		return FilteredSounds;
	}

	// Filter sounds based on Tag parameters
	for (FALSXTCharacterMovementSound Sound : Sounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Sound.Type);
		CurrentTagsContainer.AppendTags(Sound.Weight);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, UKismetSystemLibrary::GetDisplayName(Sound.Sound.Sound.Sound));
			FilteredSounds.Add(Sound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredSounds.Num() < 1)
	{
		return FilteredSounds;
	}

	return FilteredSounds;
}

TArray<FALSXTCharacterMovementSound> UAlsxtCharacterSoundComponent::SelectCharacterMovementAccentSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Type, const FGameplayTag& Weight)
{
	TMap<TEnumAsByte<EPhysicalSurface>, FALSXTCharacterMovementSounds> MovementAccentSoundsMap = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner())->MovementAccentSounds;
	TEnumAsByte<EPhysicalSurface> AccentSurface;
	IAlsxtCharacterInterface::Execute_GetAccentSurfaceForMovement(GetOwner(), AccentSurface, Type);
	TArray<FALSXTCharacterMovementSound> Sounds = MovementAccentSoundsMap.FindRef(AccentSurface).Sounds;
	FGameplayTagContainer TagsContainer;
	TagsContainer.AddTag(Type);
	TagsContainer.AddTag(Weight);

	TArray<FALSXTCharacterMovementSound> FilteredSounds;

	// Return if there are no sounds
	if (Sounds.Num() < 1)
	{
		return FilteredSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto Sound : Sounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Sound.Type);
		CurrentTagsContainer.AppendTags(Sound.Weight);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredSounds.Add(Sound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredSounds.Num() < 1)
	{
		return FilteredSounds;
	}

	return FilteredSounds;
}

TArray<FALSXTWeaponMovementSound> UAlsxtCharacterSoundComponent::SelectWeaponMovementSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Weapon, const FGameplayTag& Type)
{
	TArray<FALSXTWeaponMovementSound> Sounds = IAlsxtCharacterSoundComponentInterface::Execute_SelectWeaponSoundSettings(GetOwner())->WeaponMovementSounds;
	TArray<FALSXTWeaponMovementSound> FilteredSounds;
	FGameplayTagContainer TagsContainer;
	TagsContainer.AddTag(Weapon);
	TagsContainer.AddTag(Type);

	// Return if there are no sounds
	if (Sounds.Num() < 1)
	{
		return FilteredSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto Sound : Sounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Sound.Weapon);
		CurrentTagsContainer.AppendTags(Sound.Type);
		// FString DebugText0 = TagsContainer.ToString();
		// FString DebugText3 = CurrentTagsContainer.ToString();
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, DebugText0);
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, DebugText3);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredSounds.Add(Sound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "No Filtered WeaponMovementSound");
		}
		return FilteredSounds;
	}

	return FilteredSounds;
}

FALSXTWeaponActionSound UAlsxtCharacterSoundComponent::SelectWeaponActionSound(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Type)
{
	TArray<FALSXTWeaponActionSound> Sounds = IAlsxtCharacterSoundComponentInterface::Execute_SelectWeaponSoundSettings(GetOwner())->WeaponActionSounds;
	TArray<FALSXTWeaponActionSound> FilteredSounds;
	FGameplayTagContainer TagsContainer;
	TagsContainer.AddTag(Type);
	FALSXTWeaponActionSound SelectedSound;

	// Return if there are no sounds
	if (Sounds.Num() < 1)
	{
		return SelectedSound;
	}

	// Filter sounds based on Tag parameters
	for (auto Sound : Sounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Sound.Type);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredSounds.Add(Sound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredSounds.Num() < 1)
	{
		return SelectedSound;
	}

	// If more than one result, avoid duplicates
	if (FilteredSounds.Num() > 1)
	{
		// If FilteredSounds contains LastWeaponActionSound, remove it from FilteredSounds array to avoid duplicates
		if (FilteredSounds.Contains(LastWeaponActionSound))
		{
			int IndexToRemove = FilteredSounds.Find(LastWeaponActionSound);
			FilteredSounds.RemoveAt(IndexToRemove, 1, EAllowShrinking::Yes);
		}

		//Shuffle Array
		for (int m = FilteredSounds.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredSounds.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredSounds.Num() - 1));
		SelectedSound = FilteredSounds[RandIndex];
		LastWeaponActionSound = SelectedSound;
		return SelectedSound;
	}
	else
	{
		SelectedSound = FilteredSounds[0];
		LastWeaponActionSound = SelectedSound;
		return SelectedSound;
	}

	return SelectedSound;
}

TArray<FALSXTHoldingBreathSound> UAlsxtCharacterSoundComponent::SelectHoldingBreathSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& HoldingBreathType)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTHoldingBreathSound> HoldingBreathSounds = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner())->HoldingBreathSounds;
	TArray<FALSXTHoldingBreathSound> FilteredHoldingBreathSounds;
	TagsContainer.AddTag(Sex);
	TagsContainer.AddTag(Variant);
	TagsContainer.AddTag(HoldingBreathType);

	// Return if there are no sounds
	if (HoldingBreathSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Sounds");
		}
		return FilteredHoldingBreathSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto HoldingBreathSound : HoldingBreathSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(HoldingBreathSound.Sex);
		CurrentTagsContainer.AppendTags(HoldingBreathSound.Variant);
		CurrentTagsContainer.AppendTags(HoldingBreathSound.HoldingBreathType);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredHoldingBreathSounds.Add(HoldingBreathSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredHoldingBreathSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Filtered ActionSounds");
		}
		return FilteredHoldingBreathSounds;
	}

	return FilteredHoldingBreathSounds;
}

TArray<FALSXTCharacterActionSound> UAlsxtCharacterSoundComponent::SelectActionSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTCharacterActionSound> ActionSounds = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner())->ActionSounds;
	TArray<FALSXTCharacterActionSound> FilteredActionSounds;
	FGameplayTag StaminaTag = ConvertStaminaToStaminaTag(Stamina);
	TagsContainer.AddTag(Sex);
	TagsContainer.AddTag(Variant);
	TagsContainer.AddTag(Overlay);
	TagsContainer.AddTag(Strength);
	TagsContainer.AddTag(StaminaTag);

	// Return if there are no sounds
	if (ActionSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Sounds");
		}
		return FilteredActionSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto ActionSound : ActionSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(ActionSound.Sex);
		CurrentTagsContainer.AppendTags(ActionSound.Variant);
		CurrentTagsContainer.AppendTags(ActionSound.Overlay);
		CurrentTagsContainer.AppendTags(ActionSound.Strength);
		CurrentTagsContainer.AppendTags(ActionSound.Stamina);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredActionSounds.Add(ActionSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredActionSounds.Num() < 1)
	{
		if (GeneralCharacterSoundSettings.bDebugMode)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Filtered ActionSounds");
		}
		return FilteredActionSounds;
	}

	return FilteredActionSounds;
}

TArray<FALSXTCharacterActionSound> UAlsxtCharacterSoundComponent::SelectAttackSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTCharacterActionSound> AttackSounds = Settings->AttackSounds;
	TArray<FALSXTCharacterActionSound> FilteredAttackSounds;
	FGameplayTag StaminaTag = ConvertStaminaToStaminaTag(Stamina);
	TagsContainer.AddTagFast(Sex);
	TagsContainer.AddTagFast(Variant);
	TagsContainer.AddTagFast(Overlay);
	TagsContainer.AddTagFast(Strength);
	TagsContainer.AddTagFast(StaminaTag);

	// Return if there are no sounds
	if (AttackSounds.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Sounds");
		return FilteredAttackSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto AttackSound : AttackSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(AttackSound.Sex);
		CurrentTagsContainer.AppendTags(AttackSound.Variant);
		CurrentTagsContainer.AppendTags(AttackSound.Overlay);
		CurrentTagsContainer.AppendTags(AttackSound.Strength);
		CurrentTagsContainer.AppendTags(AttackSound.Stamina);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredAttackSounds.Add(AttackSound);
		}
	}

	// Return if there are sounds
	if (FilteredAttackSounds.Num() < 1)
	{
		return FilteredAttackSounds;
	}

	return FilteredAttackSounds;
}

TArray<FALSXTCharacterDamageSound> UAlsxtCharacterSoundComponent::SelectDamageSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& AttackMethod, const FGameplayTag& Form, const FGameplayTag& Strength)
{
	FGameplayTagContainer TagsContainer;
	// TArray<FALSXTCharacterDamageSound> DamageSounds = Settings->DamageSounds;
	TArray<FALSXTCharacterDamageSound> DamageSounds = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner())->DamageSounds;
	TArray<FALSXTCharacterDamageSound> FilteredDamageSounds;
	TArray<FSound> FilteredFSounds;
	TagsContainer.AddTagFast(Sex);
	TagsContainer.AddTagFast(ALSXTVoiceVariantTags::Default);
	TagsContainer.AddTagFast(AttackMethod);
	// TagsContainer.AddTagFast(Overlay);
	TagsContainer.AddTagFast(ALSXTImpactFormTags::Blunt);
	FGameplayTag DamageTag = ALSXTDamageAmountTags::Moderate;
	TagsContainer.AddTagFast(DamageTag);

	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TagsContainer.ToString());

	// Return if there are no sounds
	if (DamageSounds.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Damage Sounds");
		return FilteredDamageSounds;
	}

	// Filter sounds based on Tag parameters
	for (FALSXTCharacterDamageSound DamageSound : DamageSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(DamageSound.Sex);
		CurrentTagsContainer.AppendTags(DamageSound.Variant);
		CurrentTagsContainer.AppendTags(DamageSound.Form);
		CurrentTagsContainer.AppendTags(DamageSound.AttackMethod);
		CurrentTagsContainer.AppendTags(DamageSound.Damage);

		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, CurrentTagsContainer.ToString());

		// FilteredDamageSounds.Add(DamageSound);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredDamageSounds.Add(DamageSound);
			FilteredFSounds.Append(DamageSound.Sounds);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredDamageSounds.Num() < 1 )
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Filtered Damage Sounds");
		return FilteredDamageSounds;
	}
	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found Sounds");
	return FilteredDamageSounds;

}

TArray<FALSXTResponseVocalization> UAlsxtCharacterSoundComponent::SelectResponseVocalizations(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Velocity, const FGameplayTag& Form, const FGameplayTag& Health, const bool Mature)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTResponseVocalization> ResponseVocalizations = Settings->ResponseVocalizations;
	TArray<FALSXTResponseVocalization> FilteredResponseVocalizations;
	FALSXTResponseVocalization SelectedDeathSound;
	TagsContainer.AddTagFast(Sex);
	TagsContainer.AddTagFast(Variant);
	TagsContainer.AddTagFast(Velocity);
	TagsContainer.AddTagFast(Form);
	FGameplayTag DeathTag = ALSXTDamageAmountTags::Moderate;
	TagsContainer.AddTagFast(DeathTag);

	// Return if there are no sounds
	if (ResponseVocalizations.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Sounds");
		return FilteredResponseVocalizations;
	}

	// Filter sounds based on Tag parameters
	for (auto DeathSound : ResponseVocalizations)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(DeathSound.Sex);
		CurrentTagsContainer.AppendTags(DeathSound.Variant);
		CurrentTagsContainer.AppendTags(DeathSound.Velocity);
		CurrentTagsContainer.AppendTags(DeathSound.Form);
		CurrentTagsContainer.AppendTags(DeathSound.Health);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredResponseVocalizations.Add(DeathSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredResponseVocalizations.Num() < 1)
	{
		return FilteredResponseVocalizations;
	}

	return FilteredResponseVocalizations;
}

TArray<FALSXTCharacterDamageSound> UAlsxtCharacterSoundComponent::SelectDeathSounds(UAlsxtCharacterSoundSettings* Settings, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Form, const FGameplayTag& Strength)
{
	FGameplayTagContainer TagsContainer;
	TArray<FALSXTCharacterDamageSound> DeathSounds = Settings->DeathSounds;
	TArray<FALSXTCharacterDamageSound> FilteredDeathSounds;
	FALSXTCharacterDamageSound SelectedDeathSound;
	TagsContainer.AddTagFast(Sex);
	TagsContainer.AddTagFast(Variant);
	TagsContainer.AddTagFast(Overlay);
	TagsContainer.AddTagFast(Form);
	FGameplayTag DeathTag = ALSXTDamageAmountTags::Moderate;
	TagsContainer.AddTagFast(DeathTag);

	// Return if there are no sounds
	if (DeathSounds.Num() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "No Sounds");
		return FilteredDeathSounds;
	}

	// Filter sounds based on Tag parameters
	for (auto DeathSound : DeathSounds)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(DeathSound.Sex);
		CurrentTagsContainer.AppendTags(DeathSound.Variant);
		CurrentTagsContainer.AppendTags(DeathSound.Damage);
		CurrentTagsContainer.AppendTags(DeathSound.Form);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredDeathSounds.Add(DeathSound);
		}
	}

	// Return if Return is there are no filtered sounds
	if (FilteredDeathSounds.Num() < 1)
	{
		return FilteredDeathSounds;
	}

	return FilteredDeathSounds;
}

bool UAlsxtCharacterSoundComponent::ShouldPlayBreathSound()
{
	return IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner()).AudibleBreathStaminaLevels.HasTag(IAlsxtCharacterInterface::Execute_GetStaminaTag(GetOwner()));	
	// 	bool IsPaused = VocalizationMixerAudioComponent->bIsPaused;

}

bool UAlsxtCharacterSoundComponent::ShouldPlayHoldingBreathSoundDelegate(const FGameplayTag& HoldBreathType, const float Stamina)
{
	// return (TimeSinceLastActionSound >= TargetActionSoundDelay);
	return true;
}

bool UAlsxtCharacterSoundComponent::ShouldPlayActionSound(const FGameplayTag& Strength, const float Stamina)
{
	// return (TimeSinceLastActionSound >= TargetActionSoundDelay);
	return true;
}

bool UAlsxtCharacterSoundComponent::ShouldPlayAttackSound(const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const float Stamina)
{
	return (TimeSinceLastAttackSound >= TargetAttackSoundDelay);
	// return true;
}

bool UAlsxtCharacterSoundComponent::ShouldPlayDamageSound(const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	return true;
}

bool UAlsxtCharacterSoundComponent::ShouldPlayDeathSound(const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	return true;
}

void UAlsxtCharacterSoundComponent::PlayCharacterMovementSound(bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Weight)
{
	
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		MulticastPlayCharacterMovementSound(AccentSound, WeaponSound, Type, Weight);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlayCharacterMovementSoundImplementation(AccentSound, WeaponSound, Type, Weight);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// AI
		MulticastPlayCharacterMovementSound(AccentSound, WeaponSound, Type, Weight);
	}

}

void UAlsxtCharacterSoundComponent::MulticastPlayCharacterMovementSound_Implementation(bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Weight)
{
	PlayCharacterMovementSoundImplementation(AccentSound, WeaponSound, Type, Weight);
}

void UAlsxtCharacterSoundComponent::PlayCharacterMovementSoundImplementation(bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Weight)
{
	FMotionSounds MotionSounds;
	float Delay = FMath::RandRange(GeneralCharacterSoundSettings.CharacterMovementSoundDelay.X, GeneralCharacterSoundSettings.CharacterMovementSoundDelay.Y);
	StartTimeSinceLastActionSoundTimer(Delay);
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	FGameplayTag MovementStrength = ConvertWeightTagToStrengthTag(Weight);

	// MOVEMENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayCharacterMovementSound(GetOwner()))
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementSounds = SelectCharacterMovementSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementSounds = Sounds;
		}
	}

	// ACCENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayMovementAccentSound(GetOwner(), Type, MovementStrength) && AccentSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementAccentSounds = SelectCharacterMovementAccentSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementAccentSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, MovementStrength) && WeaponSound)
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}


	// if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	// {
	// 	MulticastPlaySound(MotionSounds);
	// }
	// else
	// {
	// 	ServerPlaySound(MotionSounds);
	// }

}

void UAlsxtCharacterSoundComponent::PlayWeaponMovementSound(const FGameplayTag& Weapon, const FGameplayTag& Type)
{
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FGameplayTag Strength = ConvertWeightTagToStrengthTag(Weight);
	if (GetOwner()->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}
	if (!IAlsxtCharacterSoundComponentInterface::Execute_CanPlayWeaponMovementSound(GetOwner()))
	{
		return;
	}
	float Delay = FMath::RandRange(GeneralCharacterSoundSettings.WeaponMovementSoundDelay.X, GeneralCharacterSoundSettings.WeaponMovementSoundDelay.Y);
	StartTimeSinceLastWeaponMovementSoundTimer(Delay);
	UAlsxtCharacterSoundSettings* Settings = SelectCharacterSoundSettings();
	FMotionSounds MotionSounds;

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, Strength))
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::MulticastPlayWeaponMovementSound_Implementation(const FGameplayTag& Weapon, const FGameplayTag& Type)
{
	PlayWeaponMovementSoundImplementation(Weapon, Type);
}

void UAlsxtCharacterSoundComponent::PlayWeaponMovementSoundImplementation(const FGameplayTag& Weapon, const FGameplayTag& Type)
{
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FGameplayTag Strength = ConvertWeightTagToStrengthTag(Weight);
	if (GetOwner()->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}
	if (!IAlsxtCharacterSoundComponentInterface::Execute_CanPlayWeaponMovementSound(GetOwner()))
	{
		return;
	}
	float Delay = FMath::RandRange(GeneralCharacterSoundSettings.WeaponMovementSoundDelay.X, GeneralCharacterSoundSettings.WeaponMovementSoundDelay.Y);
	StartTimeSinceLastWeaponMovementSoundTimer(Delay);
	UAlsxtCharacterSoundSettings* Settings = SelectCharacterSoundSettings();
	FMotionSounds MotionSounds;

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, Strength))
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::PlayWeaponActionSound(const FGameplayTag& Type)
{
	if (GetOwner()->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}
	if (!CanPlayWeaponActionSound())
	{
		return;
	}
	UAlsxtCharacterSoundSettings* Settings = SelectCharacterSoundSettings();
	FMotionSounds MotionSounds;
	TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
	TArray<TObjectPtr<UObject>> WeaponMovementSoundAssets;
	FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
	MotionSounds.WeaponMovementSocketName = "hand_r";
	TArray<FSound> Sounds;

	for (FALSXTWeaponMovementSound WM : WeaponMovementSounds)
	{
		Sounds.Append(WM.Sounds);
		
		for (FSound WMSound : WM.Sounds)
		{
			WeaponMovementSoundAssets.Add(WMSound.Sound);
		}
	}

	if (Sounds.IsValidIndex(0))
	{
		MotionSounds.WeaponMovementSounds = Sounds;
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::PlayHoldingBreathSound(const FGameplayTag& HoldingBreathType, const FGameplayTag& Sex, const FGameplayTag& Variant, const float Stamina)
{
	PlayHoldingBreathSoundImplementation(HoldingBreathType, Sex, Variant, Stamina);
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		MulticastPlayHoldingBreathSound(HoldingBreathType, Sex, Variant, Stamina);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlayHoldingBreathSoundImplementation(HoldingBreathType, Sex, Variant, Stamina);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// AI
		MulticastPlayHoldingBreathSound(HoldingBreathType, Sex, Variant, Stamina);
	}
}

void UAlsxtCharacterSoundComponent::MulticastPlayHoldingBreathSound_Implementation(const FGameplayTag& HoldingBreathType, const FGameplayTag& Sex, const FGameplayTag& Variant, const float Stamina)
{
	PlayHoldingBreathSoundImplementation(HoldingBreathType, Sex, Variant, Stamina);
}

void UAlsxtCharacterSoundComponent::PlayHoldingBreathSoundImplementation(const FGameplayTag& HoldingBreathType, const FGameplayTag& Sex, const FGameplayTag& Variant, const float Stamina)
{
	FMotionSounds MotionSounds;
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());

	//ACTION
	if (ShouldPlayHoldingBreathSoundDelegate(HoldingBreathType, Stamina))
	{
		TArray<FALSXTHoldingBreathSound> HoldingBreathSounds = SelectHoldingBreathSounds(Settings, IAlsxtCharacterInterface::Execute_GetCharacterSex(GetOwner()), Variant, HoldingBreathType);
		TArray<FSound> Sounds;

		for (FALSXTHoldingBreathSound HoldingBreathSound : HoldingBreathSounds)
		{
			Sounds.Append(HoldingBreathSound.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.VocalSounds = Sounds;
			// MotionSounds.BreathSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::PlayActionSound(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{	
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		MulticastPlayActionSound(MovementSound, AccentSound, WeaponSound, Type, Sex, Variant, Overlay, Strength, Stamina);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlayActionSoundImplementation(MovementSound, AccentSound, WeaponSound, Type, Sex, Variant, Overlay, Strength, Stamina);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// AI
		MulticastPlayActionSound(MovementSound, AccentSound, WeaponSound, Type, Sex, Variant, Overlay, Strength, Stamina);
	}
}

void UAlsxtCharacterSoundComponent::MulticastPlayActionSound_Implementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{
	PlayActionSoundImplementation(MovementSound, AccentSound, WeaponSound, Type, Sex, Variant, Overlay, Strength, Stamina);
}

void UAlsxtCharacterSoundComponent::ClientPlayActionSound_Implementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{
	PlayActionSoundImplementation(MovementSound, AccentSound, WeaponSound, Type, Sex, Variant, Overlay, Strength, Stamina);
}

void UAlsxtCharacterSoundComponent::PlayActionSoundImplementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Type, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const float Stamina)
{
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FMotionSounds MotionSounds;
	float Delay = FMath::RandRange(GeneralCharacterSoundSettings.ActionSoundDelay.X, GeneralCharacterSoundSettings.ActionSoundDelay.Y);
	StartTimeSinceLastActionSoundTimer(Delay);
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	FGameplayTag MovementStrength = ConvertWeightTagToStrengthTag(Weight);

	// MOVEMENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayCharacterMovementSound(GetOwner()) && MovementSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementSounds = SelectCharacterMovementSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementSounds = Sounds;
		}
	}

	// ACCENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayMovementAccentSound(GetOwner(), Type, MovementStrength) && AccentSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementAccentSounds = SelectCharacterMovementAccentSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CMA : CharacterMovementAccentSounds)
		{
			Sounds.Append(CMA.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	//ACTION
	if (ShouldPlayActionSound(MovementStrength, Stamina))
	{
		TArray<FALSXTCharacterActionSound> ActionSounds = SelectActionSounds(Settings, IAlsxtCharacterInterface::Execute_GetCharacterSex(GetOwner()), Variant, IAlsxtCharacterInterface::Execute_GetCharacterOverlayMode(GetOwner()), MovementStrength, Stamina);
		TArray<FSound> Sounds;

		for (FALSXTCharacterActionSound AC : ActionSounds)
		{
			Sounds.Append(AC.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.VocalSounds = Sounds;
		}
	}

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, MovementStrength) && WeaponSound)
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}

	// if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	// {
	// 	MulticastPlaySound(MotionSounds);
	// }
	// else
	// {
	// 	ServerPlaySound(MotionSounds);
	// }
}

void UAlsxtCharacterSoundComponent::PlayAttackSound(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float Stamina)
{
	if (GetOwner()->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FGameplayTag Type = ALSXTCharacterMovementSoundTags::Jumping;
	FMotionSounds MotionSounds;
	float Delay = FMath::RandRange(GeneralCharacterSoundSettings.ActionSoundDelay.X, GeneralCharacterSoundSettings.ActionSoundDelay.Y);
	StartTimeSinceLastActionSoundTimer(Delay);
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());

	// MOVEMENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayCharacterMovementSound(GetOwner()) && MovementSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementSounds = SelectCharacterMovementSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementSounds = Sounds;
		}
	}

	// ACCENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayMovementAccentSound(GetOwner(), Type, Strength) && AccentSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementAccentSounds = SelectCharacterMovementAccentSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CMA : CharacterMovementAccentSounds)
		{
			Sounds.Append(CMA.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	//ATTACK
	if (ShouldPlayAttackSound(AttackMode, Strength, Stamina))
	{
		TArray<FALSXTCharacterActionSound> AttackSounds = SelectAttackSounds(Settings, Sex, ALSXTVoiceVariantTags::Default, Overlay, Strength, Stamina);
		TArray<FSound> Sounds;

		for (FALSXTCharacterActionSound AC : AttackSounds)
		{
			Sounds.Append(AC.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.VocalSounds = Sounds;
		}
	}

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, Strength) && WeaponSound)
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}

}

void UAlsxtCharacterSoundComponent::PlayDamageSound(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "MulticastPlayActionSound");
		MulticastPlayDamageSound(MovementSound, AccentSound, WeaponSound, Sex, Variant, Overlay, AttackMethod, Strength, AttackForm, Damage);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "AI: PlayActionSoundImplementation");
		MulticastPlayDamageSound(MovementSound, AccentSound, WeaponSound, Sex, Variant, Overlay, AttackMethod, Strength, AttackForm, Damage);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// AI
		MulticastPlayDamageSound(MovementSound, AccentSound, WeaponSound, Sex, Variant, Overlay, AttackMethod, Strength, AttackForm, Damage);
	}
}

void UAlsxtCharacterSoundComponent::MulticastPlayDamageSound_Implementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	PlayDamageSoundImplementation(MovementSound, AccentSound, WeaponSound, Sex, Variant, Overlay, AttackMethod, Strength, AttackForm, Damage);
}

void UAlsxtCharacterSoundComponent::PlayDamageSoundImplementation(bool MovementSound, bool AccentSound, bool WeaponSound, const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FGameplayTag Type = ALSXTCharacterMovementSoundTags::Impact;
	FMotionSounds MotionSounds;

	// MOVEMENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayCharacterMovementSound(GetOwner()) && MovementSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementSounds = SelectCharacterMovementSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementSounds = Sounds;
		}
	}

	// ACCENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayMovementAccentSound(GetOwner(), Type, Strength) && AccentSound)
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementAccentSounds = SelectCharacterMovementAccentSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CMA : CharacterMovementAccentSounds)
		{
			Sounds.Append(CMA.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	//DAMAGE
	if (ShouldPlayDamageSound(AttackMethod, Strength, AttackForm, Damage))
	{
		TArray<FALSXTCharacterDamageSound> DamageSounds = SelectDamageSounds(Settings, Sex, Variant, Overlay, AttackMethod, AttackForm, Strength);
		TArray<FSound> Sounds;

		for (FALSXTCharacterDamageSound AC : DamageSounds)
		{
			Sounds.Append(AC.Sounds);
		}

		if (Sounds.Num() > 0)
		{
			MotionSounds.VocalSounds = Sounds;
		}
	}

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, Strength) && WeaponSound)
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Player
		// FString LocalRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetLocalRole());
		// FString RemoteRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetRemoteRole());
		// FString DebugString = GetOwner()->GetDebugName(GetOwner());
		// DebugString.Append(" Local Role : ");
		// DebugString.Append(LocalRoleString);
		// DebugString.Append(" Remote Role: ");
		// DebugString.Append(RemoteRoleString);
		// DebugString.Append(FString::Printf(TEXT("Sounds Found: %d"), MotionSounds.VocalSounds.Num()));
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, DebugString);

		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// AI
		// PlaySound(MotionSounds);

		// FString LocalRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetLocalRole());
		// FString RemoteRoleString = StaticEnum<ENetRole>()->GetValueAsString(GetOwner()->GetRemoteRole());
		// FString DebugString = GetOwner()->GetDebugName(GetOwner());
		// DebugString.Append(" Local Role : ");
		// DebugString.Append(LocalRoleString);
		// DebugString.Append(" Remote Role: ");
		// DebugString.Append(RemoteRoleString);
		// DebugString.Append(FString::Printf(TEXT("Sounds Found: %d"), MotionSounds.VocalSounds.Num()));
		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, DebugString);

		// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Sounds Found: %d"), MotionSounds.VocalSounds.Num()));
		PlaySound(MotionSounds);
		// ServerPlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::PlayDeathSound(const FGameplayTag& Sex, const FGameplayTag& Variant, const FGameplayTag& Overlay, const FGameplayTag& AttackMethod, const FGameplayTag& Strength, const FGameplayTag& AttackForm, const float Damage)
{
	FGameplayTag Weight = IAlsxtCharacterInterface::Execute_GetWeightTag(GetOwner());
	FGameplayTag Type = ALSXTCharacterMovementSoundTags::Impact;
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	FMotionSounds MotionSounds;

	// MOVEMENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_CanPlayCharacterMovementSound(GetOwner()))
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementSounds = SelectCharacterMovementSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CM : CharacterMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementSounds = Sounds;
		}
	}

	// ACCENT
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayMovementAccentSound(GetOwner(), Type, Strength))
	{
		TArray<FALSXTCharacterMovementSound> CharacterMovementAccentSounds = SelectCharacterMovementAccentSounds(Settings, Type, Weight);
		TArray<FSound> Sounds;

		for (FALSXTCharacterMovementSound CMA : CharacterMovementAccentSounds)
		{
			Sounds.Append(CMA.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.CharacterMovementAccentSounds = Sounds;
		}
	}

	//DEATH
	if (!CanPlayDeathSound() || !ShouldPlayDeathSound(AttackMethod, Strength, AttackForm, Damage))
	{
		TArray<FALSXTCharacterDamageSound> DeathSounds = SelectDeathSounds(Settings, Sex, Variant, Overlay, AttackForm, Strength);
		TArray<FSound> Sounds;
		TArray<TObjectPtr<UObject>> DeathSoundAssets;

		for (FALSXTCharacterDamageSound AC : DeathSounds)
		{
			Sounds.Append(AC.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.VocalSounds = Sounds;
		}
	}

	//WEAPON
	if (IAlsxtCharacterSoundComponentInterface::Execute_ShouldPlayWeaponMovementSound(GetOwner(), Type, Strength))
	{
		TArray<FALSXTWeaponMovementSound> WeaponMovementSounds = SelectWeaponMovementSounds(Settings, ALSXTWeaponTags::M4, Type);
		FName WeaponMovementSoundSocket = GetSocketForMovement(Type);
		MotionSounds.WeaponMovementSocketName = "hand_r";
		TArray<FSound> Sounds;

		for (FALSXTWeaponMovementSound CM : WeaponMovementSounds)
		{
			Sounds.Append(CM.Sounds);
		}

		if (Sounds.IsValidIndex(0))
		{
			MotionSounds.WeaponMovementSounds = Sounds;
		}
	}

	// V2
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Server
		ServerPlaySound(MotionSounds);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy && GetOwner()->GetRemoteRole() == ROLE_Authority)
	{
		// Reg
		PlaySound(MotionSounds);
	}
}

void UAlsxtCharacterSoundComponent::PlaySound(FMotionSounds MotionSounds)
{
	UAlsxtCharacterSoundSettings* Settings = IAlsxtCharacterSoundComponentInterface::Execute_SelectCharacterSoundSettings(GetOwner());
	
	if (GetWorld()->WorldType == EWorldType::EditorPreview)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Play Sound in Editor");
		// UGameplayStatics::PlaySoundAtLocation(GetOwner()->GetWorld(), Sound.Sound.Sound, Location, Rotation, 1.0f, 1.0f);
	}
	else
	{
		bool BreathSound = MotionSounds.BreathSounds.IsValidIndex(0);
		bool CharacterVocalSound = MotionSounds.VocalSounds.IsValidIndex(0);
		bool CharacterMovementSound = MotionSounds.CharacterMovementSounds.IsValidIndex(0);
		bool CharacterMovementAccentSound = MotionSounds.CharacterMovementAccentSounds.IsValidIndex(0);
		bool WeaponMovementSound = MotionSounds.WeaponMovementSounds.IsValidIndex(0);

		//CHARACTER MOVEMENT
		if (CharacterMovementSound || CharacterMovementAccentSound)
		{
			FVector CharacterMovementSocketLocation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(MotionSounds.CharacterMovementSocketName);
			FRotator CharacterMovementSocketRotation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(MotionSounds.CharacterMovementSocketName);
			CharacterMovementSoundMixer = UGameplayStatics::SpawnSoundAtLocation(GetOwner()->GetWorld(), Settings->CharacterMovementSoundMixer, CharacterMovementSocketLocation, CharacterMovementSocketRotation, 1.0f, 1.0f);

			if (IsValid(CharacterMovementSoundMixer))
			{
				CharacterMovementSoundMixer->SetIsReplicated(true);
				CharacterMovementSoundMixer->Stop();
				if (CharacterMovementSound)
				{
					FSound NewCharacterMovementSound;
					DetermineNewSound(MotionSounds.CharacterMovementSounds, PreviousCharacterMovementAssets, NewCharacterMovementSound);
					float NewCharacterMovementSoundPitch = FMath::RandRange(NewCharacterMovementSound.PitchRange.X, NewCharacterMovementSound.PitchRange.Y);
					SetNewSound(NewCharacterMovementSound.Sound, PreviousCharacterMovementAssets, GeneralCharacterSoundSettings.CharacterMovementNoRepeats);
					CurrentCharacterMovementSound = NewCharacterMovementSound;
					CharacterMovementSoundMixer->SetObjectParameter("MovementSound", CurrentCharacterMovementSound.Sound);
				}

				if (CharacterMovementAccentSound)
				{
					FSound NewCharacterMovementAccentSound;
					DetermineNewSound(MotionSounds.CharacterMovementAccentSounds, PreviousCharacterMovementAccentAssets, NewCharacterMovementAccentSound);
					float NewCharacterMovementAccentSoundPitch = FMath::RandRange(NewCharacterMovementAccentSound.PitchRange.X, NewCharacterMovementAccentSound.PitchRange.Y);
					SetNewSound(NewCharacterMovementAccentSound.Sound, PreviousCharacterMovementAccentAssets, GeneralCharacterSoundSettings.CharacterMovementAccentNoRepeats);
					CurrentCharacterMovementAccentSound = NewCharacterMovementAccentSound;
					CharacterMovementSoundMixer->SetObjectParameter("AccentSound", CurrentCharacterMovementAccentSound.Sound);
				}
				CharacterMovementSoundMixer->Play();
				CharacterMovementSoundMixer->SetTriggerParameter("UE.Source.OnPlay");
				GetOwner()->MakeNoise(1.0, Cast<APawn>(GetOwner()), CharacterMovementSocketLocation, 10, "Noise");
			}
		}			

		if (WeaponMovementSound)
		{
			// WEAPON SOUND
			FVector WeaponMovementSocketLocation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(MotionSounds.WeaponMovementSocketName);
			FRotator WeaponMovementSocketRotation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(MotionSounds.WeaponMovementSocketName);
			WeaponMovementAudioComponent = UGameplayStatics::SpawnSoundAtLocation(GetOwner()->GetWorld(), Settings->WeaponMovementMixer, WeaponMovementSocketLocation, WeaponMovementSocketRotation, 10.0f, MotionSounds.WeaponMovementSoundsPitch);
			if (IsValid(WeaponMovementAudioComponent))
			{
				WeaponMovementAudioComponent->SetIsReplicated(true);
				FSound NewWeaponMovementSound;
				DetermineNewSound(MotionSounds.WeaponMovementSounds, PreviousWeaponMovementAssets, NewWeaponMovementSound);
				float NewCharacterMovementSoundPitch = FMath::RandRange(NewWeaponMovementSound.PitchRange.X, NewWeaponMovementSound.PitchRange.Y);
				SetNewSound(NewWeaponMovementSound.Sound, PreviousWeaponMovementAssets, GeneralCharacterSoundSettings.WeaponMovementNoRepeats);
				CurrentVocalizationSound = NewWeaponMovementSound;
				WeaponMovementAudioComponent->SetObjectParameter("WeaponMovementSound", NewWeaponMovementSound.Sound);

				WeaponMovementAudioComponent->Stop();
				WeaponMovementAudioComponent->Play();
				WeaponMovementAudioComponent->SetTriggerParameter("UE.Source.OnPlay");
			}

			
			GetOwner()->MakeNoise(1.0, Cast<APawn>(GetOwner()), WeaponMovementSocketLocation, 10, "Noise");
		}

		// Breath
		if (IsValid(VocalizationMixerAudioComponent) && (BreathSound) && (!CharacterVocalSound))
		{
			if (IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner()).AudibleBreathStaminaLevels.HasTag(IAlsxtCharacterInterface::Execute_GetStaminaTag(GetOwner())))
			{
				FSound NewBreathSound;
				DetermineNewSound(MotionSounds.BreathSounds, PreviousBreathsAssets, NewBreathSound);
				float Pitch = FMath::RandRange(NewBreathSound.PitchRange.X, NewBreathSound.PitchRange.Y);
				CurrentBreathSound = NewBreathSound;
				SetNewSound(NewBreathSound.Sound, PreviousBreathsAssets, GeneralCharacterSoundSettings.BreathNoRepeats);
				FVector NewVoiceSocketLocation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName);
				FRotator NewVoiceSocketRotation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName);
				FSound EmptySound;
				CurrentVocalizationSound = EmptySound;
				VocalizationMixerAudioComponent->SetObjectParameter("VocalizationSound", nullptr);
				VocalizationMixerAudioComponent->SetObjectParameter("BreathSound", NewBreathSound.Sound);
				VocalizationMixerAudioComponent->SetFloatParameter("Pitch", Pitch);

				if (GetOwner()->GetRemoteRole() == ROLE_Authority)
				{
					VocalizationMixerAudioComponent->SetBoolParameter("Player", true);
				}

				VocalizationMixerAudioComponent->SetTriggerParameter("BreathInput");
				GetOwner()->MakeNoise(1.0, Cast<APawn>(GetOwner()), VoiceSocketLocation, 10, "Vocal");
				// OnVocalization(NewBreathSound);
				OnVocalization.Broadcast(NewBreathSound);
			}
			
			if (IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner()).VisibleBreathTypes.HasTag(IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner())))
			{
				// UE_LOG(LogTemp, Warning, TEXT("SelectNewBreathParticles"));
				CurrentBreathParticles = SelectBreathParticles(IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner()), CurrentStaminaTag);
				if (CurrentBreathParticles.IsValidIndex(0))
				{
					// UpdateVoiceSocketRotation();
					UNiagaraSystem* NiagaraSystem{ CurrentBreathParticles[0] };
					// UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner()), GeneralCharacterSoundSettings.VoiceSocketName, IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), IALSXTCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), EAttachLocation::KeepWorldPosition, true, true, ENCPoolMethod::None, true);
					ServerPlayBreathParticle(NiagaraSystem);
				}
			}
			return;
		}

		// VOCAL
		if (IsValid(VocalizationMixerAudioComponent) && (CharacterVocalSound))
		{
			FSound NewVocalizationSound;
			DetermineNewSound(MotionSounds.VocalSounds, PreviousVocalizationsAssets, NewVocalizationSound);
			float Pitch = FMath::RandRange(NewVocalizationSound.PitchRange.X, NewVocalizationSound.PitchRange.Y);
			CurrentVocalizationSound = NewVocalizationSound;
			SetNewSound(NewVocalizationSound.Sound, PreviousVocalizationsAssets, GeneralCharacterSoundSettings.VocalizationNoRepeats);
			FVector NewVoiceSocketLocation = IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName);
			VocalizationMixerAudioComponent->SetObjectParameter("VocalizationSound", NewVocalizationSound.Sound);
			FString SoundName = NewVocalizationSound.Sound->GetName();
			VocalizationMixerAudioComponent->SetFloatParameter("Pitch", Pitch);

			if (GetOwner()->GetRemoteRole() == ROLE_Authority)
			{
				VocalizationMixerAudioComponent->SetBoolParameter("Player", true);
			}

			VocalizationMixerAudioComponent->SetTriggerParameter("VocalizationInput");
			GetOwner()->MakeNoise(1.0, Cast<APawn>(GetOwner()), NewVoiceSocketLocation, 10, "Vocal");
			// OnVocalization(NewVocalizationSound);
			OnVocalization.Broadcast(NewVocalizationSound);

			if (IAlsxtCharacterSoundComponentInterface::Execute_GetBreathEffectsSettings(GetOwner()).VisibleBreathTypes.HasTag(IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner())))
			{
				// UE_LOG(LogTemp, Warning, TEXT("SelectNewBreathParticles"));
				FGameplayTag NewStaminaTag{ ConvertStaminaToStaminaTag(CurrentStamina) };
				CurrentBreathParticles = SelectBreathParticles(IAlsxtCharacterInterface::Execute_GetBreathType(GetOwner()), NewStaminaTag);
				if (CurrentBreathParticles.IsValidIndex(0))
				{
					// UpdateVoiceSocketRotation();
					// UNiagaraSystem* NiagaraSystem{ (CurrentBreathParticles.Num() > 1) ? DetermineNewBreathParticle() : CurrentBreathParticles[0] };
					UNiagaraSystem* NiagaraSystem{ CurrentBreathParticles[0] };
					UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner()), GeneralCharacterSoundSettings.VoiceSocketName, IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketLocation(GeneralCharacterSoundSettings.VoiceSocketName), IAlsxtCharacterInterface::Execute_GetCharacterMesh(GetOwner())->GetSocketRotation(GeneralCharacterSoundSettings.VoiceSocketName), EAttachLocation::KeepWorldPosition, true, true, ENCPoolMethod::None, true);
					// ServerPlayBreathParticle(NiagaraSystem);
				}
			}
			return;
		}
	}
}

void UAlsxtCharacterSoundComponent::ServerPlaySound_Implementation(FMotionSounds MotionSounds)
{
	MulticastPlaySound(MotionSounds);
}

void UAlsxtCharacterSoundComponent::MulticastPlaySound_Implementation(FMotionSounds MotionSounds)
{
	PlaySound(MotionSounds);
}

void UAlsxtCharacterSoundComponent::SpawnAudioComponent(UAudioComponent* AudioComponent, USoundBase* Sound, USceneComponent* Component, FVector Location, FRotator Rotation, float Volume, FName AttachmentSocket)
{
	if (!AttachmentSocket.IsNone())
	{
		VocalizationMixerAudioComponent = UGameplayStatics::SpawnSoundAttached(Sound, Component, AttachmentSocket, Location, EAttachLocation::KeepWorldPosition, true, Volume, 1.0f, 0.0f, nullptr, nullptr, false);

		if (IsValid(VocalizationMixerAudioComponent))
		{
			VocalizationMixerAudioComponent->SetIsReplicated(true);
		}
	}
	else
	{
		AudioComponent = UGameplayStatics::SpawnSoundAtLocation(GetOwner()->GetWorld(), Sound, Location, Rotation, Volume, 1.0f);
		VocalizationMixerAudioComponent->SetIsReplicated(true);
	}
}

void UAlsxtCharacterSoundComponent::ServerSpawnAudioComponent_Implementation(UAudioComponent* AudioComponent, USoundBase* Sound, USceneComponent* Component, FVector Location, FRotator Rotation, float Volume, FName AttachmentSocket)
{
	MulticastSpawnAudioComponent(AudioComponent, Sound, Component, Location, Rotation, Volume, AttachmentSocket);
}

void UAlsxtCharacterSoundComponent::ClientSpawnAudioComponent_Implementation(UAudioComponent* AudioComponent, USoundBase* Sound, USceneComponent* Component, FVector Location, FRotator Rotation, float Volume, FName AttachmentSocket)
{
	SpawnAudioComponent(AudioComponent, Sound, Component, Location, Rotation, Volume, AttachmentSocket);
}

void UAlsxtCharacterSoundComponent::MulticastSpawnAudioComponent_Implementation(UAudioComponent* AudioComponent, USoundBase* Sound, USceneComponent* Component, FVector Location, FRotator Rotation, float Volume, FName AttachmentSocket)
{
	SpawnAudioComponent(AudioComponent, Sound, Component, Location, Rotation, Volume, AttachmentSocket);
}