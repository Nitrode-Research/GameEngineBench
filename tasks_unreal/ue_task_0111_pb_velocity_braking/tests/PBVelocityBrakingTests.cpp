#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Character/PBPlayerCharacter.h"
#include "Character/PBPlayerMovement.h"

namespace PBMovementTest
{
	static UWorld* CreateWorld()
	{
		const FName WorldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("PBVelocityWorld"));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPBVelocityBrakingSmokeTest, "PB.Velocity.BrakingReducesSpeed", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPBVelocityBrakingSmokeTest::RunTest(const FString& Parameters)
{
	UWorld* World = PBMovementTest::CreateWorld();
	APBPlayerCharacter* Character = World->SpawnActor<APBPlayerCharacter>();
	UPBPlayerMovement* Move = Cast<UPBPlayerMovement>(Character->GetCharacterMovement());
	TestNotNull(TEXT("PB character uses PB movement component"), Move);
	Move->Velocity = FVector(600.0, 0.0, 0.0);
	Move->SetMovementMode(MOVE_Walking);
	Move->ApplyVelocityBraking(1.0f / 30.0f, 4.0f, 0.0f);
	TestTrue(TEXT("ApplyVelocityBraking reduces speed"), Move->Velocity.Size() < 600.0f);
	PBMovementTest::DestroyWorld(World);
	return true;
}
