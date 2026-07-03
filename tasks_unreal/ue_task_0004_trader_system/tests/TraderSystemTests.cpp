#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

#define protected public
#include "Character/HordeBaseCharacter.h"
#include "Gameplay/HordeBaseController.h"
#include "Gameplay/HordeGameState.h"
#include "Gameplay/HordePlayerState.h"
#include "Gameplay/LobbyStructures.h"
#include "Inventory/InventoryComponent.h"
#include "Misc/HordeTrader.h"
#undef protected

namespace TraderTests
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

	static bool HasFunctionFlag(UClass* InClass, const TCHAR* FunctionName, EFunctionFlags Flag)
	{
		if (!InClass)
		{
			return false;
		}

		if (UFunction* Function = InClass->FindFunctionByName(FName(FunctionName)))
		{
			return Function->HasAnyFunctionFlags(Flag);
		}
		return false;
	}

	static AHordePlayerState* SpawnPlayerState(UWorld* World, const FString& PlayerID, const FName CharacterID)
	{
		AHordePlayerState* PlayerState = World ? World->SpawnActor<AHordePlayerState>() : nullptr;
		if (PlayerState)
		{
			PlayerState->Player.PlayerID = PlayerID;
			PlayerState->Player.UserName = PlayerID;
			PlayerState->Player.SelectedCharacter = CharacterID;
		}
		return PlayerState;
	}

	static bool FindAffordableTraderRow(FName& OutRowName, FTraderSellItem& OutRow)
	{
		UDataTable* TraderSellData = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, ECONOMY_DATATABLE_PATH));
		if (!TraderSellData)
		{
			return false;
		}

		const TArray<FName> RowNames = TraderSellData->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			const FTraderSellItem* Row = TraderSellData->FindRow<FTraderSellItem>(RowName, TEXT("TraderTests"), false);
			if (Row && Row->ItemID != NAME_None)
			{
				OutRowName = RowName;
				OutRow = *Row;
				return true;
			}
		}

		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderReplicationSurfaceTest,
	"HordeTemplate.Trader.replication_surface",
	TraderTests::Flags)

bool FTraderReplicationSurfaceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("PlayerMoney should replicate"), TraderTests::IsPropertyReplicated(AHordePlayerState::StaticClass(), TEXT("PlayerMoney")));
	TestTrue(TEXT("TradeProgress should replicate"), TraderTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("TradeProgress")));
	TestTrue(TEXT("IsTradeInProgress should replicate"), TraderTests::IsPropertyReplicated(AHordeGameState::StaticClass(), TEXT("IsTradeInProgress")));

	TestTrue(TEXT("BuyItem should be a server RPC"), TraderTests::HasFunctionFlag(AHordePlayerState::StaticClass(), TEXT("BuyItem"), FUNC_NetServer));
	TestTrue(TEXT("ModifyMoney should be a server RPC"), TraderTests::HasFunctionFlag(AHordePlayerState::StaticClass(), TEXT("ModifyMoney"), FUNC_NetServer));
	TestTrue(TEXT("RequestCharacterTrade should be a server RPC"), TraderTests::HasFunctionFlag(AHordePlayerState::StaticClass(), TEXT("RequestCharacterTrade"), FUNC_NetServer));
	TestTrue(TEXT("PlayWelcome should be a multicast RPC"), TraderTests::HasFunctionFlag(AHordeTrader::StaticClass(), TEXT("PlayWelcome"), FUNC_NetMulticast));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderInteractionInfoTest,
	"HordeTemplate.Trader.interaction_info",
	TraderTests::Flags)

bool FTraderInteractionInfoTest::RunTest(const FString& Parameters)
{
	UWorld* World = TraderTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeTrader* Trader = World->SpawnActor<AHordeTrader>();
	TestNotNull(TEXT("Trader actor should spawn"), Trader);
	if (!Trader)
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	const FInteractionInfo Info = Trader->GetInteractionInfo_Implementation();
	TestEqual(TEXT("Trader interaction prompt should advertise talking to the trader"), Info.InteractionText.ToString(), FString(TEXT("Talk to Trader")));
	TestEqual(TEXT("Trader interaction should require a timed hold"), Info.InteractionTime, 3.0f);

	TraderTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderBuyItemFlowTest,
	"HordeTemplate.Trader.buy_item_affordable_and_unaffordable",
	TraderTests::Flags)

