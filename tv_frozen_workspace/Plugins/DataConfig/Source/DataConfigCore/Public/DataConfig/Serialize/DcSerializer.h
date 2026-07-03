#pragma once

#include "CoreMinimal.h"
#include "DataConfig/DcTypes.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"

struct DATACONFIGCORE_API FDcSerializer : public FNoncopyable
{
	FDcResult Serialize(FDcSerializeContext& Ctx);

	void AddDirectHandler(FFieldClass* PropertyClass, FDcSerializeDelegate&& Delegate);
	void AddDirectHandler(UClass* PropertyClass, FDcSerializeDelegate&& Delegate);
	void AddPredicatedHandler(FDcSerializePredicate&& Predicate, FDcSerializeDelegate&& Delegate, const FName Name = NAME_None);
	void AddStructHandler(UStruct* Struct, FDcSerializeDelegate&& Delegate);

	struct FPredicatedHandlerEntry
	{
		FDcSerializePredicate Predicate;
		FDcSerializeDelegate Handler;
		FName Name;
	};
	TArray<FPredicatedHandlerEntry> PredicatedSerializers;

	TMap<UClass*, FDcSerializeDelegate> UClassSerializerMap;
	TMap<FFieldClass*, FDcSerializeDelegate> FieldClassSerializerMap;
	TMap<UStruct*, FDcSerializeDelegate> StructSerializerMap;
};


