// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/ECRSettingsShared.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "System/ECRLocalPlayer.h"
#include "Internationalization/Culture.h"

static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");

namespace ECRSettingsSharedCVars
{
	static float DefaultGamepadLeftStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadLeftStickInnerDeadZone(
		TEXT("gpad.DefaultLeftStickInnerDeadZone"),
		DefaultGamepadLeftStickInnerDeadZone,
		TEXT("Gamepad left stick inner deadzone")
	);

	static float DefaultGamepadRightStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadRightStickInnerDeadZone(
		TEXT("gpad.DefaultRightStickInnerDeadZone"),
		DefaultGamepadRightStickInnerDeadZone,
		TEXT("Gamepad right stick inner deadzone")
	);	
}

UECRSettingsShared::UECRSettingsShared()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRSettingsShared::Initialize(UECRLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsShared::SaveSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


/*static*/ UECRSettingsShared* UECRSettingsShared::LoadOrCreateSettings(const UECRLocalPlayer* LocalPlayer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UECRSettingsShared::ApplySettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


//////////////////////////////////////////////////////////////////////

void UECRSettingsShared::SetAllowAudioInBackgroundSetting(EECRAllowBackgroundAudioSetting NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsShared::ApplyBackgroundAudioSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


//////////////////////////////////////////////////////////////////////

void UECRSettingsShared::ApplyCultureSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsShared::ResetCultureToCurrentSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


const FString& UECRSettingsShared::GetPendingCulture() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	static const FString StubValue{};
	return StubValue;
}


void UECRSettingsShared::SetPendingCulture(const FString& NewCulture)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsShared::OnCultureChanged()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRSettingsShared::ClearPendingCulture()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRSettingsShared::IsUsingDefaultCulture() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRSettingsShared::ResetToDefaultCulture()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


//////////////////////////////////////////////////////////////////////

void UECRSettingsShared::ApplyInputSensitivity()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

