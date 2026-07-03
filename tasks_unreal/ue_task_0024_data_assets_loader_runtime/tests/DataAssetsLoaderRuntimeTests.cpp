// Copyright 2026 GameDevBench. DataAssetsLoader runtime automation tests (Bomber).

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "DalSubsystem.h"
#include "DataAssets/BmrSoundsDataAsset.h"
#include "DataAssets/BmrUIDataAsset.h"

namespace BomberDataAssetsLoaderRuntimeTests
{
	static UDalSubsystem* GetSubsystem(FAutomationTestBase& Test)
	{
		UDalSubsystem* Subsystem = UDalSubsystem::GetDataAssetsLoaderSubsystem();
		Test.TestNotNull(TEXT("UDalSubsystem engine subsystem must exist"), Subsystem);
		return Subsystem;
	}

	static UBmrSoundsDataAsset* NewSoundsAsset()
	{
		return NewObject<UBmrSoundsDataAsset>(GetTransientPackage());
	}

	static UBmrUIDataAsset* NewUIAsset()
	{
		return NewObject<UBmrUIDataAsset>(GetTransientPackage());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberDAL_RegisterLookupUnregister,
	"Bomber.DataAssetsLoaderRuntime.register_lookup_unregister",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberDAL_RegisterLookupUnregister::RunTest(const FString& Parameters)
{
	// REQUIRED: validates the direct data-asset cache contract.
	UDalSubsystem* Subsystem = BomberDataAssetsLoaderRuntimeTests::GetSubsystem(*this);
	if (!Subsystem)
	{
		return true;
	}

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	TestNull(TEXT("Sounds data asset should not be registered after explicit unregister"),
		UDalSubsystem::GetDataAsset(UBmrSoundsDataAsset::StaticClass()));

	UBmrSoundsDataAsset* SoundsAsset = BomberDataAssetsLoaderRuntimeTests::NewSoundsAsset();
	if (!TestNotNull(TEXT("Synthetic sounds data asset must be constructible"), SoundsAsset))
	{
		return true;
	}

	Subsystem->RegisterDataAsset(UBmrSoundsDataAsset::StaticClass(), SoundsAsset);
	TestTrue(TEXT("GetDataAsset must return the same registered sounds asset"),
		UDalSubsystem::GetDataAsset(UBmrSoundsDataAsset::StaticClass()) == SoundsAsset);

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	TestNull(TEXT("GetDataAsset must return null after unregister"),
		UDalSubsystem::GetDataAsset(UBmrSoundsDataAsset::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberDAL_ImmediateAndPendingAssetListeners,
	"Bomber.DataAssetsLoaderRuntime.immediate_and_pending_asset_listeners",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberDAL_ImmediateAndPendingAssetListeners::RunTest(const FString& Parameters)
{
	// REQUIRED: validates immediate listener delivery and pending listener delivery-on-register.
	UDalSubsystem* Subsystem = BomberDataAssetsLoaderRuntimeTests::GetSubsystem(*this);
	if (!Subsystem)
	{
		return true;
	}

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	UBmrSoundsDataAsset* SoundsAsset = BomberDataAssetsLoaderRuntimeTests::NewSoundsAsset();
	if (!TestNotNull(TEXT("Synthetic sounds data asset must be constructible"), SoundsAsset))
	{
		return true;
	}

	bool bPendingCallbackFired = false;
	const UBmrSoundsDataAsset* PendingCallbackAsset = nullptr;
	Subsystem->ListenForDataAsset<UBmrSoundsDataAsset>(
		Subsystem,
		[&bPendingCallbackFired, &PendingCallbackAsset](const UBmrSoundsDataAsset& Asset)
		{
			bPendingCallbackFired = true;
			PendingCallbackAsset = &Asset;
		});

	TestFalse(TEXT("Pending listener must not fire before the asset is registered"), bPendingCallbackFired);

	Subsystem->RegisterDataAsset(UBmrSoundsDataAsset::StaticClass(), SoundsAsset);
	TestTrue(TEXT("Pending listener must fire when the asset class is registered"), bPendingCallbackFired);
	TestTrue(TEXT("Pending listener must receive the registered asset"), PendingCallbackAsset == SoundsAsset);

	bool bImmediateCallbackFired = false;
	const UBmrSoundsDataAsset* ImmediateCallbackAsset = nullptr;
	Subsystem->ListenForDataAsset<UBmrSoundsDataAsset>(
		Subsystem,
		[&bImmediateCallbackFired, &ImmediateCallbackAsset](const UBmrSoundsDataAsset& Asset)
		{
			bImmediateCallbackFired = true;
			ImmediateCallbackAsset = &Asset;
		});

	TestTrue(TEXT("Listener must fire immediately when asset is already registered"), bImmediateCallbackFired);
	TestTrue(TEXT("Immediate listener must receive the registered asset"), ImmediateCallbackAsset == SoundsAsset);

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberDAL_MultiAssetListenerWaitsForAllClasses,
	"Bomber.DataAssetsLoaderRuntime.multi_asset_listener_waits_for_all_classes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberDAL_MultiAssetListenerWaitsForAllClasses::RunTest(const FString& Parameters)
{
	// REQUIRED: validates aggregate listener completion after all requested asset classes are available.
	UDalSubsystem* Subsystem = BomberDataAssetsLoaderRuntimeTests::GetSubsystem(*this);
	if (!Subsystem)
	{
		return true;
	}

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	Subsystem->UnregisterDataAsset(UBmrUIDataAsset::StaticClass());

	bool bAllAssetsReady = false;
	const TArray<TSubclassOf<UDalPrimaryDataAsset>> RequiredClasses{
		UBmrSoundsDataAsset::StaticClass(),
		UBmrUIDataAsset::StaticClass()
	};
	Subsystem->ListenForDataAssets(
		Subsystem,
		RequiredClasses,
		[&bAllAssetsReady]()
		{
			bAllAssetsReady = true;
		});

	UBmrSoundsDataAsset* SoundsAsset = BomberDataAssetsLoaderRuntimeTests::NewSoundsAsset();
	UBmrUIDataAsset* UIAsset = BomberDataAssetsLoaderRuntimeTests::NewUIAsset();
	if (!TestNotNull(TEXT("Synthetic sounds data asset must be constructible"), SoundsAsset)
		|| !TestNotNull(TEXT("Synthetic UI data asset must be constructible"), UIAsset))
	{
		return true;
	}

	Subsystem->RegisterDataAsset(UBmrSoundsDataAsset::StaticClass(), SoundsAsset);
	TestFalse(TEXT("Aggregate listener must wait for every requested asset class"), bAllAssetsReady);

	Subsystem->RegisterDataAsset(UBmrUIDataAsset::StaticClass(), UIAsset);
	TestTrue(TEXT("Aggregate listener must fire after all requested asset classes are registered"), bAllAssetsReady);

	Subsystem->UnregisterDataAsset(UBmrSoundsDataAsset::StaticClass());
	Subsystem->UnregisterDataAsset(UBmrUIDataAsset::StaticClass());
	return true;
}
