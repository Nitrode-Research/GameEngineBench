#pragma once

#include "CoreMinimal.h"
#include "Templates/Tuple.h"
#include "DataConfig/DcTypes.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"

struct DATACONFIGCORE_API FDcDeserializer : public FNoncopyable
{
	FDcResult Deserialize(FDcDeserializeContext& Ctx);

	void AddDirectHandler(FFieldClass* PropertyClass, FDcDeserializeDelegate&& Delegate);
	void AddDirectHandler(UClass* PropertyClass, FDcDeserializeDelegate&& Delegate);
	void AddPredicatedHandler(FDcDeserializePredicate&& Predicate, FDcDeserializeDelegate&& Delegate, const FName Name = NAME_None);
	void AddStructHandler(UStruct* Struct, FDcDeserializeDelegate&& Delegate);

	struct FPredicatedHandlerEntry
	{
		FDcDeserializePredicate Predicate;
		FDcDeserializeDelegate Handler;
		FName Name;
	};
	TArray<FPredicatedHandlerEntry> PredicatedDeserializers;

	TMap<UClass*, FDcDeserializeDelegate> UClassDeserializerMap;
	TMap<FFieldClass*, FDcDeserializeDelegate> FieldClassDeserializerMap;
	TMap<UStruct*, FDcDeserializeDelegate> StructDeserializeMap;
};

