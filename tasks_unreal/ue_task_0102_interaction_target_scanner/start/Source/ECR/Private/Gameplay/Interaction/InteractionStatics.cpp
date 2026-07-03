// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/InteractionStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Gameplay/Interaction/IInteractableTarget.h"

UInteractionStatics::UInteractionStatics()
	: Super(FObjectInitializer::Get())
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


AActor* UInteractionStatics::GetActorFromInteractableTarget(TScriptInterface<IInteractableTarget> InteractableTarget)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UInteractionStatics::GetInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UInteractionStatics::AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UInteractionStatics::AppendInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

