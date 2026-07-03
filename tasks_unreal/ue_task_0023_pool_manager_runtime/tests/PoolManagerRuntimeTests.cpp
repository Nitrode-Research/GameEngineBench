// Copyright 2026 GameDevBench. PoolManager runtime automation tests (Bomber).

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"

#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

#include "Data/PoolObjectData.h"
#include "Data/PoolObjectHandle.h"
#include "Data/PoolObjectState.h"
#include "Data/SpawnRequestPriority.h"
#include "PoolManagerSubsystem.h"

namespace BomberPoolManagerRuntimeTests
{
	static const TCHAR* MapPath = TEXT("/Game/Bomber/Maps/Main");

	static UWorld* GetPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberPoolManager_UObjectCheckoutReturnAndReuse,
	"Bomber.PoolManagerRuntime.uobject_checkout_return_and_reuse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberPoolManager_UObjectCheckoutReturnAndReuse::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	// REQUIRED: validates the core single-object checkout, registration, return, lookup, and reuse contract.
	using namespace BomberPoolManagerRuntimeTests;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = BomberPoolManagerRuntimeTests::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		UPoolManagerSubsystem* PoolManager = World->GetSubsystem<UPoolManagerSubsystem>();
		if (!TestNotNull(TEXT("PoolManager subsystem must exist on the world"), PoolManager))
		{
			return true;
		}

		UObject* FirstObject = nullptr;
		FPoolObjectHandle FirstHandle = PoolManager->TakeFromPool(
			UCurveFloat::StaticClass(),
			FTransform::Identity,
			[&FirstObject](const FPoolObjectData& Data)
			{
				FirstObject = Data.PoolObject;
			},
			ESpawnRequestPriority::Critical);

		if (!TestTrue(TEXT("Taking from an empty UCurveFloat pool must return a valid handle"), FirstHandle.IsValid()))
		{
			return true;
		}

		if (!TestNotNull(TEXT("Critical checkout must synchronously provide the spawned UObject callback value"), FirstObject))
		{
			return true;
		}

		TestTrue(TEXT("The checked-out object must be registered in the pool"), PoolManager->IsRegistered(FirstObject));
		TestTrue(TEXT("The checked-out object must be active"), PoolManager->IsActive(FirstObject));
		TestEqual(TEXT("The registered object count must be one after first checkout"),
			PoolManager->GetRegisteredObjectsNum(UCurveFloat::StaticClass()), 1);
		TestEqual(TEXT("No free objects should remain immediately after checkout"),
			PoolManager->GetFreeObjectsNum(UCurveFloat::StaticClass()), 0);
		TestEqual(TEXT("Handle lookup must return the same object"),
			PoolManager->FindPoolObjectByHandle(FirstHandle).PoolObject.Get(), FirstObject);
		TestEqual(TEXT("Object lookup must return the same handle"),
			PoolManager->FindPoolHandleByObject(FirstObject), FirstHandle);

		TestTrue(TEXT("Returning the object must succeed"), PoolManager->ReturnToPool(FirstObject));
		TestTrue(TEXT("Returned object must still be registered"), PoolManager->IsRegistered(FirstObject));
		TestTrue(TEXT("Returned object must now be free"), PoolManager->IsFreeObjectInPool(FirstObject));
		TestEqual(TEXT("One free object should be available after return"),
			PoolManager->GetFreeObjectsNum(UCurveFloat::StaticClass()), 1);

		UObject* ReusedObject = nullptr;
		FPoolObjectHandle ReusedHandle = PoolManager->TakeFromPool(
			UCurveFloat::StaticClass(),
			FTransform::Identity,
			[&ReusedObject](const FPoolObjectData& Data)
			{
				ReusedObject = Data.PoolObject;
			},
			ESpawnRequestPriority::Critical);

