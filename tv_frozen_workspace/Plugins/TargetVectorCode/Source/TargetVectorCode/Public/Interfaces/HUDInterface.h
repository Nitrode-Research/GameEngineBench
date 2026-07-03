#pragma once

#include "Internationalization/Text.h"
#include "CommonActivatableWidget.h" 
#include "CommonButtonBase.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonAnimatedSwitcher.h"
#include "GameFramework/Actor.h"
#include "Utility/TVStructs.h"
#include "NativeGameplayTags.h"
#include "Style/StyleSettings.h"
#include "HUDInterface.generated.h"

UINTERFACE(Blueprintable)
class UHUDInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IHUDInterface {
	GENERATED_BODY()

public:

	// MAIN LAYER STACKS
	
	// Player Effects Overlay Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetPlayerEffectsOverlayLayerStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void DisplayPlayerEffectsOverlayLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void RemovePlayerEffectsOverlayLayer();

	// Game Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetGameLayerStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void DisplayGameLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void RemoveGameLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void HideGameLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void UnhideGameLayer();

	// Game Menu Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetGameMenuLayerStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void DisplayGameMenuLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void RemoveGameMenuLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void HideGameMenuLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void UnhideGameMenuLayer();

	// Menu Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetMenuLayerStack(UCommonActivatableWidgetStack*& Stack);

	// Overlay Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetOverlayLayerStack(UCommonActivatableWidgetStack*& Stack);

	// Title Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetTitleLayerStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void DisplayTitleLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void RemoveTitleLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void HideTitleLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void UnhideTitleLayer();

	// Modal Queue Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetModalQueueLayerStack(UCommonActivatableWidgetQueue*& Queue);

	// HUD Effects Overlay Layer

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void GetHUDEffectsOverlayLayerStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void DisplayHUDEffectsOverlayLayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void RemoveHUDEffectsOverlayLayer();

	// PRIMARY LAYOUT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Layers")
	void InitializePrimaryLayout();

	// MUSIC

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Now Playing")
	void GetCurrentMusic(FMusicInfo& MusicInfo);

