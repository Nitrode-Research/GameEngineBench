// Copyright (c) Yevhenii Selivanov

#include "Structures/BmrMoverSyncState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrMoverSyncState)

FMoverDataStructBase* FBmrMoverSyncState::Clone() const
{
	FBmrMoverSyncState* CopyPtr = new FBmrMoverSyncState(*this);
	return CopyPtr;
}

bool FBmrMoverSyncState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	const bool bSuccess = Super::NetSerialize(Ar, Map, bOutSuccess);

	Ar << SkatePowerupAttribute;

	bOutSuccess = bSuccess;
	return bSuccess;
}

void FBmrMoverSyncState::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);

	Out.Appendf("SkatePowerup[%f]\n", SkatePowerupAttribute);
}

bool FBmrMoverSyncState::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const FBmrMoverSyncState* AuthorityBmrState = static_cast<const FBmrMoverSyncState*>(&AuthorityState);

	// Reconcile if skate powerup value doesn't match
	static constexpr float Tolerance = 0.01f;
	return !FMath::IsNearlyEqual(SkatePowerupAttribute, AuthorityBmrState->SkatePowerupAttribute, Tolerance);
}

void FBmrMoverSyncState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FBmrMoverSyncState* ToState = static_cast<const FBmrMoverSyncState*>(&To);

	// Snap to target value - powerup counts should be discrete, not interpolated
	SkatePowerupAttribute = ToState->SkatePowerupAttribute;
}