		TestEqual(TEXT("Checkout after return must reuse the same UObject instance"), ReusedObject, FirstObject);
		TestEqual(TEXT("Reused checkout must preserve the existing handle"), ReusedHandle, FirstHandle);
		TestEqual(TEXT("Reusing an inactive object must not grow the registered object count"),
			PoolManager->GetRegisteredObjectsNum(UCurveFloat::StaticClass()), 1);
		TestTrue(TEXT("Reused object must be active again"), PoolManager->IsActive(ReusedObject));

		PoolManager->EmptyAllPools();
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
#else
	AddWarning(TEXT("PoolManager runtime tests require WITH_EDITOR PIE support."));
	return true;
#endif
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBomberPoolManager_ArrayCheckoutAndEmptyPoolState,
	"Bomber.PoolManagerRuntime.array_checkout_and_empty_pool_state",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBomberPoolManager_ArrayCheckoutAndEmptyPoolState::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	// REQUIRED: validates aggregate checkout, handle lookup, free counts, and pool cleanup.
	using namespace BomberPoolManagerRuntimeTests;

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World = BomberPoolManagerRuntimeTests::GetPIEWorld();
		if (!TestNotNull(TEXT("PIE world must be available"), World))
		{
			return true;
		}

		UPoolManagerSubsystem* PoolManager = World->GetSubsystem<UPoolManagerSubsystem>();
		if (!TestNotNull(TEXT("PoolManager subsystem must exist on the world"), PoolManager))
		{
			return true;
		}

		TArray<FPoolObjectHandle> Handles;
		TArray<FPoolObjectData> CallbackObjects;
		PoolManager->TakeFromPoolArray(
			Handles,
			UCurveFloat::StaticClass(),
			3,
			[&CallbackObjects](const TArray<FPoolObjectData>& SpawnedObjects)
			{
				CallbackObjects = SpawnedObjects;
			},
			ESpawnRequestPriority::Critical);

		if (!TestEqual(TEXT("Array checkout must return three handles"), Handles.Num(), 3))
		{
			return true;
		}
		TestEqual(TEXT("Critical array checkout must invoke the aggregate callback with three objects"), CallbackObjects.Num(), 3);
		TestEqual(TEXT("The pool must contain three registered objects after array checkout"),
			PoolManager->GetRegisteredObjectsNum(UCurveFloat::StaticClass()), 3);
		TestEqual(TEXT("No free objects should remain while the checked-out array is active"),
			PoolManager->GetFreeObjectsNum(UCurveFloat::StaticClass()), 0);

		TArray<FPoolObjectData> ObjectsByHandle;
		PoolManager->FindPoolObjectsByHandles(ObjectsByHandle, Handles);
		TestEqual(TEXT("All returned handles must resolve to registered pool objects"), ObjectsByHandle.Num(), 3);

		TArray<UObject*> ObjectsToReturn;
		for (const FPoolObjectData& Data : ObjectsByHandle)
		{
			if (Data.PoolObject)
			{
				ObjectsToReturn.Add(Data.PoolObject);
			}
		}
		TestEqual(TEXT("All resolved pool objects must be valid UObjects"), ObjectsToReturn.Num(), 3);
		TestTrue(TEXT("Returning the object array must succeed"), PoolManager->ReturnToPoolArray(ObjectsToReturn));
		TestEqual(TEXT("All returned objects must be counted as free"),
			PoolManager->GetFreeObjectsNum(UCurveFloat::StaticClass()), 3);

		PoolManager->EmptyPool(UCurveFloat::StaticClass());
		TestFalse(TEXT("EmptyPool must remove the class from the subsystem"),
			PoolManager->ContainsClassInPool(UCurveFloat::StaticClass()));
		TestEqual(TEXT("No registered objects should remain after EmptyPool"),
			PoolManager->GetRegisteredObjectsNum(UCurveFloat::StaticClass()), 0);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
#else
	AddWarning(TEXT("PoolManager runtime tests require WITH_EDITOR PIE support."));
	return true;
#endif
}
