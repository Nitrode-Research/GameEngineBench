// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ECRInputComponent.h"
#include "System/ECRLocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Settings/ECRSettingsLocal.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"

UECRInputComponent::UECRInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UECRInputComponent::AddInputMappings(const UECRInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	UECRLocalPlayer* LocalPlayer = InputSubsystem->GetLocalPlayer<UECRLocalPlayer>();
	check(LocalPlayer);

	// Add any registered input mappings from the settings!
	if (UECRSettingsLocal* LocalSettings = UECRSettingsLocal::Get())
	{	
		// Tell enhanced input about any custom keymappings that the player may have customized
		for (const TPair<FName, FKey>& Pair : LocalSettings->GetCustomPlayerInputConfig())
		{
			if (Pair.Key != NAME_None && Pair.Value.IsValid())
			{
				if (UEnhancedInputUserSettings* UserSettings = InputSubsystem->GetUserSettings())
				{
					FMapPlayerKeyArgs Args;
					Args.MappingName = Pair.Key;
					Args.NewKey = Pair.Value;
					Args.Slot = EPlayerMappableKeySlot::First;

					FGameplayTagContainer FailureReason;
					UserSettings->MapPlayerKey(Args, FailureReason);
				}
			}
		}
	}
}

void UECRInputComponent::RemoveInputMappings(const UECRInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	UECRLocalPlayer* LocalPlayer = InputSubsystem->GetLocalPlayer<UECRLocalPlayer>();
	check(LocalPlayer);
	
	if (UECRSettingsLocal* LocalSettings = UECRSettingsLocal::Get())
	{
		// Remove any registered input contexts
		const TArray<FLoadedMappableConfigPair>& Configs = LocalSettings->GetAllRegisteredInputConfigs();
		for (const FLoadedMappableConfigPair& Pair : Configs)
		{
			UEnhancedInputUserSettings* UserSettings = InputSubsystem->GetUserSettings();
			for (const TPair<TObjectPtr<UInputMappingContext>, int32>& MappingPair : Pair.Config->GetMappingContexts())
			{
				if (UInputMappingContext* MappingContext = MappingPair.Key.Get())
				{
					InputSubsystem->RemoveMappingContext(MappingContext);
					if (UserSettings)
					{
						UserSettings->UnregisterInputMappingContext(MappingContext);
					}
				}
			}
		}
		
		// Clear any player mapped keys from enhanced input
		for (const TPair<FName, FKey>& Pair : LocalSettings->GetCustomPlayerInputConfig())
		{
			if (UEnhancedInputUserSettings* UserSettings = InputSubsystem->GetUserSettings())
			{
				FMapPlayerKeyArgs Args;
				Args.MappingName = Pair.Key;
				Args.Slot = EPlayerMappableKeySlot::First;

				FGameplayTagContainer FailureReason;
				UserSettings->UnMapPlayerKey(Args, FailureReason);
			}
		}
	}
}

void UECRInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