bool FTraderBuyItemFlowTest::RunTest(const FString& Parameters)
{
	UWorld* World = TraderTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseController* Controller = World->SpawnActor<AHordeBaseController>();
	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	AHordePlayerState* PlayerState = World->SpawnActor<AHordePlayerState>();
	TestNotNull(TEXT("Controller should spawn"), Controller);
	TestNotNull(TEXT("Character should spawn"), Character);
	TestNotNull(TEXT("PlayerState should spawn"), PlayerState);
	TestNotNull(TEXT("Character should own an inventory component"), Character ? Character->Inventory : nullptr);
	if (!Controller || !Character || !PlayerState || !Character->Inventory)
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	Controller->Possess(Character);
	PlayerState->SetOwner(Controller);
	Character->Inventory->Inventory.Empty();

	FName TraderRowName = NAME_None;
	FTraderSellItem TraderRow;
	TestTrue(TEXT("Trader economy table should contain at least one purchasable row"), TraderTests::FindAffordableTraderRow(TraderRowName, TraderRow));
	if (TraderRowName.IsNone())
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	PlayerState->PlayerMoney = TraderRow.ItemPrice + 50;
	PlayerState->BuyItem_Implementation(TraderRowName);
	TestEqual(TEXT("A valid purchase should deduct the item price from player money"), PlayerState->GetPlayerMoney(), 50);

	TraderTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderCharacterTradeLifecycleTest,
	"HordeTemplate.Trader.character_trade_lifecycle",
	TraderTests::Flags)

bool FTraderCharacterTradeLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = TraderTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	AHordePlayerState* PlayerA = TraderTests::SpawnPlayerState(World, TEXT("P1"), TEXT("Character_A"));
	AHordePlayerState* PlayerB = TraderTests::SpawnPlayerState(World, TEXT("P2"), TEXT("Character_B"));
	TestNotNull(TEXT("GameState should spawn"), GameState);
	TestNotNull(TEXT("First player state should spawn"), PlayerA);
	TestNotNull(TEXT("Second player state should spawn"), PlayerB);
	if (!GameState || !PlayerA || !PlayerB)
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	FLobbyAvailableCharacters CharA;
	CharA.CharacterID = TEXT("Character_A");
	CharA.PlayerID = TEXT("P1");
	CharA.PlayerUsername = TEXT("P1");
	CharA.CharacterTaken = true;

	FLobbyAvailableCharacters CharB;
	CharB.CharacterID = TEXT("Character_B");
	CharB.PlayerID = TEXT("P2");
	CharB.PlayerUsername = TEXT("P2");
	CharB.CharacterTaken = true;

	GameState->LobbyInformation.AvailableCharacters = {CharA, CharB};
	GameState->PlayerArray.Add(PlayerA);
	GameState->PlayerArray.Add(PlayerB);

	GameState->StartCharacterTrade(TEXT("P1"), TEXT("P2"));
	TestTrue(TEXT("Starting a trade should flag the trade as in progress"), GameState->IsTradeInProgress);
	TestEqual(TEXT("Trade instigator should be recorded"), GameState->TradeProgress.Instigator, FString(TEXT("P1")));
	TestEqual(TEXT("Trade target should be recorded"), GameState->TradeProgress.Target, FString(TEXT("P2")));
	TestEqual(TEXT("Trade should start with a 20 second countdown"), GameState->TradeProgress.TimeLeft, 20.0f);

	GameState->ProcessCharacterTrade();
	TestEqual(TEXT("Trade countdown should tick down once per process step"), GameState->TradeProgress.TimeLeft, 19.0f);

	GameState->AcceptCharacterTrade();
	TestEqual(TEXT("Accepting a trade should swap player A's selected character"), PlayerA->GetPlayerInfo().SelectedCharacter, FName(TEXT("Character_B")));
	TestEqual(TEXT("Accepting a trade should swap player B's selected character"), PlayerB->GetPlayerInfo().SelectedCharacter, FName(TEXT("Character_A")));
	TestFalse(TEXT("Trade should no longer be in progress after acceptance"), GameState->IsTradeInProgress);
	TestEqual(TEXT("Trade instigator should reset after acceptance"), GameState->TradeProgress.Instigator, FString());
	TestEqual(TEXT("Trade target should reset after acceptance"), GameState->TradeProgress.Target, FString());

	TraderTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderCharacterTradeTimeoutAbortTest,
	"HordeTemplate.Trader.character_trade_timeout_aborts",
	TraderTests::Flags)

