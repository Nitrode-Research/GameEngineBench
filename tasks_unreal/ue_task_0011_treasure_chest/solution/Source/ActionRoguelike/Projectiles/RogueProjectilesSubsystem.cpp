// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueProjectilesSubsystem.h"

#include "ActionRoguelike.h"
#include "GenericTeamAgentInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraDataChannel.h"
#include "NiagaraDataChannelAccessor.h"
#include "NiagaraDataChannelFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "RogueProjectileData.h"
#include "Core/RogueGameState.h"
#include "ProfilingDebugging/CountersTrace.h"

TRACE_DECLARE_INT_COUNTER(LightweightProjectilesCount, TEXT("Game/DataOnlyProjectileCount"));


int32 URogueProjectilesSubsystem::CreateProjectile(FVector InPosition, FVector InDirection, URogueProjectileData* ProjectileConfig, AActor* InstigatorActor)
{
	return 0;
}

void URogueProjectilesSubsystem::InternalCreateProjectile(FVector InPosition, FVector InDirection, URogueProjectileData* ProjectileConfig, AActor* InstigatorActor, uint32 NewID)
{
}


void URogueProjectilesSubsystem::RemoveProjectileID(uint32 IdToRemove)
{
}


void URogueProjectilesSubsystem::Tick(float DeltaTime)
{
}


void URogueProjectilesSubsystem::SpawnImpactFX(const UWorld* World, const FProjectileItem& ProjConfig, FVector ImpactPosition, FRotator ImpactRotation)
{
}

uint32 URogueProjectilesSubsystem::GetUniqueProjID(FVector InPos, float InGameTime)
{
	return 0;
}


TStatId URogueProjectilesSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URogueProjectilesSubsystem, STATGROUP_Tickables);
}


bool URogueProjectilesSubsystem::HasAuthority() const
{
	return false;
}
