#pragma once

#include "AudioMixerBlueprintLibrary.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectEQ.h"
#include "SubmixEffects/SubmixEffectMultiBandCompressor.h"
#include "SubmixEffects/SubmixEffectFilter.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Utility/TVStructs.h"
#include "AudioInterface.generated.h"

UINTERFACE(Blueprintable)
class UAudioInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IAudioInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetAllLevelsToDefault();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetMainLevel(float& MainLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetMainLevel(float MainLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoMainLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetMainLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetAudioDescriptionLevel(float& AudioDescriptionLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetAudioDescriptionLevel(float AudioDescriptionLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoAudioDescriptionLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetAudioDescriptionLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetHeadsetLevel(float& HeadsetLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetHeadsetLevel(float HeadsetLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoHeadsetLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetHeadsetLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetSFXLevel(float& SFXLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetSFXLevel(float SFXLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoSFXLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetSFXLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetMusicLevel(float& MusicLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetMusicLevel(float MusicLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoMusicLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetMusicLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetDialogLevel(float& DialogLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetDialogLevel(float DialogLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoDialogLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetDialogLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void GetVOIPLevel(float& VOIPLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void SetVOIPLevel(float VOIPLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void DemoVOIPLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Levels")
	void ResetVOIPLevel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetMainSubmix(USoundSubmix*& MainSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetHeadsetSubmix(USoundSubmix*& HeadsetSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetHeadsetCompressionSubmix(USoundSubmix*& HeadsetCompressionSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetHeadsetCompressionSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetHeadsetCompressionSubmixSettings(const FSubmixEffectMultibandCompressorSettings Settings);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetHeadsetCompressionSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetHeadsetEQSubmix(USoundSubmix*& HeadsetEQSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetHeadsetEQSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetHeadsetEQSubmixSettings(const FSubmixEffectSubmixEQSettings Settings);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetHeadsetEQSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetAudioDescriptionSubmix(USoundSubmix*& AudioDescriptionSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetCompressionSubmix(USoundSubmix*& CompressionSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetCompressionSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetCompressionSubmixSettings(const FSubmixEffectMultibandCompressorSettings Settings);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetCompressionSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetEQSubmix(USoundSubmix*& EQSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetEQSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetEQSubmixSettings(const FSubmixEffectSubmixEQSettings Settings);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetEQSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetDamageEffectSubmix(USoundSubmix*& DamageEffectSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetDamageEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetDamageEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetSuppressionEffectSubmix(USoundSubmix*& SuppressionEffectSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetSuppressionEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetSuppressionEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetPausedEffectSubmix(USoundSubmix*& PausedEffectSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetPausedEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetPausedEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetMusicSubmix(USoundSubmix*& MusicSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetEarProEffectSubmix(USoundSubmix*& GetEarProSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetEarProEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetEarProSubmixSettings(const FSubmixEffectSubmixEQSettings Settings);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetEarProSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetPOVEffectSubmix(USoundSubmix*& GetPOVEffectSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetPOVEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetPOVEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetReverbSubmix(USoundSubmix*& ReverbSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetSFXSubmix(USoundSubmix*& SFXSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetDialogRadioEffectSubmix(USoundSubmix*& GetDialogRadioSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetDialogRadioEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetDialogRadioEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetDialogSubmix(USoundSubmix*& GetDialogSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetVOIPRadioEffectSubmix(USoundSubmix*& GetVOIPRadioSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void SetVOIPRadioEffectSubmix(float Mix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void ResetVOIPRadioEffectSubmix();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Audio|Submix")
	void GetVOIPSubmix(USoundSubmix*& GetVOIPSubmix);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Music|Playback")
	void PlaySound2D(FSound Music, bool Loop, UAudioComponent*& AudioComponent);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Music|Playback")
	void PlaySoundAtLocation(FSound Music, FVector Location, FRotator Rotation, UAudioComponent*& AudioComponent);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game Instance|Music|Playback")
	void PlaySoundAttached(FSound Music, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, FRotator Rotation, UAudioComponent*& AudioComponent);

};