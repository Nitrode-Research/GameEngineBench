// Copyright (c) Yevhenii Selivanov

#include "PoolManagerSubsystem.h"

#include "Data/PoolObjectState.h"
#include "Data/SpawnRequest.h"
#include "Factories/PoolFactory_UObject.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerSubsystem)

UPoolManagerSubsystem* UPoolManagerSubsystem::GetPoolManagerByClass(TSubclassOf<UPoolManagerSubsystem> OptionalClass, const UObject* OptionalWorldContext)
{
	if (!OptionalClass)
	{
		OptionalClass = StaticClass();
	}

	const UWorld* World = OptionalWorldContext && GEngine
		? GEngine->GetWorldFromContextObject(OptionalWorldContext, EGetWorldErrorMode::ReturnNull)
		: (GEngine ? GEngine->GetCurrentPlayWorld() : nullptr);

#if WITH_EDITOR
	if (!World && GIsEditor && GEditor)
	{
		World = GEditor->IsPlaySessionInProgress()
			? (GEditor->GetCurrentPlayWorld() ? GEditor->GetCurrentPlayWorld() : (GEditor->GetPIEWorldContext() ? GEditor->GetPIEWorldContext()->World() : nullptr))
			: GEditor->GetEditorWorldContext().World();
		World = World ? World : GWorld;
	}
#endif

	return World ? Cast<UPoolManagerSubsystem>(World->GetSubsystemBase(OptionalClass)) : nullptr;
}

void UPoolManagerSubsystem::BPTakeFromPool(const UClass* ObjectClass, const FTransform& Transform, const FOnTakenFromPool& Completed, ESpawnRequestPriority Priority)
{
}

FPoolObjectHandle UPoolManagerSubsystem::TakeFromPool(const UClass* ObjectClass, const FTransform& Transform, const FOnSpawnCallback& Completed, ESpawnRequestPriority Priority)
{
	return FPoolObjectHandle::EmptyHandle;
}

const FPoolObjectData* UPoolManagerSubsystem::TakeFromPoolOrNull(const UClass* ObjectClass, const FTransform& Transform)
{
	return nullptr;
}

void UPoolManagerSubsystem::BPTakeFromPoolArray(const UClass* ObjectClass, int32 Amount, const FOnTakenFromPoolArray& Completed, ESpawnRequestPriority Priority)
{
}

void UPoolManagerSubsystem::TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, const UClass* ObjectClass, int32 Amount, const FOnSpawnAllCallback& Completed, ESpawnRequestPriority Priority)
{
	OutHandles.Reset();
}

void UPoolManagerSubsystem::TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, TArray<FSpawnRequest>& InRequests, const FOnSpawnAllCallback& Completed)
{
	OutHandles.Reset();
}

void UPoolManagerSubsystem::TakeFromPoolArrayOrNull(TArray<FPoolObjectData>& OutObjects, TArray<FSpawnRequest>& InRequests)
{
	OutObjects.Reset();
}

bool UPoolManagerSubsystem::ReturnToPool_Implementation(UObject* Object)
{
	return false;
}

bool UPoolManagerSubsystem::ReturnToPool(const FPoolObjectHandle& Handle)
{
	return false;
}

bool UPoolManagerSubsystem::ReturnToPoolArray_Implementation(const TArray<UObject*>& Objects)
{
	return false;
}

bool UPoolManagerSubsystem::ReturnToPoolArray(const TArray<FPoolObjectHandle>& Handles)
{
	return false;
}

bool UPoolManagerSubsystem::RegisterObjectInPool_Implementation(const FPoolObjectData& InData, bool bNotify)
{
	return false;
}

FPoolObjectHandle UPoolManagerSubsystem::CreateNewObjectInPool_Implementation(const FSpawnRequest& InRequest)
{
	return FPoolObjectHandle::EmptyHandle;
}

