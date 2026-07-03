#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

#define protected public
#include "Misc/Door.h"
#include "Misc/EndGameTrigger.h"
#include "Misc/SafeZoneDoor.h"
#undef protected

namespace InteractionDoorTests
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
	FInteractionDoorReplicationSurfaceTest,
	"HordeTemplate.InteractionDoor.replication_surface",
	InteractionDoorTests::Flags)

bool FInteractionDoorReplicationSurfaceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Normal door open state should replicate"), InteractionDoorTests::IsPropertyReplicated(ADoor::StaticClass(), TEXT("bIsOpen")));
	TestTrue(TEXT("Safe zone door open state should replicate"), InteractionDoorTests::IsPropertyReplicated(ASafeZoneDoor::StaticClass(), TEXT("bIsOpen")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDoorTogglePromptTest,
	"HordeTemplate.InteractionDoor.normal_door_toggle",
	InteractionDoorTests::Flags)

bool FDoorTogglePromptTest::RunTest(const FString& Parameters)
{
	UWorld* World = InteractionDoorTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	ADoor* Door = World->SpawnActor<ADoor>();
	TestNotNull(TEXT("Door actor should spawn"), Door);
	if (!Door)
	{
		InteractionDoorTests::DestroyTestWorld(World);
		return false;
	}

	FInteractionInfo Info = Door->GetInteractionInfo_Implementation();
	TestFalse(TEXT("A closed door should expose an interaction prompt"), Info.InteractionText.ToString().IsEmpty());
	TestTrue(TEXT("Door interaction should require a positive timed hold"), Info.InteractionTime > 0.0f);
	TestTrue(TEXT("Closed doors should be interactable"), Info.AllowedToInteract);

	Door->Interact_Implementation(nullptr);
	TestTrue(TEXT("Interacting with a normal door should open it"), Door->bIsOpen);

	Info = Door->GetInteractionInfo_Implementation();
	TestFalse(TEXT("An open door should expose an interaction prompt"), Info.InteractionText.ToString().IsEmpty());

	Door->Interact_Implementation(nullptr);
	TestFalse(TEXT("Interacting again should close the door"), Door->bIsOpen);

	InteractionDoorTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSafeZoneDoorPermanentOpenTest,
	"HordeTemplate.InteractionDoor.safe_zone_permanent_open",
	InteractionDoorTests::Flags)

bool FSafeZoneDoorPermanentOpenTest::RunTest(const FString& Parameters)
{
	UWorld* World = InteractionDoorTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	ASafeZoneDoor* Door = World->SpawnActor<ASafeZoneDoor>();
	TestNotNull(TEXT("Safe zone door actor should spawn"), Door);
	if (!Door)
	{
		InteractionDoorTests::DestroyTestWorld(World);
		return false;
	}

	FInteractionInfo Info = Door->GetInteractionInfo_Implementation();
	TestFalse(TEXT("A closed safe zone door should expose an interaction prompt"), Info.InteractionText.ToString().IsEmpty());
	TestTrue(TEXT("Safe zone door interaction should require a positive timed hold"), Info.InteractionTime > 0.0f);

	Door->Interact_Implementation(nullptr);
	TestTrue(TEXT("First interaction should permanently open the safe zone door"), Door->bIsOpen);

	Door->Interact_Implementation(nullptr);
	TestTrue(TEXT("Further interactions should keep the safe zone door open"), Door->bIsOpen);

	InteractionDoorTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndGameTriggerSurfaceTest,
	"HordeTemplate.InteractionDoor.end_game_trigger_surface",
	InteractionDoorTests::Flags)

bool FEndGameTriggerSurfaceTest::RunTest(const FString& Parameters)
{
	UWorld* World = InteractionDoorTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AEndGameTrigger* Trigger = World->SpawnActor<AEndGameTrigger>();
	TestNotNull(TEXT("End-game trigger should spawn"), Trigger);
	TestNotNull(TEXT("End-game trigger should own a collision box"), Trigger ? Trigger->BoxComponent : nullptr);
	TestEqual(TEXT("End-game trigger should use the collision box as a child of the root"), Trigger && Trigger->BoxComponent ? Trigger->BoxComponent->GetAttachParent() : nullptr, Trigger ? Trigger->GetRootComponent() : nullptr);
	TestTrue(TEXT("End-game trigger collision should generate overlap events"), Trigger && Trigger->BoxComponent ? Trigger->BoxComponent->GetGenerateOverlapEvents() : false);

	InteractionDoorTests::DestroyTestWorld(World);
	return true;
}
