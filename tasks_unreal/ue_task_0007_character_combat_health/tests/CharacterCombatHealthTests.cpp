#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#define protected public
#include "Character/HordeBaseCharacter.h"
#undef protected

namespace CharacterCombatTests
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

	static bool InvokeNoParamUFunction(UObject* Object, const FName FunctionName)
	{
		if (!Object)
		{
			return false;
		}

		UFunction* Function = Object->FindFunction(FunctionName);
		if (!Function)
		{
			return false;
		}

		Object->ProcessEvent(Function, nullptr);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterCombatDamageAndDeathTest,
	"HordeTemplate.CharacterCombat.damage_and_death",
	CharacterCombatTests::Flags)

bool FCharacterCombatDamageAndDeathTest::RunTest(const FString& Parameters)
{
	UWorld* World = CharacterCombatTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character)
	{
		CharacterCombatTests::DestroyTestWorld(World);
		return false;
	}

	Character->TakeDamage(30.0f, FDamageEvent(), nullptr, nullptr);
	TestEqual(TEXT("Taking damage should reduce health"), Character->GetHealth(), 70.0f);
	TestFalse(TEXT("Non-lethal damage should not mark the character dead"), Character->GetIsDead());

	Character->TakeDamage(100.0f, FDamageEvent(), nullptr, nullptr);
	TestTrue(TEXT("Lethal damage should mark the character dead"), Character->GetIsDead());
	TestEqual(TEXT("Lethal damage should clamp health at zero"), Character->GetHealth(), 0.0f);
	TestTrue(TEXT("Death should enable ragdoll physics on the mesh"), Character->GetMesh()->IsSimulatingPhysics());
	TestEqual(TEXT("Death should move the capsule to the dead collision profile"), Character->GetCapsuleComponent()->GetCollisionProfileName(), FName(TEXT("DeadAI")));
	TestEqual(TEXT("Death should transition the character into the post-death lifespan window"), Character->GetLifeSpan(), 20.0f);

	const float HealthAfterDeath = Character->GetHealth();
	Character->TakeDamage(50.0f, FDamageEvent(), nullptr, nullptr);
	TestEqual(TEXT("Already-dead characters should ignore further damage"), Character->GetHealth(), HealthAfterDeath);

	CharacterCombatTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterCombatHealingClampTest,
	"HordeTemplate.CharacterCombat.healing_clamp",
	CharacterCombatTests::Flags)

bool FCharacterCombatHealingClampTest::RunTest(const FString& Parameters)
{
	UWorld* World = CharacterCombatTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character)
	{
		CharacterCombatTests::DestroyTestWorld(World);
		return false;
	}

	Character->TakeDamage(40.0f, FDamageEvent(), nullptr, nullptr);
	TestEqual(TEXT("Damage setup should reduce health"), Character->GetHealth(), 60.0f);

	Character->AddHealth(25.0f);
	TestEqual(TEXT("Healing should restore health"), Character->GetHealth(), 85.0f);

	Character->AddHealth(100.0f);
	TestEqual(TEXT("Healing should never exceed max health"), Character->GetHealth(), 100.0f);

	CharacterCombatTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterCombatSprintLifecycleTest,
	"HordeTemplate.CharacterCombat.sprint_lifecycle",
	CharacterCombatTests::Flags)

bool FCharacterCombatSprintLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = CharacterCombatTests::CreateTestWorld();
	TestNotNull(TEXT("Synthetic world should be created"), World);
	if (!World)
	{
		return false;
	}

	AHordeBaseCharacter* Character = World->SpawnActor<AHordeBaseCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character)
	{
		CharacterCombatTests::DestroyTestWorld(World);
		return false;
	}

	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Character->GetCharacterMovement()->Velocity = FVector(200.0f, 0.0f, 0.0f);
	Character->Stamina = 10.0f;
	Character->ServerStartSprinting_Implementation();

	TestTrue(TEXT("Server sprint request should enter sprinting when moving on the ground"), Character->IsSprinting);
	TestEqual(TEXT("Sprinting should raise movement speed"), Character->GetCharacterMovement()->MaxWalkSpeed, 600.0f);

	TestTrue(TEXT("DecreaseStamina should be exposed as a UFUNCTION"),
		CharacterCombatTests::InvokeNoParamUFunction(Character, FName(TEXT("DecreaseStamina"))));
	TestTrue(TEXT("Stamina should drain while sprinting"), Character->GetStamina() < 10.0f);

	Character->Stamina = 0.0f;
	TestTrue(TEXT("DecreaseStamina should remain invokable at zero stamina"),
		CharacterCombatTests::InvokeNoParamUFunction(Character, FName(TEXT("DecreaseStamina"))));
	TestFalse(TEXT("Exhausted stamina should stop sprinting"), Character->IsSprinting);
	TestEqual(TEXT("Stopping sprint should restore walk speed"), Character->GetCharacterMovement()->MaxWalkSpeed, 300.0f);

	const float StaminaAfterStop = Character->GetStamina();
	TestTrue(TEXT("IncreaseStamina should be exposed as a UFUNCTION"),
		CharacterCombatTests::InvokeNoParamUFunction(Character, FName(TEXT("IncreaseStamina"))));
	TestTrue(TEXT("Stamina should recover after sprinting stops"), Character->GetStamina() >= StaminaAfterStop);

	CharacterCombatTests::DestroyTestWorld(World);
	return true;
}
