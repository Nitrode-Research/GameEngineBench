// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AlsxtCharacterPlayer.h"
#include "ModularPlayerController.h"
#include "AbilitySystemInterface.h"
#include "AlsxtPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;
class UAlsxtAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetupInputComponentDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnyInputDetected);

/**
* @file AlsxtPlayerController.h
* @brief template class that contains all shared Logic and Data for Player Classes.
* AAlsxtCharacterPlayer depends on AAlsxtPlayerState and AAlsxtPlayerController to function.
* Create a Blueprint class based on this class, do not use the C++ class directly in the Editor
*/
UCLASS()
class ALSXT_API AAlsxtPlayerController : public AModularPlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSetupInputComponentDelegate OnSetupPlayerInputComponentUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAnyInputDetected OnAnyInputDetected;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "References")
	TSoftObjectPtr<AAlsxtCharacterPlayer> AlsxtCharacterPlayer;
	
	UPROPERTY(VisibleAnywhere, Category = "References")
	TSoftObjectPtr<AAlsxtPlayerState> AlsxtPlayerState;
	

	UPROPERTY(EditAnywhere, Category = "References")
	TSoftObjectPtr<UAlsxtAbilitySystemComponent> AlsxtAbilitySystemComponent;

	void Input_OnHandleAnyInputKey();

	void Input_OnMainMenuAction(const FInputActionValue& ActionValue);

	void Input_OnInventoryAction(const FInputActionValue& ActionValue);

	void Input_OnHandSignalAction(const FInputActionValue& ActionValue);
	
	UPROPERTY(VisibleAnywhere, Category = "ALSXT Player Controller")
	bool CanControlCharacter {true};

	UPROPERTY(VisibleAnywhere, Category = "ALSXT Player Controller")
	bool CanControlPawn {false};
	
protected:
	// Server only
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnRep_PlayerState() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AnyInputDetectionAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MainMenuAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> HandSignalAction;

public:
	virtual void SetupInputComponent() override;

protected:
	virtual void BeginPlay() override;
	
};
