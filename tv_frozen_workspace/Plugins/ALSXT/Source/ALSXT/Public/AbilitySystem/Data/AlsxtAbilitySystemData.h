// MIT

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "AlsxtAbilitySystemData.generated.h"

struct FAlsxtCustomAttributeInitializer;
class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

USTRUCT(Blueprintable)
struct ALSXT_API FAlsxtCustomAttributeMaxValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FScalableFloat MaxFloat{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute MaxAttribute{};
};

USTRUCT(Blueprintable)
struct ALSXT_API FAlsxtCustomAttributeInitializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FScalableFloat Value;
};

// Contains data used to initialize an Ability System Component.
USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtAbilitySystemInitializationData
{
	GENERATED_BODY()

	// An array of Attribute Sets to create.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSet<TSubclassOf<UAttributeSet>> AttributeSets;

	// A map of Attributes / float used to set base values.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TMap<FGameplayAttribute, FAlsxtCustomAttributeInitializer> AttributeBaseValues;

	// An Array of Gameplay Abilities to give.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSet<TSubclassOf<UGameplayAbility>> GameplayAbilities;

	// An array of Gameplay Effects to apply.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSet<TSubclassOf<UGameplayEffect>> GameplayEffects;

	// A container of GameplayTags to apply.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FGameplayTagContainer GameplayTags;
};

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtAbilitySystemInitializationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	FAlsxtAbilitySystemInitializationData AbilitySystemInitializationData;
};

// Returns the value of an Attribute based on the search type.
UENUM(BlueprintType)
enum class EAlsxtAttributeSearchType : uint8
{
	// Returns the final value of the Attribute including all stateful Gameplay Effect modifiers.
	FinalValue,

	// Returns the base value of the Attribute. (Excludes duration based Gameplay Effect modifiers)
	BaseValue,

	// Returns the Final Value minus the Base Value.
	BonusValue
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCustomContextDataBase
{
	GENERATED_BODY()
	
	virtual ~FAlsxtCustomContextDataBase() = default;
	
	/** Overridden to serialize new fields */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		return true;
	};
	
	FAlsxtCustomContextDataBase() {}
};

template <>
struct ALSXT_API TStructOpsTypeTraits<FAlsxtCustomContextDataBase> : public TStructOpsTypeTraitsBase2<FAlsxtCustomContextDataBase>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true // Necessary so that TSharedPtr<STRUCT> Data is copied around
	};
};


USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCustomGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()
	
	const TArray<FInstancedStruct>& GetAllCustomContextData() const { return CustomContextData; }
	TArray<FInstancedStruct>& GetMutableAllCustomContextData() { return CustomContextData; }

protected:
	UPROPERTY(EditAnywhere, meta=(ExcludeBaseStruct))
	TArray<FInstancedStruct> CustomContextData{};

	UPROPERTY(EditAnywhere, meta=(ExcludeBaseStruct))
	TObjectPtr<const AActor> TargetActor;
 
public:

	void SetTargetActor(const AActor* InTargetActor)
	{
		TargetActor = InTargetActor;
	}

	const AActor* GetTargetActor() const
	{
		return TargetActor.Get();
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	
	template <typename T>
		const T* FindCustomContextData() const
	{
		for (const auto& Fragment : CustomContextData)
		{
			if (const T* TypedFragment = Fragment.GetPtr<T>())
				return TypedFragment;
		}

		return nullptr;
	}

	/** Adds a Context Fragment to the ContextFragments array */
	template <typename T>
	void AddCustomContextData(const T& Fragment)
	{
		TInstancedStruct<FAlsxtCustomContextDataBase> InstancedStruct;
		InstancedStruct.InitializeAs<T>();
		T& Mutable = InstancedStruct.GetMutable<T>();
		Mutable = Fragment;
				
		// CustomContextData.Add(MoveTemp(InstancedStruct));
	}
	
	/** Returns the actual struct used for serialization, subclasses must override this! */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FAlsxtCustomGameplayEffectContext::StaticStruct();
	}
 
	/** Creates a copy of this context, used to duplicate for later modifications */
	virtual FAlsxtCustomGameplayEffectContext* Duplicate() const override
	{
		FAlsxtCustomGameplayEffectContext* NewContext = new FAlsxtCustomGameplayEffectContext();
		*NewContext = *this;
		NewContext->AddActors(Actors);
		
		NewContext->TargetActor = TargetActor;
		
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}
};
 
template <>
struct ALSXT_API TStructOpsTypeTraits<FAlsxtCustomGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FAlsxtCustomGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true // Necessary so that TSharedPtr<STRUCT> Data is copied around
	};
};


/** A "processed" version of GSGameplayEffectContainer that can be passed around and eventually applied */
USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGameplayEffectContainerSpec
{
	GENERATED_BODY()

public:
	FAlsxtGameplayEffectContainerSpec() {}

	/** Computed target data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	FGameplayAbilityTargetDataHandle TargetData;

	/** List of gameplay effects to apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TArray<FGameplayEffectSpecHandle> TargetGameplayEffectSpecs;

	/** Returns true if this has any valid effect specs */
	bool HasValidEffects() const;

	/** Returns true if this has any valid targets */
	bool HasValidTargets() const;

	/** Adds new targets to target data */
	void AddTargets(const TArray<FGameplayAbilityTargetDataHandle>& InTargetData, const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors);

	/** Clears target data */
	void ClearTargets();
};