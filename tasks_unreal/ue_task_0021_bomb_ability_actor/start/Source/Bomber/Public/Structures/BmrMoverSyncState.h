// Copyright (c) Yevhenii Selivanov

#pragma once

// UE
#include "MoverTypes.h"

#include "BmrMoverSyncState.generated.h"

/**
 * Extends the Mover sync state to provide bomber-specific movement data.
 */
USTRUCT(BlueprintType)
struct BOMBER_API FBmrMoverSyncState : public FMoverDataStructBase
{
	GENERATED_BODY()

public:
	/** Number of collected Skate powerups that affect movement speed */
	UPROPERTY(BlueprintReadWrite)
	float SkatePowerupAttribute = 0.f;

	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;
};

template <>
struct TStructOpsTypeTraits<FBmrMoverSyncState> : public TStructOpsTypeTraitsBase2<FBmrMoverSyncState>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};