void UPoolManagerSubsystem::CreateNewObjectsArrayInPool(TArray<FSpawnRequest>& InRequests, TArray<FPoolObjectHandle>& OutAllHandles, const FOnSpawnAllCallback& Completed)
{
	OutAllHandles.Reset();
}

void UPoolManagerSubsystem::AddFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass)
{
}

void UPoolManagerSubsystem::RemoveFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass)
{
}

UPoolFactory_UObject* UPoolManagerSubsystem::FindPoolFactoryChecked(const UClass* ObjectClass) const
{
	return nullptr;
}

const UClass* UPoolManagerSubsystem::GetObjectClassByFactory(const TSubclassOf<UPoolFactory_UObject>& FactoryClass)
{
	return nullptr;
}

void UPoolManagerSubsystem::InitializeAllFactories()
{
}

void UPoolManagerSubsystem::ClearAllFactories()
{
	AllFactories.Reset();
}

void UPoolManagerSubsystem::EmptyPool_Implementation(const UClass* ObjectClass)
{
}

void UPoolManagerSubsystem::EmptyAllPools_Implementation()
{
	Pools.Reset();
}

void UPoolManagerSubsystem::EmptyAllByPredicate(const TFunctionRef<bool(const UObject* Object)> Predicate)
{
}

EPoolObjectState UPoolManagerSubsystem::GetPoolObjectState_Implementation(const UObject* Object) const
{
	return EPoolObjectState::None;
}

bool UPoolManagerSubsystem::ContainsObjectInPool_Implementation(const UObject* Object) const
{
	return false;
}

bool UPoolManagerSubsystem::ContainsClassInPool_Implementation(const UClass* ObjectClass) const
{
	return false;
}

bool UPoolManagerSubsystem::IsActive_Implementation(const UObject* Object) const
{
	return false;
}

bool UPoolManagerSubsystem::IsFreeObjectInPool_Implementation(const UObject* Object) const
{
	return false;
}

int32 UPoolManagerSubsystem::GetFreeObjectsNum_Implementation(const UClass* ObjectClass) const
{
	return 0;
}

bool UPoolManagerSubsystem::IsRegistered_Implementation(const UObject* Object) const
{
	return false;
}

int32 UPoolManagerSubsystem::GetRegisteredObjectsNum_Implementation(const UClass* ObjectClass) const
{
	return 0;
}

const FPoolObjectData& UPoolManagerSubsystem::FindPoolObjectByHandle(const FPoolObjectHandle& Handle) const
{
	return FPoolObjectData::EmptyObject;
}

const FPoolObjectHandle& UPoolManagerSubsystem::FindPoolHandleByObject(const UObject* Object) const
{
	return FPoolObjectHandle::EmptyHandle;
}

void UPoolManagerSubsystem::FindPoolObjectsByHandles(TArray<FPoolObjectData>& OutObjects, const TArray<FPoolObjectHandle>& InHandles) const
{
	OutObjects.Reset();
}

void UPoolManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPoolManagerSubsystem::Deinitialize()
{
	ClearAllFactories();
	Pools.Reset();
	Super::Deinitialize();
}

void UPoolManagerSubsystem::OnGameFeatureDeactivating(const UGameFeatureData* GameFeatureData, FGameFeatureDeactivatingContext& Context, const FString& PluginURL)
{
}

FPoolContainer& UPoolManagerSubsystem::FindPoolOrAdd(const UClass* ObjectClass)
{
	static FPoolContainer StubPool;
	StubPool = FPoolContainer();
	return StubPool;
}

FPoolContainer* UPoolManagerSubsystem::FindPool(const UClass* ObjectClass)
{
	return nullptr;
}

void UPoolManagerSubsystem::SetObjectStateInPool(EPoolObjectState NewState, UObject& InObject, FPoolContainer& InPool, bool bNotify)
{
}

void UPoolManagerSubsystem::RemoveObjectInPool(UObject& InObject, FPoolContainer& InPool)
{
}
