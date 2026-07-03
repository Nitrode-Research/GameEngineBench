#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Character/PBPlayerCharacter.h"
#include "Character/PBPlayerMovement.h"

namespace PBNoClipTest
{
	static UWorld* CreateWorld()
	{
		const FName WorldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("PBNoClipWorld"));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		return World;
	}

	static void DestroyWorld(UWorld* World)
	{
		if (!World) return;
		World->BeginTearingDown();
		World->CleanupWorld();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPBNoClipToggleTest, "PB.NoClip.ToggleChangesMovementMode", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPBNoClipToggleTest::RunTest(const FString& Parameters)
{
	UWorld* World = PBNoClipTest::CreateWorld();
	APBPlayerCharacter* Character = World->SpawnActor<APBPlayerCharacter>();
	UPBPlayerMovement* Move = Cast<UPBPlayerMovement>(Character->GetCharacterMovement());
	TestNotNull(TEXT("PB character uses PB movement component"), Move);
	Move->SetNoClip(true);
	TestEqual(TEXT("NoClip enables flying mode"), Move->MovementMode, MOVE_Flying);
	TestFalse(TEXT("NoClip disables actor collision"), Character->GetActorEnableCollision());
	Move->SetNoClip(false);
	TestEqual(TEXT("NoClip disables back to walking mode"), Move->MovementMode, MOVE_Walking);
	TestTrue(TEXT("NoClip re-enables actor collision"), Character->GetActorEnableCollision());
	PBNoClipTest::DestroyWorld(World);
	return true;
}
