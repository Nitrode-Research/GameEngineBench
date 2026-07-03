#pragma once

#include "Blueprint/UserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "UINavigationInterface.generated.h"

UINTERFACE(Blueprintable)
class UUINavigationInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IUINavigationInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void OnActivatedNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void InitializeButtons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetupNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetupButtons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Appearance")
	void SetupAppearance();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void OnNavigationInputDown(const FKeyEvent& KeyEvent);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void FindButtonFromWidgetClass(TSubclassOf<class UCommonActivatableWidget> WidgetClass, bool& Found, UCommonButtonBase*& Button);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void DoButtonsHaveFocus(bool& ButtonsHaveFocus);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void GetDefaultButton(UCommonButtonBase*& Button);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SelectDefaultButton();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SelectPreviousButton();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void GetPreviousButton(UCommonButtonBase*& Button, int& Index);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SelectNextButton();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void GetNextButton(UCommonButtonBase*& Button, int& Index);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void GetSelectedButton(UCommonButtonBase*& Button, int& Index);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetStartingLocation(const TArray<TSubclassOf<class UCommonActivatableWidget>>& WidgetClass, bool Select);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void GetStartingLocation(TArray<TSubclassOf<class UCommonActivatableWidget>>& WidgetClass);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SelectStartingLocation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void ResetStartingLocation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetChildStartingLocation(bool& HasChildren, TArray<TSubclassOf<class UCommonActivatableWidget>>& StartingLocation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void EnableNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void DisableNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void DisplayNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void HideNavigation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void DisplayMask(bool Solid);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void RemoveMask();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void RotateMask(float Rotation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void DisplayScanlines();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void RemoveScanlines();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void DisplayBlur(float Strength);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation|Main Menu")
	void RemoveBlur();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetBackgroundImage(const UTexture* Image);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void RemoveBackgroundImage();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void SetBackgroundForegroundImage(const UTexture* Image);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation")
	void RemoveBackgroundForegroundImage();
};