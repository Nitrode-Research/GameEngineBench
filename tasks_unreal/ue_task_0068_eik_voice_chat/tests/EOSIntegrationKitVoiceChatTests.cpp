#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "VoiceChat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIKVoiceChatTransmitModeTest,
	"EOSIntegrationKit.VoiceChat.transmit_to_all_channels_updates_transmit_mode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEIKVoiceChatTransmitModeTest::RunTest(const FString&)
{
	FModuleManager::Get().LoadModule(TEXT("EIKVoiceChat"));

	IVoiceChat* VoiceChat = IVoiceChat::Get();
	if (!TestNotNull(TEXT("EIK voice chat modular feature must be available"), VoiceChat))
	{
		return true;
	}

	TestEqual(
		TEXT("A freshly initialized voice user must start in this project's default transmit mode"),
		VoiceChat->GetTransmitMode(),
		EVoiceChatTransmitMode::None);

	VoiceChat->TransmitToNoChannels();
	TestEqual(
		TEXT("TransmitToNoChannels should reset the mode to None before the discriminator"),
		VoiceChat->GetTransmitMode(),
		EVoiceChatTransmitMode::None);

	VoiceChat->TransmitToAllChannels();
	TestEqual(
		TEXT("TransmitToAllChannels must update the voice user transmit mode to All"),
		VoiceChat->GetTransmitMode(),
		EVoiceChatTransmitMode::All);

	return true;
}
