#pragma once

#include "GameFramework/RootMotionSource.h"
#include "Settings/AlsxtCombatSettings.h"
#include "State/AlsxtCombatState.h"
#include "AlsxtRootMotionSource_CombatAttack.generated.h"

class UALSXTCombatAttackSettings;

USTRUCT()
struct ALSXT_API FAlsxtRootMotionSource_CombatAttack : public FRootMotionSource
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UAlsxtCombatSettings> CombatSettings{nullptr};

	UPROPERTY()
	FAlsxtCombatState CombatState;

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
	float AttackHeight{0.0f};

public:
	FAlsxtRootMotionSource_CombatAttack();

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
struct TStructOpsTypeTraits<FAlsxtRootMotionSource_CombatAttack> : public TStructOpsTypeTraitsBase2<FAlsxtRootMotionSource_CombatAttack>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
