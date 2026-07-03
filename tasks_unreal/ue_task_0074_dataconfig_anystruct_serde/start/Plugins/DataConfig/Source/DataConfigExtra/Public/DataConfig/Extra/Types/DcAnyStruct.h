#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointerInternals.h"
#include "DcAnyStruct.generated.h"

USTRUCT(BlueprintType)
struct DATACONFIGEXTRA_API FDcAnyStruct
{
	GENERATED_BODY()

	FDcAnyStruct(SharedPointerInternals::FNullTag* = nullptr) {}
	FDcAnyStruct(void* InDataPtr, UScriptStruct* InStructClass) {}
	FDcAnyStruct(FDcAnyStruct const& InOther) = default;
	FDcAnyStruct(FDcAnyStruct&& InOther) = default;

	void Reset()
	{
		DataPtr = nullptr;
		StructClass = nullptr;
	}

	FDcAnyStruct& operator=(SharedPointerInternals::FNullTag*)
	{
		Reset();
		return *this;
	}

	FDcAnyStruct& operator=(FDcAnyStruct const& InOther) = default;
	FDcAnyStruct& operator=(FDcAnyStruct&& InOther) = default;

	int32 GetSharedReferenceCount() const
	{
		return 0;
	}

	bool IsValid() const
	{
		return false;
	}

	template<class T>
	FDcAnyStruct(T* StructPtr)
	{
	}

	template<class T>
	T* GetChecked() const
	{
		return nullptr;
	}

	void* DataPtr = nullptr;
	UScriptStruct* StructClass = nullptr;

	void DebugDump();
};
