#pragma once

#include "GameFramework/RootMotionSource.h"
#include "Settings/AlsxtImpactReactionSettings.h"
#include "State/AlsxtImpactReactionState.h"
#include "AlsxtRootMotionSource_ImpactReaction.generated.h"

class UAlsxtImpactReactionSettings;

USTRUCT()
struct ALSXT_API FAlsxtRootMotionSource_ImpactReaction : public FRootMotionSource
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UAlsxtImpactReactionSettings> ImpactReactionSettings{nullptr};

	UPROPERTY()
	FAlsxtImpactReactionState ImpactReactionState;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> TargetPrimitive;

	UPROPERTY()
	FVector TargetRelativeLocation{ForceInit};

	UPROPERTY()
	FRotator TargetRelativeRotation{ForceInit};

	UPROPERTY()
	FVector ActorFeetLocationOffset{ForceInit};

	UPROPERTY()
	FRotator ActorRotationOffset{ForceInit};

	UPROPERTY(Meta = (ClampMin = 0, ForceUnits = "cm"))
	float ImpactHeight{0.0f};

public:
	FAlsxtRootMotionSource_ImpactReaction();

	virtual FRootMotionSource* Clone() const override;

	virtual bool Matches(const FRootMotionSource* Other) const override;

	virtual void PrepareRootMotion(float SimulationDeltaTime, float DeltaTime, const ACharacter& Character,
	                               const UCharacterMovementComponent& Movement) override;

	virtual bool NetSerialize(FArchive& Archive, UPackageMap* Map, bool& bSuccess) override;

	virtual UScriptStruct* GetScriptStruct() const override;

	virtual FString ToSimpleString() const override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
};

template <>
struct TStructOpsTypeTraits<FAlsxtRootMotionSource_ImpactReaction> : public TStructOpsTypeTraitsBase2<FAlsxtRootMotionSource_ImpactReaction>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
