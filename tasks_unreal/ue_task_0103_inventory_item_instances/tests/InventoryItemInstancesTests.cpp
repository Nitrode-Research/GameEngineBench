#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Inventory/ECRInventoryManagerComponent.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"

namespace ECRInventoryItemInstancesTests
{
static bool LoadProjectFile(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutSource)
{
	const FString AbsolutePath = FPaths::ProjectDir() / RelativePath;
	if (!FFileHelper::LoadFileToString(OutSource, *AbsolutePath))
	{
		Test.AddError(FString::Printf(TEXT("Could not read %s"), RelativePath));
		return false;
	}
	return true;
}

static bool LoadCombined(FAutomationTestBase& Test, const TCHAR* const* RelativePaths, int32 Count, FString& OutCombined)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FString Source;
		if (!LoadProjectFile(Test, RelativePaths[Index], Source))
		{
			return false;
		}
		OutCombined += Source;
		OutCombined += TEXT("\n");
	}
	return true;
}

static bool RequireTokens(FAutomationTestBase& Test, const TCHAR* Label, const FString& Source, const TCHAR* const* Tokens, int32 Count)
{
	bool bAllPresent = true;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const TCHAR* Token = Tokens[Index];
		const bool bPresent = Source.Contains(Token);
		bAllPresent &= Test.TestTrue(FString::Printf(TEXT("%s contains '%s'"), Label, Token), bPresent);
	}
	return bAllPresent;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryItemInstancesSourceContractTokensTest,
	"ECR.InventoryItems.source_contract_tokens_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInventoryItemInstancesSourceContractTokensTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/ECR/Private/Gameplay/Inventory/ECRInventoryItemDefinition.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/ECRInventoryItemInstance.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/ECRInventoryManagerComponent.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/IPickupable.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/InventoryFragment_EquippableItem.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/InventoryFragment_PickupIcon.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/InventoryFragment_QuickBarIcon.cpp"),
		TEXT("Source/ECR/Private/Gameplay/Inventory/InventoryFragment_SetStats.cpp")

	};
	if (!ECRInventoryItemInstancesTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED: these implementation tokens are absent from the stripped baseline and present in the reference solution.
	const TCHAR* RequiredTokens[] = {
		TEXT("DOREPLIFETIME"),
		TEXT("AddStack"),
		TEXT("RemoveStack"),
		TEXT("GetStackCount"),
		TEXT("ContainsTag"),
		TEXT("GetDefault"),
		TEXT("GetItemDef"),
		TEXT("Printf"),
		TEXT("GetNameSafe"),
		TEXT("GetWorld"),
		TEXT("BroadcastMessage"),
		TEXT("GetOwner"),
		TEXT("MarkItemDirty"),
		TEXT("MarkArrayDirty")

	};
	return ECRInventoryItemInstancesTests::RequireTokens(*this, TEXT("inventory_item_instances"), Source, RequiredTokens, UE_ARRAY_COUNT(RequiredTokens));
}

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test: count/consume consistency invariant.
//
// Whatever GetTotalItemCountByDefinition reports for a definition, ConsumeItemsByDefinition
// must be able to consume exactly that many. This invariant holds for ANY self-consistent
// implementation — entry-based (count = number of stacks; consume removes whole entries) OR
// stack-aware (count = sum of stack sizes; consume decrements within stacks). It is violated
// only by a reader that MIXES the two conventions (e.g. a stack-aware count paired with an
// entry-based consume that removes a whole entry per unit): it then reports N for a single
// stack of N but can only consume one, so Consume(N) returns false.
//
// Headless-safe: a plain AActor + UActorComponent in a standalone world (no PlayerController /
// Character / Slate). Standalone => the owner has authority, which ConsumeItemsByDefinition needs.
// ─────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryItemInstancesConsumeMatchesCountTest,
	"ECR.InventoryItems.consume_matches_reported_count",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInventoryItemInstancesConsumeMatchesCountTest::RunTest(const FString& Parameters)
{
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorldContext()->World();
	if (!TestNotNull(TEXT("standalone game world must exist"), World))
	{
		return true;
	}
	World->InitializeActorsForPlay(FURL());

	AActor* Owner = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("owner actor must spawn"), Owner))
	{
		return true;
	}

	UECRInventoryManagerComponent* Inventory = NewObject<UECRInventoryManagerComponent>(Owner);
	Inventory->RegisterComponent();

	TSubclassOf<UECRInventoryItemDefinition> ItemDef = UECRInventoryItemDefinition::StaticClass();

	// Add a single stack of five of the same definition.
	UECRInventoryItemInstance* Added = Inventory->AddItemDefinition(ItemDef, 5);
	TestNotNull(TEXT("AddItemDefinition must create an item instance (gutted baseline returns null)"), Added);

	// The invariant: consume exactly what the count query reports.
	const int32 ReportedCount = Inventory->GetTotalItemCountByDefinition(ItemDef);
	TestTrue(TEXT("reported total count must be positive after adding a stack"), ReportedCount > 0);
	const bool bConsumedAll = Inventory->ConsumeItemsByDefinition(ItemDef, ReportedCount);
	TestTrue(TEXT("must consume exactly the reported total count (count and consume must agree)"), bConsumedAll);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