	// NAVIGATION

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void Back();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void AddBreadcrumbEntry(FBreadcrumbEntry BreadcrumbEntry);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void RemoveBreadcrumbEntry(FBreadcrumbEntry BreadcrumbEntry);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void GetBreadcrumb(TArray<FBreadcrumbEntry>& Breadcrumb);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Appearance")
	void SetupAppearance();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void InitializeButtons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void SetupNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void SetCurrentMenuStack(const UCommonActivatableWidgetStack* Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void GetCurrentMenuStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void GetFocusedWidget(UUserWidget*& FocusedWidget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void SetFocusedWidget(const UUserWidget* FocusedWidget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation")
	void SetStartingLocation(const TArray<TSubclassOf<class UCommonActivatableWidget>>& WidgetClass);

	// MAIN MENU

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void IsInMainMenuLevel(bool& IsInMainMenuLevel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void IsMainMenuActive(bool& IsActive);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta=(AutoCreateRefTerm = "StartingLocation"),Category = "Navigation|Main Menu")
	void DisplayMainMenu(const TArray<TSubclassOf<class UCommonActivatableWidget>>& StartingLocation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void EnableMainMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisableMainMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void IsMainMenuNavigationEnabled(bool& IsEnabled);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayMainMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void HideMainMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void IsMainMenuNavigationVisible(bool& IsEnabled);

	// PROMPTS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void DisplayQuitGamePrompt();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void RemoveQuitGamePrompt();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void IsQuitGamePromptActive(bool& IsActive);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void DisplayAbandonChangesPrompt();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void RemoveAbandonChangesPrompt();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void IsAbandonChangesPromptActive(bool& IsActive);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Quit Game")
	void ConfirmAbandonChangesPrompt();

	// SCANLINES LAYER

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayMainMenuScanlines();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenuScanlines();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayMainMenuBlur(float Strength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenuBlur();

	// MASK LAYER

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayMainMenuMask(bool Solid);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenuMask();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RotateMainMenuMask(float Rotation);

	// BACKGROUND LAYERS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuBackgroundImage(const UTexture* Image);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenuBackgroundImage();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuBackgroundForegroundImage(const UTexture* Image);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveMainMenuBackgroundForegroundImage();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveAllMainMenuBackgroundLayers();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Debug")
	void GetOverlayMenu(UUserWidget*& OverlayMenu);

	// GAME MENU

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void IsGameMenuActive(bool& IsActive);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta=(AutoCreateRefTerm = "StartingLocation"),Category = "Navigation|Game Menu")
	void DisplayGameMenu(const TArray<TSubclassOf<class UCommonActivatableWidget>>& StartingLocation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void RemoveGameMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void EnableGameMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void DisableGameMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void IsGameMenuNavigationEnabled(bool& IsEnabled);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void DisplayGameMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void HideGameMenuNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void IsGameMenuNavigationVisible(bool& IsEnabled);

	// SPECIFIC NAVIGTION

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuProfile(const UCommonActivatableWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void GetMainMenuProfile(UCommonActivatableWidget*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuSettings(const UCommonActivatableWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void GetMainMenuSettings(UCommonActivatableWidget*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuSettingsGameplay(const UCommonActivatableWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void GetMainMenuSettingsGameplay(UCommonActivatableWidget*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SetMainMenuSettingsVideo(const UCommonActivatableWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void GetMainMenuSettingsVideo(UCommonActivatableWidget*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectMainMenuDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectMainMenuPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectMainMenuNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectPlayDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectPlayPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectPlayNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void SelectOperationsDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Game Menu")
	void SelectOperationsPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectOperationsNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Profile")
	void SelectProfileDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Profile")
	void SelectProfilePreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Profile")
	void SelectProfileNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectServerDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectServerPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectServerNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectGameMasterDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectGameMasterPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectGameMasterNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectUGCDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectUGCPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectUGCNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectShopDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectShopPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectShopNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectLockerDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectLockerPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void SelectLockerNextTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Settings")
	void SelectSettingsDefaultTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Settings")
	void SelectSettingsPreviousTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu|Settings")
	void SelectSettingsNextTab();

	// GAME LAYER

	// VIDEO CALL

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Video Call")
	void DisplayVideoCall(FNonPlayerCharacter Character, FSound Sound);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Video Call")
	void RemoveVideoCall();

	// ALERT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Alert")
	void DisplayAlert(TSubclassOf<class UCommonActivatableWidget> Widget, const FText& Instigator, const FText& Description);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Alert")
	void HideAlert();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Alert")
	void UnhideAlert();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Alert")
	void RemoveAlert();

	// NOTIFICATION

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Notification")
	void DisplayNotification(FNotification Notification);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Notification")
	void HideNotification();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Notification")
	void UnhideNotification();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Notification")
	void RemoveNotification(FNotification Notification);

	// SERVER EVENT TIMER LIST

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Timer List")
	void DisplayServerEventTimer(TSubclassOf<class UCommonActivatableWidget> Widget, const FText& Instigator, const FText& Description);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Timer List")
	void HideServerEventTimerList();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Timer List")
	void UnhideServerEventTimerList();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Timer List")
	void RemoveServerEventTimerList();

	// SERVER EVENT UPDATE

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Update")
	void DisplayServerEventUpdate(TSubclassOf<class UCommonActivatableWidget> Widget, const FText& TopSubtitle, const FText& MainTitle, const FText& BottomSubtitle);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Update")
	void HideServerEventUpdate();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Update")
	void UnhideServerEventUpdate();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Update")
	void RemoveServerEventUpdate();

	// SERVER EVENT NOTIFICATION

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Notification")
	void DisplayServerEventNotification(FMission Mission, const TArray<FObjective>& Objectives);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Notification")
	void HideServerEventNotification();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Notification")
	void UnhideServerEventNotification();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Notification")
	void RemoveServerEventNotification();

	// HINT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Hint")
	void DisplayHint(const FText& Title, const FText& PrefixText, FDataTableRowHandle InputAction, const FText& SuffixSubtitle);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Hint")
	void HideHint();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Hint")
	void UnhideHint();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Event|Hint")
	void RemoveHint();

	// VEHICLE GAUGE CLUSTER

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Gauge Cluster")
	void DisplayVehicleGaugeCluster(TSubclassOf<class UCommonActivatableWidget> Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Gauge Cluster")
	void HideVehicleGaugeCluster();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Gauge Cluster")
	void UnhideVehicleGaugeCluster();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Gauge Cluster")
	void RemoveVehicleGaugeCluster();

	// VEHICLE CONTEXTUAL

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Contextual")
	void DisplayVehicleContextual(TSubclassOf<class UCommonActivatableWidget> Widget, const TArray<FContextualInputAction>& ContextualInputPrompts);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Contextual")
	void HideVehicleContextual();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Contextual")
	void UnhideVehicleContextual();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vehicle|Contextual")
	void RemoveVehicleContextual();

	// GAME MODE 1

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void DisplayGameMode1(TSubclassOf<class UCommonActivatableWidget> Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void HideGameMode1();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void UnhideGameMode1();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void RemoveGameMode1();

	// GAME MODE 2

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void DisplayGameMode2(TSubclassOf<class UCommonActivatableWidget> Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void HideGameMode2();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void UnhideGameMode2();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Game Mode")
	void RemoveGameMode2();

	// PRE-GAME

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game")
	void ClearPreGameStack();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Pre-Game|Deploy Menu")
	void DisplayPreGameDeployMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Pre-Game|Deploy Menu")
	void RemovePreGameDeployMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Deploy Menu")
	void DisplayDeployMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Deploy Menu")
	void RemoveDeployMenu();

	// SUMMARY

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Summary")
	void DisplayPreGameSummary();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Summary")
	void RemovePreGameSummary();

	// BRIEFING

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Briefing")
	void DisplayPreGameBriefing();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Briefing")
	void RemovePreGameBriefing();

	// ATMOSPHERIC

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Atmospheric")
	void DisplayPreGameAtmospheric();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Atmospheric")
	void RemovePreGameAtmospheric();

	// CINEMATIC

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Cinematic")
	void DisplayPreGameCinematic();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Pre-Game|Cinematic")
	void RemovePreGameCinematic();

	// MOTD

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|MOTD")
	void DisplayMOTD();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|MOTD")
	void RemoveMOTD();

	// COUNTDOWN

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Countdown")
	void DisplayStandby();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Layer|Countdown")
	void RemoveStandby();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Countdown")
	void DisplayCountdown(FGameplayTag Style, float PreCountDelayLength, float CountdownLength, float PostCountDelayLength, const FText &StandyLabel, const FText &InProgressLabel, const FText &CompletedLabel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Countdown")
	void StartCountdown();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Countdown")
	void RemoveCountdown();

	// CONTEXTUAL INPUT PROMPT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Contextual Input Prompt")
	void AddContextualInputPrompts(const TArray<FContextualInputAction>& ContextualInputPrompts);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Contextual Input Prompt")
	void RemoveContextualInputPrompts(const TArray<FContextualInputAction>& ContextualInputPrompts);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Contextual Input Prompt")
	void RemoveAllContextualInputPrompts();

	// WEAPON INPUT PROMPT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Weapon Input Prompt")
	void AddWeaponInputPrompts(const TArray<FContextualInputAction>& WeaponInputPrompts);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Weapon Input Prompt")
	void RemoveWeaponInputPrompts(const TArray<FContextualInputAction>& WeaponInputPrompts);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Weapon Input Prompt")
	void RemoveAllWeaponInputPrompts();

	// CROSSHAIR

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void SetCrosshair(TSubclassOf<class UCommonActivatableWidget> Widget);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void GetCrosshair(UCommonActivatableWidget*& Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void DisplayCrosshair();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void HideCrosshair();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void UnhideCrosshair();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void SetCrosshairMode(EWidgetCenter CrosshairMode);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void SetCrosshairLocation(FVector Location);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Crosshair")
	void RemoveCrosshair();

	// HIT CONFIRMATION

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Hit Confirmation")
	void DisplayHitConfirmation(FGameplayTag Type, FGameplayTag Mode, FVector Location);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Hit Confirmation")
	void HideHitConfirmation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Hit Confirmation")
	void UnhideHitConfirmation();

	// COMPASS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Compass")
	void DisplayCompass();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Compass")
	void HideCompass();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Compass")
	void UnhideCompass();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Compass")
	void RemoveCompass();

	// ENVIRONMENT INFO

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Enivornment Info")
	void DisplayEnivornmentInfo();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Enivornment Info")
	void HideEnivornmentInfo();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Enivornment Info")
	void UnhideEnivornmentInfo();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Enivornment Info")
	void RemoveEnivornmentInfo();

	// SERVER LOG

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void DisplayServerLog();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void HideServerLog();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void UnhideServerLog();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void UpdateClientsServerLog();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void FadeOutServerLog(float InitialDelay, float FadeOutDuration);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Server Log")
	void RemoveServerLog();

	// NEXT MAP

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Next Map")
	void DisplayNextMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Next Map")
	void HideNextMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Next Map")
	void UnhideNextMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Next Map")
	void RemoveNextMap();

	// MINIMAP

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|MiniMap")
	void DisplayMiniMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|MiniMap")
	void HideMiniMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|MiniMap")
	void UnhideMiniMap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|MiniMap")
	void RemoveMiniMap();

	// COMMS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Comms")
	void DisplayComms();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Comms")
	void HideComms();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Comms")
	void UnhideComms();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Comms")
	void RemoveComms();

	// VITALS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vitals")
	void DisplayVitals();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vitals")
	void HideVitals();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vitals")
	void UnhideVitals();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Vitals")
	void RemoveVitals();

	// EQUIPPED ITEM

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Equipped Item")
	void DisplayEquippedItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Equipped Item")
	void HideEquippedItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Equipped Item")
	void UnhideEquippedItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Equipped Item")
	void FadeOutEquippedItem(float InitialDelay, float FadeOutDuration);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Equipped Item")
	void RemoveEquippedItem();

	// Now Playing
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Now Playing")
	void GetNowPlayingStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Now Playing")
	void DisplayNowPlaying(float DisplayLengthOverride);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Now Playing")
	void HideNowPlaying();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Now Playing")
	void UnhideNowPlaying();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Now Playing")
	void RemoveNowPlaying();

	// ACTION AWARD

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Action Award")
	void DisplayActionAward(FNotification Notification);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Action Award")
	void RemoveActionAward();

	// PROGRESSION BAR

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Progression Bar")
	void DisplayProgressionBar(FGameplayTag Type, int StartXP, int GoalXP, int StartPoints, int GoalPoints);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Progression Bar")
	void RemoveProgressionBar();

	// ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions")
	void DisplayActions();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions")
	void HideActions();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions")
	void UnhideActions();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions")
	void FadeOutActions(float InitialDelay, float FadeOutDuration);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions")
	void RemoveActions();

	// PLAYER ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions|Player")
	void AddPlayerActionClient(FPlayerAction PlayerAction);

	// FIRETEAM ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions|Fireteam")
	void AddFireteamActionClient(FFireteamAction FireteamAction);

	// SECTION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions|Section")
	void AddSectionActionClient(FSectionAction SectionAction);

	// ORGANIZATION ACTIONS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Actions|Organization")
	void AddOrganizationActionClient(FOrganizationAction OrganizationAction);

	// GAME MENU LAYER

	// TUTORIAL

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Tutorial")
	void DisplayTutorial(const FText& Title, const FText& PrefixText, FDataTableRowHandle InputAction, const FText& SuffixSubtitle);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Tutorial")
	void HideTutorial();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Tutorial")
	void UnhideTutorial();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Tutorial")
	void RemoveTutorial();

	// READINESS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Readiness")
	void DisplayReadiness();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Readiness")
	void HideReadiness();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Readiness")
	void UnhideReadiness();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Layer|Readiness")
	void RemoveReadiness();

	// RADIAL MENU

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Radial Menu")
	void DisplayRadialMenu(FGameplayTag RadialMenuType);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Radial Menu")
	void RemoveRadialMenu();

	// BIG ITEM

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Big Item")
	void DisplayBigItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Big Item")
	void HideBigItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Big Item")
	void UnhideBigItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Big Item")
	void RemoveBigItem();

	// CHAT

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void DisplayChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void FocusChatInput();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void SetChatHeightOverride(float HeightOverride);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void ClearChatHeightOverride();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void SetChatChannel(const FGameplayTag& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	TArray<FString> GetOpenChatWhisperChannels();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void SetChatWhisperChannel(const FString& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void CloseChatWhisperChannel(const FString& Channel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Game Menu Layer|Chat")
	void OnAddChatMessage(FChatMessage ChatMessage);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void UpdateClientsChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void HideChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void UnhideChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void FadeOutChat(float InitialDelay, float FadeOutDuration);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void ClearCurrentChatChannel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void ClearAllChatChannels();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Chat")
	void RemoveChat();

	// QUICK SLOTS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void DisplayQuickSlots(bool Persistent, bool DisplayAllSlots);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void HideQuickSlots();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void UnhideQuickSlots();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void GetQuickSlots(UCommonActivatableWidget*& QuickSlots);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void FadeOutQuickSlots(float InitialDelay, float FadeOutDuration);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Quick Slots")
	void RemoveQuickSlots();

	// INSPECT ITEM

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Inspect Item")
	void DisplayInspectItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Inspect Item")
	void RemoveInspectItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Inspect Item")
	void HideInspectItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Inspect Item")
	void UnhideInspectItem();

	// PLAYER DEATH

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Player Death")
	void StartDeathSequence();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Player Death")
	void DisplayPlayerDeathHUDEffect();

	// SPECTATING

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Spectating")
	void DisplayKillCamMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Spectating")
	void RemoveKillCamMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Spectating")
	void DisplaySpectatorMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Spectating")
	void RemoveSpectatorMenu();

	// DEBRIEFING

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Debriefing")
	void StartDebriefingSequence();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Debriefing")
	void DisplayDebriefingMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Debriefing")
	void RemoveDebriefingMenu();

	// OVERLAY LAYER

	// BLACK SCREEN

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Overlay Layer|Black Screen")
	void DisplayBlackScreen(float InitialDelay, float FadeLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Overlay Layer|Black Screen")
	void RemoveBlackScreen(float InitialDelay, float FadeLength);

	// WIPE LAYER

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Wipe Stack")
	void GetWipeStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD||Wipe Stack")
	void DisplayWipeScreen(int Selection);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "HUD|Wipe Stack")
	void ResetWipeStack();

	// TITLE LAYER

	// SUBTITLES

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Subtitles")
	void GetSubtitleStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Subtitles")
	void AddSubtitle(FSubtitleEntry SubtitleEntry);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Subtitles")
	void RemoveAllSubtitles();

	// OPENING CREDITS 

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Opening Credits")
	void GetOpeningCreditsStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Opening Credits")
	void DisplayOpeningCredit(float DisplayLength, const FText& Title, const FText& Name);

	// ENDING CREDITS

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Ending Credits")
	void GetEndCreditsStack(UCommonActivatableWidgetStack*& Stack);

	// DATE TIME LOCATION 

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Time Date Location Title")
	void GetTimeDateLocationTitleStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Time Date Location Title")
	void DisplayTimeDateLocationTitle(float DisplayLength);

	// Cutscene Now Playing

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Now Playing")
	void GetCutsceneNowPlayingStack(UCommonActivatableWidgetStack*& Stack);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Now Playing")
	void DisplayCutsceneNowPlaying(FMusicInfo MusicInfo, float DisplayLength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Now Playing")
	void HideCutsceneNowPlaying();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Now Playing")
	void UnhideCutsceneNowPlaying();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Title Layer|Now Playing")
	void RemoveCutsceneNowPlaying();

	// SHORTCUTS

	// MAIN MENU SHORTCUTS

	// Profile

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToProfile();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToProfileDashboard();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToMessages();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToNotifications();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToLoadouts();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToCharacter();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToCharacterCustomization();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToProfileStats();

	// Server - Status and Moderation

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToServer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToServerInformation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToServerStats();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToServerModeration();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToServerOverwatch();

	// Game Master

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMaster();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterDashboard();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterWorld();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterResources();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterMisssions();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterIntel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToGameMasterIntervention();

	// Shop

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToShop();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToShopClothing();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToShopWeapons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToShopEquipment();

	// Locker

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToLocker();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToLockerClothing();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToLockerWeapons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Main Menu")
	void GoToLockerEquipment();

	// GAME MENU SHORTCUTS

	// Player

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToPlayer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToPlayerDashboard();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToPlayerProgress();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToPlayerLoadout();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToRequests();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToPlayerGameStats();

	// Operations

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcut|Game Menu")
	void GoToOperations();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToOperationsDashboard();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToRoster();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToMissions();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToIntel();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToScoreboard();

	// Map

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToMap();

	// Inventory

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToInventory();

	// Apps

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToApps();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Shortcuts|Game Menu")
	void GoToApp(FGameplayTag App);

	// App Intro Animations

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Apps")
	void AddPlayedAppIntroAnimation(FGameplayTag App);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Apps")
	void ShouldAppIntroAnimationPlay(FGameplayTag App, bool& ShouldPlay);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Game Menu Layer|Apps")
	void ResetAppIntroAnimations();

	// RENDER TARGET

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Render Target")
	void SetInteractiveRenderTarget(const AActor* Actor);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Render Target")
	void GetInteractiveRenderTarget(AActor*& Actor);

	// REVIEW FOR REMOVAL

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Pre Game Game Master Menu")
	void DisplayPreGameGameMasterMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Pre Game Game Master Menu")
	void RemovePreGameGameMasterMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Player Ready Overlay")
	void DisplayPlayerReadyOverlay();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|Player Ready Overlay")
	void RemovePlayerReadyOverlay();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|In Game Game Master Menu")
	void DisplayInGameGameMasterMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD|In Game Game Master Menu")
	void RemoveInGameGameMasterMenu();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "HUD")
	void GetServerTabStack(UCommonActivatableWidgetStack*& Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayServerTabLobby();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveServerTabLobby();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void DisplayServerTabInGame();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveServerTabInGame();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void InitializeServerTab();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,Category = "Navigation|Main Menu")
	void RemoveServerTab();

};