// Copyright 2026 GameDevBench. Sounds subsystem automation tests (Bomber).

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

#include "Subsystems/BmrSoundsSubsystem.h"

namespace BomberSoundsSubsystemTests
{
	static const TCHAR* MapPath = TEXT("/Game/Bomber/Maps/Main");

	static UWorld* GetPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	static bool LoadSubsystemSource(FString& OutSource)
	{
		const FString SourcePath = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("Source/Bomber/Private/Subsystems/BmrSoundsSubsystem.cpp"));
		return FFileHelper::LoadFileToString(OutSource, *SourcePath);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberSounds_VolumeSettersUpdateSubsystemState,
	"Bomber.SoundsSubsystem.volume_setters_update_subsystem_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberSounds_VolumeSettersUpdateSubsystemState::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	// REQUIRED: validates config-backed runtime volume state independent of audible playback availability.
	using namespace BomberSoundsSubsystemTests;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = BomberSoundsSubsystemTests::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		UBmrSoundsSubsystem* SoundsSubsystem = World->GetSubsystem<UBmrSoundsSubsystem>();
		if (!TestNotNull(TEXT("Sounds subsystem must exist on the PIE world"), SoundsSubsystem))
		{
			return true;
		}

		SoundsSubsystem->SetMasterVolume(0.42);
		SoundsSubsystem->SetMusicVolume(0.37);
		SoundsSubsystem->SetSFXVolume(0.73);

		TestTrue(TEXT("SetMasterVolume must update the stored master volume"),
			FMath::IsNearlyEqual(SoundsSubsystem->GetMasterVolume(), 0.42));
		TestTrue(TEXT("SetMusicVolume must update the stored music volume"),
			FMath::IsNearlyEqual(SoundsSubsystem->GetMusicVolume(), 0.37));
		TestTrue(TEXT("SetSFXVolume must update the stored SFX volume"),
			FMath::IsNearlyEqual(SoundsSubsystem->GetSFXVolume(), 0.73));
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
#else
	AddWarning(TEXT("Sounds subsystem runtime test requires WITH_EDITOR PIE support."));
	return true;
#endif
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberSounds_SingletonSoundComponentLifecycleSource,
	"Bomber.SoundsSubsystem.singleton_sound_component_lifecycle_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberSounds_SingletonSoundComponentLifecycleSource::RunTest(const FString& Parameters)
{
	// PARTIAL: audio playback is engine-gated in headless runs, so verify the singleton component lifecycle implementation paths.
	FString Source;
	if (!TestTrue(TEXT("BmrSoundsSubsystem.cpp must be readable from the project source tree"),
		BomberSoundsSubsystemTests::LoadSubsystemSource(Source)))
	{
		return true;
	}

	TestTrue(TEXT("PlaySingleSound2D must cache components by sound asset"),
		Source.Contains(TEXT("SoundComponents.Add")));
	TestTrue(TEXT("Singleton sound components must disable auto-destroy so they can be reused"),
		Source.Contains(TEXT("bAutoDestroy = false")));
	TestTrue(TEXT("PlaySingleSound2D must stop an already-playing component before replay"),
		Source.Contains(TEXT("SoundComponentRef.IsPlaying()")) && Source.Contains(TEXT("SoundComponentRef.Stop()")));
	TestTrue(TEXT("DestroySingleSound2D must destroy the cached audio component"),
		Source.Contains(TEXT("DestroyComponent()")));
	TestTrue(TEXT("DestroySingleSound2D must remove the sound from the component map"),
		Source.Contains(TEXT("SoundComponents.Remove")));
	TestTrue(TEXT("DestroyAllSoundComponents must clear all cached sound components"),
		Source.Contains(TEXT("SoundComponents.Empty()")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberSounds_WorldAndGameStateRoutingSource,
	"Bomber.SoundsSubsystem.world_and_game_state_routing_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberSounds_WorldAndGameStateRoutingSource::RunTest(const FString& Parameters)
{
	// PARTIAL: verifies lifecycle/message/registry routing that is difficult to observe without full game state.
	FString Source;
	if (!TestTrue(TEXT("BmrSoundsSubsystem.cpp must be readable from the project source tree"),
		BomberSoundsSubsystemTests::LoadSubsystemSource(Source)))
	{
		return true;
	}

	TestTrue(TEXT("OnWorldBeginPlay must set the base sound mix"),
		Source.Contains(TEXT("SetBaseSoundMix")));
	TestTrue(TEXT("OnWorldBeginPlay must reapply persisted volume settings"),
		Source.Contains(TEXT("SetMasterVolume(MasterVolume)"))
		&& Source.Contains(TEXT("SetMusicVolume(MusicVolume)"))
		&& Source.Contains(TEXT("SetSFXVolume(SFXVolume)")));
	TestTrue(TEXT("Subsystem must listen for local pawn ready messages"),
		Source.Contains(TEXT("Player_LocalPawnReady")) && Source.Contains(TEXT("OnLocalPlayerStateReady")));
	TestTrue(TEXT("Subsystem must listen for game-state changed messages"),
		Source.Contains(TEXT("GameState_Changed")) && Source.Contains(TEXT("OnGameStateChanged")));
	TestTrue(TEXT("Subsystem must bind and load background music registry rows"),
		Source.Contains(TEXT("BindAndLoad<FBmrSoundsBackgroundRow>")));
	TestTrue(TEXT("GameStarting must trigger in-game music and start countdown SFX"),
		Source.Contains(TEXT("FBmrGameStateTag::GameStarting"))
		&& Source.Contains(TEXT("PlayInGameMusic()"))
		&& Source.Contains(TEXT("PlayStartGameCountdownSFX()")));
	TestTrue(TEXT("Menu state must stop in-game music and start countdown SFX"),
		Source.Contains(TEXT("FBmrGameStateTag::Menu"))
		&& Source.Contains(TEXT("StopInGameMusic()"))
		&& Source.Contains(TEXT("StopStartGameCountdownSFX()")));
	TestTrue(TEXT("Local player ready must bind to the end-game state delegate"),
		Source.Contains(TEXT("OnEndGameStateChanged.AddUniqueDynamic")));
	return true;
}
