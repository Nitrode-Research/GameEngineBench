// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueInteractionComponent.h"

#include "ActionRoguelike.h"
#include "Core/RogueGameplayInterface.h"
#include "DrawDebugHelpers.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "UI/RogueWorldUserWidget.h"
#include "Engine/OverlapResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueInteractionComponent)


static TAutoConsoleVariable CVarInteractionDebugDrawing(TEXT("game.Interaction.DebugDraw"),
	false,
	TEXT("Enable Debug Helper Rendering for Interaction Component. (0 = Disabled, 1 = Enabled)"),
	ECVF_Cheat);


URogueInteractionComponent::URogueInteractionComponent()
{
}


void URogueInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void URogueInteractionComponent::FindBestInteractable()
{
}


void URogueInteractionComponent::PrimaryInteract()
{
}


void URogueInteractionComponent::ServerInteract_Implementation(AActor* InFocus)
{
}
