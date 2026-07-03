#pragma once

#include "CoreMinimal.h"
#include "ISpudObject.h"
#include "SpudPropertySerializationTestTypes.generated.h"

UENUM()
enum class ESpudPropertyTestEnum : uint8
{
	Unset,
	Alpha,
	Beta
};

USTRUCT()
struct FSpudPropertyInnerStruct
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	int32 SavedInt = 0;

	UPROPERTY(SaveGame)
	FVector SavedVector = FVector::ZeroVector;

	UPROPERTY()
	int32 RuntimeOnlyInt = 0;
};

UCLASS()
class USpudPropertyNestedBase : public UObject, public ISpudObjectCallback
{
	GENERATED_BODY()

public:
	static int32 CallbackSequence;

	UPROPERTY(SaveGame)
	int32 SavedNestedInt = 0;

	UPROPERTY()
	int32 RuntimeOnlyNestedInt = 0;

	int32 PreStoreOrder = 0;
	int32 PostStoreOrder = 0;
	int32 PreRestoreOrder = 0;
	int32 PostRestoreOrder = 0;

	virtual void SpudPreStore_Implementation(const USpudState* State) override;
	virtual void SpudPostStore_Implementation(const USpudState* State) override;
	virtual void SpudPreRestore_Implementation(const USpudState* State) override;
	virtual void SpudPostRestore_Implementation(const USpudState* State) override;
};

UCLASS()
class USpudPropertyNestedDerived : public USpudPropertyNestedBase
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FString SavedDerivedString;
};

UCLASS()
class USpudPropertyRootObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	bool SavedBool = false;

	UPROPERTY(SaveGame)
	int32 SavedInt = 0;

	UPROPERTY(SaveGame)
	double SavedDouble = 0.0;

	UPROPERTY(SaveGame)
	FString SavedString;

	UPROPERTY(SaveGame)
	FName SavedName;

	UPROPERTY(SaveGame)
	FText SavedText;

	UPROPERTY(SaveGame)
	ESpudPropertyTestEnum SavedEnum = ESpudPropertyTestEnum::Unset;

	UPROPERTY(SaveGame)
	FVector SavedVector = FVector::ZeroVector;

	UPROPERTY(SaveGame)
	TArray<int32> SavedIntArray;

	UPROPERTY(SaveGame)
	TSubclassOf<UObject> SavedClass;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> SavedSoftObject;

	UPROPERTY(SaveGame)
	FSpudPropertyInnerStruct SavedStruct;

	UPROPERTY(SaveGame)
	TMap<FString, int32> SavedMap;

	UPROPERTY(SaveGame)
	TArray<FSpudPropertyInnerStruct> SavedStructArray;

	UPROPERTY(SaveGame)
	TObjectPtr<USpudPropertyNestedBase> NestedObject = nullptr;

	UPROPERTY(SaveGame)
	TObjectPtr<USpudPropertyNestedBase> NullNestedObject = nullptr;

	UPROPERTY()
	int32 RuntimeOnlyInt = 0;
};
