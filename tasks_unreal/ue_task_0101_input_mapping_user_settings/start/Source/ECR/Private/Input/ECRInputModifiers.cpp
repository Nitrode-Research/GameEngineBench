// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ECRInputModifiers.h"
#include "Settings/ECRSettingsShared.h"
#include "System/ECRLocalPlayer.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "Input/ECRAimSensitivityData.h"

DEFINE_LOG_CATEGORY_STATIC(LogECRInputModifiers, Log, All);

//////////////////////////////////////////////////////////////////////
// ECRInputModifiersHelpers

namespace ECRInputModifiersHelpers
{
	/** Returns the owning ECRLocalPlayer of an Enhanced Player Input pointer */
	static UECRLocalPlayer* GetLocalPlayer(const UEnhancedPlayerInput* PlayerInput)
	{
		if (PlayerInput)
		{
			if (APlayerController* PC = Cast<APlayerController>(PlayerInput->GetOuter()))
			{
				return Cast<UECRLocalPlayer>(PC->GetLocalPlayer());
			}
		}
		return nullptr;
	}
	
}

//////////////////////////////////////////////////////////////////////
// UECRSettingBasedScalar

FInputActionValue UECRSettingBasedScalar::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


//////////////////////////////////////////////////////////////////////
// UECRInputModifierDeadZone

FInputActionValue UECRInputModifierDeadZone::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FLinearColor UECRInputModifierDeadZone::GetVisualizationColor_Implementation(FInputActionValue SampleValue, FInputActionValue FinalValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// UECRInputModifierGamepadSensitivity

FInputActionValue UECRInputModifierGamepadSensitivity::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// UECRInputModifierMouseSensitivity

FInputActionValue UECRInputModifierMouseSensitivity::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue CurrentValue, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


//////////////////////////////////////////////////////////////////////
// UECRInputModifierAimInversion

FInputActionValue UECRInputModifierAimInversion::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}
