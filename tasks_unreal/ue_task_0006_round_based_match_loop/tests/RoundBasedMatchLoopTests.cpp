#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

#define protected public
#include "Gameplay/HordeGameState.h"
#undef protected

namespace RoundBasedTests
{
	static constexpr EAutomationTestFlags Flags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	static UWorld* CreateTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		if (World && GEngine)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
		}
		return World;
	}

	static void DestroyTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	static bool IsPropertyReplicated(UClass* InClass, const FName& PropertyName)
	{
		if (!InClass)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			if (FProperty* Property = *It; Property && Property->GetFName() == PropertyName)
			{
				return (Property->GetPropertyFlags() & CPF_Net) != 0;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoundBasedReplicationSurfaceTest,
	"HordeTemplate.RoundBased.replication_surface",
	RoundBasedTests::Flags)

bool FRoundBasedReplicationSurfaceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("GameStatus should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("GameStatus")));
	TestTrue(TEXT("RoundTime should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("RoundTime")));
	TestTrue(TEXT("PauseTime should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("PauseTime")));
	TestTrue(TEXT("IsRoundPaused should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("IsRoundPaused")));
	TestTrue(TEXT("GameRound should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("GameRound")));
	TestTrue(TEXT("ZedsLeft should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("ZedsLeft")));
	TestTrue(TEXT("NextLevel should replicate"), RoundBasedTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("NextLevel")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoundBasedPauseEntryTest,
	"HordeTemplate.RoundBased.pause_phase_entry",
	RoundBasedTests::Flags)

bool FRoundBasedPauseEntryTest::RunTest(const FString& Parameters)
{
	UWorld* World = RoundBasedTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	TestNotNull(TEXT("GameState should spawn"), GameState);
	if (!GameState)
	{
		RoundBasedTests::DestroyTestWorld(World);
		return false;
	}

	GameState->RoundTime = 42.0f;
	GameState->StartRoundBasedGame();
	TestTrue(TEXT("Starting the round-based loop should begin the pause timer"), World->GetTimerManager().IsTimerActive(GameState->PauseTimer));
	TestTrue(TEXT("Starting the round-based loop should mark the match as paused"), GameState->IsRoundPaused);
	TestEqual(TEXT("Starting a pause phase should clear the round timer display"), GameState->RoundTime, 0.0f);

	RoundBasedTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoundBasedMapRotationTest,
	"HordeTemplate.RoundBased.map_rotation_wrap",
	RoundBasedTests::Flags)

bool FRoundBasedMapRotationTest::RunTest(const FString& Parameters)
{
	UWorld* World = RoundBasedTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	TestNotNull(TEXT("GameState should spawn"), GameState);
	if (!GameState)
	{
		RoundBasedTests::DestroyTestWorld(World);
		return false;
	}

	const FName CurrentLevel(*UGameplayStatics::GetCurrentLevelName(World, true));
	GameState->LobbyInformation.LobbyMapRotation = {CurrentLevel, FName(TEXT("SecondMap"))};
	TestEqual(TEXT("Map rotation should advance to the next configured level"), GameState->GetNextLevelInRotation(false), FName(TEXT("SecondMap")));

	GameState->LobbyInformation.LobbyMapRotation = {FName(TEXT("FirstMap")), CurrentLevel};
	TestEqual(TEXT("Map rotation should wrap to the first configured level after the last"), GameState->GetNextLevelInRotation(false), FName(TEXT("FirstMap")));

	TestEqual(TEXT("Reset-level requests should keep the current level"), GameState->GetNextLevelInRotation(true), CurrentLevel);

	RoundBasedTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoundBasedZedsLeftCountTest,
	"HordeTemplate.RoundBased.zeds_left_reflects_living_zed_count",
	RoundBasedTests::Flags)

bool FRoundBasedZedsLeftCountTest::RunTest(const FString& Parameters)
{
	UWorld* World = RoundBasedTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	TestNotNull(TEXT("GameState should spawn"), GameState);
	if (!GameState)
	{
		RoundBasedTests::DestroyTestWorld(World);
		return false;
	}

	// Seed a non-zero value to ensure UpdateAliveZeds actually writes the result.
	GameState->ZedsLeft = 99;
	// In a world with no spawned ZedPawns, the alive count must be zero.
	// A stub that never updates ZedsLeft leaves it at 99.
	GameState->UpdateAliveZeds();
	TestEqual(TEXT("ZedsLeft must be 0 when no ZedPawns exist in the world"), GameState->ZedsLeft, 0);

	RoundBasedTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoundBasedEndRoundClearsTimerTest,
	"HordeTemplate.RoundBased.end_game_round_clears_active_round_timer",
	RoundBasedTests::Flags)

bool FRoundBasedEndRoundClearsTimerTest::RunTest(const FString& Parameters)
{
	UWorld* World = RoundBasedTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	TestNotNull(TEXT("GameState should spawn"), GameState);
	if (!GameState)
	{
		RoundBasedTests::DestroyTestWorld(World);
		return false;
	}

	World->GetTimerManager().SetTimer(GameState->RoundTimer, []() {}, 1.0f, true);
	TestTrue(TEXT("Test setup should activate the round timer"), World->GetTimerManager().IsTimerActive(GameState->RoundTimer));

	GameState->EndGameRound();
	TestFalse(TEXT("Ending a round should always clear the active round timer before branching"), World->GetTimerManager().IsTimerActive(GameState->RoundTimer));

	RoundBasedTests::DestroyTestWorld(World);
	return true;
}
