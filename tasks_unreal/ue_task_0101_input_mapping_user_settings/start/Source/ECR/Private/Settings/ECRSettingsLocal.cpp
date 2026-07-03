// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/ECRSettingsLocal.h"
#include "CommonInputBaseTypes.h"
#include "EnhancedActionKeyMapping.h"
#include "PlayerMappableInputConfig.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "System/ECRLocalPlayer.h"

UECRSettingsLocal::UECRSettingsLocal()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UECRSettingsLocal* UECRSettingsLocal::Get()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}



void UECRSettingsLocal::SetToDefaults()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRSettingsLocal::RegisterInputConfig(ECommonInputType Type, const UPlayerMappableInputConfig* NewConfig,
                                            const bool bIsActive)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UECRSettingsLocal::UnregisterInputConfig(const UPlayerMappableInputConfig* ConfigToRemove)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRSettingsLocal::ActivateInputConfig(const UPlayerMappableInputConfig* Config)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsLocal::DeactivateInputConfig(const UPlayerMappableInputConfig* Config)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


const UPlayerMappableInputConfig* UECRSettingsLocal::GetInputConfigByName(FName ConfigName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRSettingsLocal::GetRegisteredInputConfigsOfType(ECommonInputType Type,
                                                        TArray<FLoadedMappableConfigPair>& OutArray) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsLocal::AddOrUpdateCustomKeyboardBindings(const FName MappingName, const FKey NewKey,
                                                          UECRLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

