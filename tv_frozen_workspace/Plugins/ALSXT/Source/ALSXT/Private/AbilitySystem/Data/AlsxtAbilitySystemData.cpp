// MIT

#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"

bool FAlsxtCustomGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bool bCombinedSuccess = FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

    enum RepFlag
    {
        REP_CustomContextData,
        REP_TargetActor,
        REP_MAX
    };

    uint16 RepBits = 0;
    if (Ar.IsSaving())
    {
        if (!CustomContextData.IsEmpty())
        {
            RepBits |= 1 << REP_CustomContextData;
        }

        if (TargetActor)
        {
            RepBits |= 1 << REP_TargetActor;
        }
    }

    Ar.SerializeBits(&RepBits, REP_MAX);
    if (RepBits & (1 << REP_CustomContextData))
    {
        bCombinedSuccess &= SafeNetSerializeTArray_WithNetSerialize<31>(Ar, CustomContextData, Map);
    }
    if (RepBits & (1 << REP_TargetActor))
    {
        Ar << TargetActor;
    }

    return bCombinedSuccess;
}

bool FAlsxtGameplayEffectContainerSpec::HasValidEffects() const
{
    return !TargetGameplayEffectSpecs.IsEmpty();
}

bool FAlsxtGameplayEffectContainerSpec::HasValidTargets() const
{
    return TargetData.Num() > 0;
}

void FAlsxtGameplayEffectContainerSpec::AddTargets(const TArray<FGameplayAbilityTargetDataHandle>& InTargetData, const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors)
{
    for (const FGameplayAbilityTargetDataHandle& TD : InTargetData)
    {
        TargetData.Append(TD);
    }

    for (const FHitResult& HitResult : HitResults)
    {
        FGameplayAbilityTargetData_SingleTargetHit* const NewData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
        TargetData.Add(NewData);
    }

    if (!TargetActors.IsEmpty())
    {
        FGameplayAbilityTargetData_ActorArray* const NewData = new FGameplayAbilityTargetData_ActorArray();
        NewData->TargetActorArray.Append(TargetActors);
        TargetData.Add(NewData);
    }
}

void FAlsxtGameplayEffectContainerSpec::ClearTargets()
{
    TargetData.Clear();
}