#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#define protected public
#define private public
#include "UIExtensionSystem.h"
#undef private
#undef protected

namespace UIExtensionTest
{
	static FGameplayTag GetLayerTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Layer.Game")), false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIExtensionContractMatchingTest,
	"UIExtension.System.contract_matching_accepts_valid_widget_data_and_context",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUIExtensionContractMatchingTest::RunTest(const FString&)
{
	// The contract is exercised through the public registration path: a point that only
	// allows UUserWidget data must be notified for a compatible extension and must NOT be
	// notified for an incompatible one ("reject incompatible registrations instead of
	// silently accepting everything"). (FUIExtensionPoint::DoesExtensionPassContract is
	// not module-exported, so the contract is observed via the subsystem, not called
	// directly.)
	const FGameplayTag Tag = UIExtensionTest::GetLayerTag();
	if (!TestTrue(TEXT("UI.Layer.Game gameplay tag must be available"), Tag.IsValid()))
	{
		return true;
	}

	UUIExtensionSubsystem* Subsystem = NewObject<UUIExtensionSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("UI extension subsystem should be constructible"), Subsystem))
	{
		return true;
	}

	int32 CompatibleFires = 0;
	int32 IncompatibleFires = 0;
	TArray<UClass*> AllowedDataClasses{UUserWidget::StaticClass()};
	Subsystem->RegisterExtensionPointForContext(
		Tag, nullptr, EUIExtensionPointMatch::ExactMatch, AllowedDataClasses,
		FExtendExtensionPointDelegate::CreateLambda(
			[&](EUIExtensionAction Action, const FUIExtensionRequest& Request)
			{
				if (Action == EUIExtensionAction::Added)
				{
					if (Request.Data == static_cast<UObject*>(UUserWidget::StaticClass())) { ++CompatibleFires; }
					else { ++IncompatibleFires; }
				}
			}));

	// Incompatible data (a plain UObject class is not a UUserWidget) must be rejected.
	Subsystem->RegisterExtensionAsData(Tag, nullptr, UObject::StaticClass(), 1);
	// Compatible data (a UUserWidget class) must be accepted and notified.
	Subsystem->RegisterExtensionAsData(Tag, nullptr, UUserWidget::StaticClass(), 1);

	TestEqual(TEXT("A compatible widget-class extension must pass the contract and notify the point"),
		CompatibleFires, 1);
	TestEqual(TEXT("An incompatible extension must be rejected by the contract (not notified)"),
		IncompatibleFires, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIExtensionRegistrationTest,
	"UIExtension.System.registering_point_and_data_produces_valid_handles_and_callback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUIExtensionRegistrationTest::RunTest(const FString&)
{
	const FGameplayTag Tag = UIExtensionTest::GetLayerTag();
	TestTrue(TEXT("UI.Layer.Game gameplay tag should be available for the test"), Tag.IsValid());

	UUIExtensionSubsystem* Subsystem = NewObject<UUIExtensionSubsystem>(GetTransientPackage());
	TestNotNull(TEXT("UI extension subsystem should be constructible"), Subsystem);
	if (!Subsystem || !Tag.IsValid())
	{
		return true;
	}

	bool bCallbackTriggered = false;
	FUIExtensionRequest ObservedRequest;

	TArray<UClass*> AllowedDataClasses{UUserWidget::StaticClass()};
	FUIExtensionPointHandle PointHandle = Subsystem->RegisterExtensionPointForContext(
		Tag,
		nullptr,
		EUIExtensionPointMatch::ExactMatch,
		AllowedDataClasses,
		FExtendExtensionPointDelegate::CreateLambda(
			[&](EUIExtensionAction Action, const FUIExtensionRequest& Request)
			{
				if (Action == EUIExtensionAction::Added)
				{
					bCallbackTriggered = true;
					ObservedRequest = Request;
				}
			}));

	FUIExtensionHandle ExtensionHandle = Subsystem->RegisterExtensionAsData(Tag, nullptr, UUserWidget::StaticClass(), 7);

	TestTrue(TEXT("RegisterExtensionPointForContext should return a valid handle"), PointHandle.IsValid());
	TestTrue(TEXT("RegisterExtensionAsData should return a valid handle"), ExtensionHandle.IsValid());
	TestTrue(TEXT("Registering matching data should notify the extension point"), bCallbackTriggered);
	TestEqual(TEXT("The extension request should carry through the requested priority"), ObservedRequest.Priority, 7);
	TestTrue(TEXT("The extension request should carry through the data object"),
		ObservedRequest.Data == static_cast<UObject*>(UUserWidget::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIExtensionCleanupTest,
	"UIExtension.System.unregistering_handles_cleans_extension_maps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUIExtensionCleanupTest::RunTest(const FString&)
{
	const FGameplayTag Tag = UIExtensionTest::GetLayerTag();
	TestTrue(TEXT("UI.Layer.Game gameplay tag should be available for the test"), Tag.IsValid());

	UUIExtensionSubsystem* Subsystem = NewObject<UUIExtensionSubsystem>(GetTransientPackage());
	TestNotNull(TEXT("UI extension subsystem should be constructible"), Subsystem);
	if (!Subsystem || !Tag.IsValid())
	{
		return true;
	}

	TArray<UClass*> AllowedDataClasses{UUserWidget::StaticClass()};
	// A bound delegate is required: the solution rejects a point with an unbound callback.
	FUIExtensionPointHandle PointHandle = Subsystem->RegisterExtensionPointForContext(
		Tag, nullptr, EUIExtensionPointMatch::ExactMatch, AllowedDataClasses,
		FExtendExtensionPointDelegate::CreateLambda([](EUIExtensionAction, const FUIExtensionRequest&) {}));
	FUIExtensionHandle ExtensionHandle = Subsystem->RegisterExtensionAsData(Tag, nullptr, UUserWidget::StaticClass(), 1);

	// Registration must actually populate the subsystem's tracking maps. The start stubs
	// return empty handles and never touch the maps, so this fails on start/.
	TestTrue(TEXT("Registering an extension point should return a valid handle"), PointHandle.IsValid());
	TestTrue(TEXT("Registering an extension should return a valid handle"), ExtensionHandle.IsValid());
	TestEqual(TEXT("The registered extension point should be tracked in the subsystem"),
		Subsystem->ExtensionPointMap.Num(), 1);
	TestEqual(TEXT("The registered extension should be tracked in the subsystem"),
		Subsystem->ExtensionMap.Num(), 1);

	Subsystem->UnregisterExtension(ExtensionHandle);
	Subsystem->UnregisterExtensionPoint(PointHandle);

	TestEqual(TEXT("All extension points should be removed after unregister"), Subsystem->ExtensionPointMap.Num(), 0);
	TestEqual(TEXT("All extensions should be removed after unregister"), Subsystem->ExtensionMap.Num(), 0);

	return true;
}
