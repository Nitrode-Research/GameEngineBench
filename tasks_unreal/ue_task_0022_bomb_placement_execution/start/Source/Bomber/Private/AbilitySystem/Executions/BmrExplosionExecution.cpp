// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Executions/BmrExplosionExecution.h"

// Bomber
#include "AbilitySystem/Attributes/BmrHealthAttributeSet.h"
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrGeneratedMap.h"
#include "Bomber.h"
#include "Components/BmrMapComponent.h"
#include "DataAssets/BmrBombDataAsset.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "UtilityLibraries/BmrActorUtilsLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "AbilitySystemGlobals.h"
#include "Structures/BmrGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrExplosionExecution)

UBmrExplosionExecution::UBmrExplosionExecution()
{
}

void UBmrExplosionExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
}
