// Copyright (c) Yevhenii Selivanov

#include "Subsystems/BmrSoundsSubsystem.h"

#include "DataAssets/BmrSoundsDataAsset.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrSoundsSubsystem)

UBmrSoundsSubsystem& UBmrSoundsSubsystem::Get()
{
	UBmrSoundsSubsystem* SoundsSubsystem = GetSoundsSubsystem();
	checkf(SoundsSubsystem, TEXT("%s: 'SoundsSubsystem' is null"), *FString(__FUNCTION__));
	return *SoundsSubsystem;
}

UBmrSoundsSubsystem* UBmrSoundsSubsystem::GetSoundsSubsystem(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : (GEngine ? GEngine->GetCurrentPlayWorld() : nullptr);
	return World ? World->GetSubsystem<UBmrSoundsSubsystem>() : nullptr;
}

bool UBmrSoundsSubsystem::CanPlaySounds()
{
	return false;
}

void UBmrSoundsSubsystem::PlaySingleSound2D(USoundBase* InSound)
{
}

void UBmrSoundsSubsystem::StopSingleSound2D(USoundBase* InSound)
{
}

void UBmrSoundsSubsystem::DestroySingleSound2D(USoundBase* InSound)
{
}

void UBmrSoundsSubsystem::DestroyAllSoundComponents()
{
	SoundComponents.Reset();
}

void UBmrSoundsSubsystem::SetSoundVolumeByClass(USoundClass* InSoundClass, float InVolume)
{
}

void UBmrSoundsSubsystem::SetMasterVolume(double InVolume)
{
}

void UBmrSoundsSubsystem::SetMusicVolume(double InVolume)
{
}

void UBmrSoundsSubsystem::SetSFXVolume(double InVolume)
{
}

void UBmrSoundsSubsystem::PlayInGameMusic()
{
}

void UBmrSoundsSubsystem::StopInGameMusic()
{
}

void UBmrSoundsSubsystem::PlayEndGameCountdownSFX()
{
}

void UBmrSoundsSubsystem::StopEndGameCountdownSFX()
{
}

void UBmrSoundsSubsystem::PlayStartGameCountdownSFX()
{
}

void UBmrSoundsSubsystem::StopStartGameCountdownSFX()
{
}

void UBmrSoundsSubsystem::PlayUIClickSFX()
{
}

void UBmrSoundsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void UBmrSoundsSubsystem::Deinitialize()
{
	DestroyAllSoundComponents();
	Super::Deinitialize();
}

void UBmrSoundsSubsystem::OnEndGameStateChanged_Implementation(EBmrEndGameState EndGameState)
{
}

void UBmrSoundsSubsystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
}

void UBmrSoundsSubsystem::OnLocalPlayerStateReady_Implementation(const FGameplayEventData& Payload)
{
}

void UBmrSoundsSubsystem::OnSoundRowsChanged_Implementation()
{
}
