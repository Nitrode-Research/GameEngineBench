// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/RogueAction.h"
#include "ActionSystem/RogueActionComponent.h"
#include "ActionRoguelike.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueAction)


void URogueAction::Initialize(URogueActionComponent* NewActionComp) {}


bool URogueAction::CanStart_Implementation(AActor* Instigator)
{
	return false;
}


void URogueAction::StartAction_Implementation(AActor* Instigator) {}


void URogueAction::StopAction_Implementation(AActor* Instigator) {}


float URogueAction::CooldownTimeRemaining() const
{
	return 0.0f;
}

UWorld* URogueAction::GetWorld() const
{
	return nullptr;
}


TSoftObjectPtr<UTexture2D> URogueAction::GetIcon() const
{
	return {};
}


URogueActionComponent* URogueAction::GetOwningComponent() const
{
	return nullptr;
}


void URogueAction::OnRep_RepData() {}


bool URogueAction::IsRunning() const
{
	return false;
}


void URogueAction::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URogueAction, RepData);
	DOREPLIFETIME(URogueAction, TimeStarted);
	DOREPLIFETIME(URogueAction, CooldownUntil);
	DOREPLIFETIME(URogueAction, ActionComp);
}
