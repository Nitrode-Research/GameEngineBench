// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "UObject/ObjectMacros.h"
#include "PCGComponent.h"
#include "ActorData.generated.h"


USTRUCT(BlueprintType)
struct FPCGActorSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Serialization")
	TSubclassOf<AActor> PCGActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Serialization")
	FTransform ActorTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Serialization")
	int32 PCGSeed {0}; // Example: if your PCG graph uses a seed

	// Example: Storing exposed parameters as a map of FName to FString (adapt as needed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Serialization")
	TMap<FName, FString> PCGParameters; 

	// Add other relevant PCG data here, e.g., references to input assets, specific settings
};


USTRUCT(BlueprintType)
struct FSerializedGameplayEffectData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayEffectSpecHandle EffectSpecHandle;

	UPROPERTY(BlueprintReadWrite)
	FActiveGameplayEffectHandle ActiveEffectHandle;

	// Add other relevant GameplayEffect data
};

USTRUCT(BlueprintType)
struct FSerializedASCData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FSerializedGameplayEffectData> ActiveEffects;

	UPROPERTY(BlueprintReadWrite)
	TMap<FGameplayAttribute, float> Attributes; // Example: Serialize attributes
};

USTRUCT(BlueprintType)
struct FMyAppliedGameplayEffectData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayEffectSpecHandle SpecHandle;

	UPROPERTY()
	FActiveGameplayEffectHandle ActiveHandle;
};

USTRUCT(BlueprintType)
struct FMyAttributeSetData
{
	GENERATED_BODY()

	// Example: Add properties for your attribute set values
	UPROPERTY()
	float Health {0};
};

USTRUCT(BlueprintType)
struct FMyActorSerializationData
{
	GENERATED_BODY()

	UPROPERTY()
	FString ActorName;

	UPROPERTY()
	FTransform ActorTransform;

	// Optional: If actors have specific data like PCGActorData
	UPROPERTY()
	TMap<FString, FString> PCGActorDataProperties; // Example: Store as key-value pairs

	UPROPERTY()
	TArray<FMyAppliedGameplayEffectData> AppliedEffects;

	UPROPERTY()
	FMyAttributeSetData AttributeData;

	UPROPERTY()
	TArray<FGameplayAbilitySpecDef> GrantedAbilities; // Example: Store ability definitions

	// Add other variables you want to serialize
	UPROPERTY()
	int32 SomeIntegerVariable {0};
    
	UPROPERTY()
	FString SomeStringVariable;
};

USTRUCT(BlueprintType)
struct FMyLevelSerializationData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMyActorSerializationData> ActorsData;
};