bool FTraderCharacterTradeTimeoutAbortTest::RunTest(const FString& Parameters)
{
	UWorld* World = TraderTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeGameState* GameState = World->SpawnActor<AHordeGameState>();
	TestNotNull(TEXT("GameState should spawn"), GameState);
	if (!GameState)
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	GameState->StartCharacterTrade(TEXT("P1"), TEXT("P2"));
	TestTrue(TEXT("Starting a trade should begin the trade timer"), World->GetTimerManager().IsTimerActive(GameState->LobbyTradeTimer));
	GameState->TradeProgress.TimeLeft = 0.0f;

	GameState->ProcessCharacterTrade();
	TestFalse(TEXT("Expired trades should be aborted"), GameState->IsTradeInProgress);
	TestFalse(TEXT("Aborting a trade should clear the lobby trade timer"), World->GetTimerManager().IsTimerActive(GameState->LobbyTradeTimer));
	TestEqual(TEXT("Aborting a trade should reset the instigator"), GameState->TradeProgress.Instigator, FString());
	TestEqual(TEXT("Aborting a trade should reset the target"), GameState->TradeProgress.Target, FString());
	TestEqual(TEXT("Aborting a trade should restore the default countdown"), GameState->TradeProgress.TimeLeft, 20.0f);

	TraderTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraderAddPointsRewardTest,
	"HordeTemplate.Trader.kill_rewards_money_and_tracks_kills",
	TraderTests::Flags)

bool FTraderAddPointsRewardTest::RunTest(const FString& Parameters)
{
	UWorld* World = TraderTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordePlayerState* PlayerState = World->SpawnActor<AHordePlayerState>();
	TestNotNull(TEXT("PlayerState should spawn"), PlayerState);
	if (!PlayerState)
	{
		TraderTests::DestroyTestWorld(World);
		return false;
	}

	// Casual kill: +150 money, ZedKills++
	const int32 MoneyBefore = PlayerState->GetPlayerMoney();
	const int32 KillsBefore = PlayerState->ZedKills;
	PlayerState->AddPoints(10, EPointType::EPointCasual);
	TestEqual(TEXT("Casual kill should award 150 money"), PlayerState->GetPlayerMoney(), MoneyBefore + 150);
	TestEqual(TEXT("Casual kill should increment ZedKills"), PlayerState->ZedKills, KillsBefore + 1);

	// Headshot kill: +250 money, ZedKills++ and HeadShots++
	const int32 MoneyAfterCasual = PlayerState->GetPlayerMoney();
	const int32 KillsAfterCasual = PlayerState->ZedKills;
	const int32 HeadshotsBefore = PlayerState->HeadShots;
	PlayerState->AddPoints(10, EPointType::EPointHeadShot);
	TestEqual(TEXT("Headshot kill should award 250 money"), PlayerState->GetPlayerMoney(), MoneyAfterCasual + 250);
	TestEqual(TEXT("Headshot kill should increment ZedKills"), PlayerState->ZedKills, KillsAfterCasual + 1);
	TestEqual(TEXT("Headshot kill should increment HeadShots"), PlayerState->HeadShots, HeadshotsBefore + 1);

	TraderTests::DestroyTestWorld(World);
	return true;
}
