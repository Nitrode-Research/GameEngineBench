#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GASShooterPlayerStateControllerTests
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

#define GS_REQUIRE_TOKENS(Test, Label, Source, Tokens) \
	GASShooterPlayerStateControllerTests::RequireTokens(Test, Label, Source, Tokens, UE_ARRAY_COUNT(Tokens))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerStateASCAttributePathsTest,
	"GASShooter.PlayerStateController.playerstate_asc_attribute_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlayerStateASCAttributePathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterPlayerStateControllerTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Player/GSPlayerState.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R1/R2/R6): PlayerState must own ASC/attributes and expose durable attribute/player values.
	const TCHAR* RequiredTokens[] = {
		TEXT("AbilitySystemComponent = CreateDefaultSubobject<UGSAbilitySystemComponent>(TEXT(\"AbilitySystemComponent\"))"),
		TEXT("AbilitySystemComponent->SetIsReplicated(true)"),
		TEXT("AttributeSetBase = CreateDefaultSubobject<UGSAttributeSetBase>(TEXT(\"AttributeSetBase\"))"),
		TEXT("AmmoAttributeSet = CreateDefaultSubobject<UGSAmmoAttributeSet>(TEXT(\"AmmoAttributeSet\"))"),
		TEXT("NetUpdateFrequency = 100.0f"),
		TEXT("return AttributeSetBase->GetCharacterLevel()"),
		TEXT("return AttributeSetBase->GetXP()")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSPlayerState ASC and attributes"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLocalHudAndControllerHelperPathsTest,
	"GASShooter.PlayerStateController.local_hud_and_controller_helper_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLocalHudAndControllerHelperPathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	if (!GASShooterPlayerStateControllerTests::LoadProjectFile(*this, TEXT("Source/GASShooter/Private/Player/GSPlayerController.cpp"), Source))
	{
		return false;
	}

	// REQUIRED (R3/R4/R8): HUD creation is local-only, idempotent, initialized from PlayerState/pawn, and helper calls update the widget.
	const TCHAR* RequiredTokens[] = {
		TEXT("if (UIHUDWidget)"),
		TEXT("if (!IsLocalPlayerController())"),
		TEXT("AGSPlayerState* PS = GetPlayerState<AGSPlayerState>()"),
		TEXT("CreateWidget<UGSHUDWidget>(this, UIHUDWidgetClass)"),
		TEXT("UIHUDWidget->AddToViewport()"),
		TEXT("UIHUDWidget->SetCurrentHealth(PS->GetHealth())"),
		TEXT("UIHUDWidget->SetEquippedWeaponSprite(CurrentWeapon->PrimaryIcon)"),
		TEXT("CreateHUD()")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GSPlayerController HUD paths"), Source, RequiredTokens);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerRespawnLifecyclePathsTest,
	"GASShooter.PlayerStateController.server_respawn_lifecycle_paths_present",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerRespawnLifecyclePathsTest::RunTest(const FString& Parameters)
{
	FString Source;
	const TCHAR* Paths[] = {
		TEXT("Source/GASShooter/GASShooterGameModeBase.cpp"),
		TEXT("Source/GASShooter/Private/Player/GSPlayerController.cpp")
	};
	if (!GASShooterPlayerStateControllerTests::LoadCombined(*this, Paths, UE_ARRAY_COUNT(Paths), Source))
	{
		return false;
	}

	// REQUIRED (R5/R7): death/respawn must route through server authority, spectator transition, new hero possession, and refreshed avatar info.
	const TCHAR* RequiredTokens[] = {
		TEXT("GetWorld()->SpawnActor<ASpectatorPawn>"),
		TEXT("RespawnDelegate = FTimerDelegate::CreateUObject(this, &AGASShooterGameModeBase::RespawnHero, Controller)"),
		TEXT("PC->SetRespawnCountdown(RespawnDelay)"),
		TEXT("GetWorld()->SpawnActor<AGSHeroCharacter>"),
		TEXT("OldSpectatorPawn->Destroy()"),
		TEXT("Controller->Possess(Hero)"),
		TEXT("PC->ClientSetControlRotation(PlayerStart->GetActorRotation())"),
		TEXT("PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn)"),
		TEXT("ServerKill()")
	};
	return GS_REQUIRE_TOKENS(*this, TEXT("GASShooter respawn lifecycle paths"), Source, RequiredTokens);
}

#undef GS_REQUIRE_TOKENS
