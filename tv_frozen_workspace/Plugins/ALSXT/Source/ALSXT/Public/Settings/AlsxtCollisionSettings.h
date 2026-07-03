#pragma once

#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "AlsxtCollisionSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCollisionEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS", Meta = (Categories = "Als.Action Strength"))
	FGameplayTagContainer ImpactStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<UNiagaraSystem*> Particles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	USoundBase* Sound{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<UMaterialInterface*> Decals;

};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCollisionEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FAlsxtCollisionEffect> Effects;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtCollisionEffectMap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ForceInlineRow))
	TMap<TEnumAsByte<EPhysicalSurface>, FAlsxtCollisionEffects> Effects;

};

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtCollisionEffectsSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ForceInlineRow))
	TMap<TEnumAsByte<EPhysicalSurface>, FAlsxtCollisionEffectMap> CollisionEffectMap;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtDamageResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float Force{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float BaseDamage{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float ShockDamage{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float PunctureDamage{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float BurnDamage{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float ExplosiveDamage{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float Penetration{ 0.0f };
};