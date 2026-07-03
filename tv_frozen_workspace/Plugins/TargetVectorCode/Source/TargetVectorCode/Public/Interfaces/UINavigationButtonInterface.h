#pragma once

#include "Blueprint/UserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "UINavigationButtonInterface.generated.h"

UINTERFACE(Blueprintable)
class UUINavigationButtonInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IUINavigationButtonInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void SetStack(UCommonActivatableWidgetStack* Stack);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void SetButtonsArray(const TArray<UCommonButtonBase*>& OtherButtons);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void SetWidget(TSubclassOf<class UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void GetButtonNavigationInfo(UCommonActivatableWidgetStack*& Stack, TSubclassOf<class UCommonActivatableWidget>& WidgetClass);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void OnSelected();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Navigation Button")
	void UnselectOtherButtons();
};