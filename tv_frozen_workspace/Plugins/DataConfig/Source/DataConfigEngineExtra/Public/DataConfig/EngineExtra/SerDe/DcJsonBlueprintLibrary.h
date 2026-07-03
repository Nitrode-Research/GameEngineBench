#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DcJsonBlueprintLibrary.generated.h"

/// BP nodes for JSON conversion
///
/// DumpValue/LoadValue - takes everything except `Self`
/// DumpObject/LoadObject - handles object with exception that always expand the root object

UCLASS(BlueprintType)
class DATACONFIGENGINEEXTRA_API UDcJsonBlueprintLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, CustomThunk, Category="Json", meta = (DisplayName = "DataConfig Dump Value To Json", CustomStructureParam = "Struct", AutoCreateRefTerm = "Struct"))
	static UPARAM(DisplayName="Success") bool DumpValue(const int32& Struct, FString& OutJsonString);
	DECLARE_FUNCTION(execDumpValue);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="Json", meta = (DisplayName = "DataConfig Dump Object To Json"))
	static UPARAM(DisplayName="Success") bool DumpObject(const UObject* Obj, FString& OutJsonString);
	DECLARE_FUNCTION(execDumpObject);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="Json", meta = (DisplayName = "DataConfig Load Value From Json", CustomStructureParam = "Struct", AutoCreateRefTerm = "Struct"))
	static UPARAM(DisplayName="Success") bool LoadValue(const int32& Struct, const FString& InJsonString);
	DECLARE_FUNCTION(execLoadValue);                //  ^^^^^ need this const to make `Struct` as input

	UFUNCTION(BlueprintCallable, CustomThunk, Category="Json", meta = (DisplayName = "DataConfig Load Object From Json"))
	static UPARAM(DisplayName="Success") bool LoadObject(UObject* Obj, const FString& InJsonString);
	DECLARE_FUNCTION(execLoadObject);
};
