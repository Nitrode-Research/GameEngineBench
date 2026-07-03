// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/RogueActionComponent.h"

#include "ActionSystem/RogueAction.h"
#include "../ActionRoguelike.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueActionComponent)


URogueActionComponent::URogueActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	SetIsReplicatedByDefault(true);

	bReplicateUsingRegisteredSubObjectList = true;
}


void URogueActionComponent::InitializeComponent()
{
	if (AttributeSet == nullptr)
	{
		AttributeSet = NewObject<URogueAttributeSet>(this, URogueAttributeSet::StaticClass());
	}

	Super::InitializeComponent();

	InitAttributeSet();

	if (GetOwner()->HasAuthority())
	{
		AddReplicatedSubObject(AttributeSet);
	}
}

void URogueActionComponent::BeginPlay()
{
	Super::BeginPlay();
}


void URogueActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


FRogueAttribute* URogueActionComponent::GetAttribute(FGameplayTag InAttributeTag)
{
	check(AttributeSet);

	FRogueAttribute** FoundAttribute = AttributeSet->AttributeCache.Find(InAttributeTag);
	if (FoundAttribute)
	{
		return *FoundAttribute;
	}

	return nullptr;
}

float URogueActionComponent::GetAttributeValue(FGameplayTag InAttributeTag)
{
	check(AttributeSet);

	FRogueAttribute** FoundAttribute = AttributeSet->AttributeCache.Find(InAttributeTag);
	if (FoundAttribute)
	{
		return (*FoundAttribute)->GetValue();
	}

	return 0.0f;
}


bool URogueActionComponent::K2_GetAttribute(FGameplayTag InAttributeTag, float& CurrentValue, float& Base, float& Delta)
{
	return false;
}


bool URogueActionComponent::ApplyAttributeChange(const FAttributeModification& Modification)
{
	return false;
}


void URogueActionComponent::RemoveDynamicAttributeListener(FAttributeChangedDynamicSignature Event) {}


bool URogueActionComponent::ApplyAttributeChange(FGameplayTag InAttributeTag, float InMagnitude, AActor* Instigator, EAttributeModifyType ModType, FGameplayTagContainer InContextTags)
{
	return false;
}


void URogueActionComponent::OnRep_AttributeSet() {}


void URogueActionComponent::InitAttributeSet()
{
	AttributeSet->InitializeAttributes(this);
}


FAttributeChangedSignature& URogueActionComponent::GetAttributeListenerDelegate(FGameplayTag InTag)
{
	static FAttributeChangedSignature Dummy;
	return Dummy;
}

void URogueActionComponent::AddDynamicAttributeListener(FAttributeChangedDynamicSignature Event, FGameplayTag InTag, bool bCallImmediately /*= false*/) {}

void URogueActionComponent::BroadcastAttributeChanged(FGameplayTag InTag, float InNewValue, FAttributeModification InModification) {}


void URogueActionComponent::SetDefaultAttributeSet(const TSubclassOf<URogueAttributeSet>& InNewClass)
{
	check(!HasBeenInitialized());

	const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get();
	AttributeSet = Cast<URogueAttributeSet>(ObjectInitializer.CreateDefaultSubobject(this, TEXT("Attributes"), InNewClass, InNewClass));
}


URogueActionComponent* URogueActionComponent::GetActionComponent(AActor* FromActor)
{
	return nullptr;
}


void URogueActionComponent::AddAction(AActor* Instigator, TSubclassOf<URogueAction> ActionClass) {}


void URogueActionComponent::RemoveAction(URogueAction* ActionToRemove) {}


URogueAction* URogueActionComponent::GetAction(TSubclassOf<URogueAction> ActionClass) const
{
	return nullptr;
}


bool URogueActionComponent::StartActionByName(AActor* Instigator, FGameplayTag ActionName)
{
	return false;
}


bool URogueActionComponent::StopActionByName(AActor* Instigator, FGameplayTag ActionName)
{
	return false;
}

void URogueActionComponent::StopAllActions() {}


void URogueActionComponent::ServerStartAction_Implementation(AActor* Instigator, FGameplayTag ActionName) {}


void URogueActionComponent::ServerStopAction_Implementation(AActor* Instigator, FGameplayTag ActionName) {}


void URogueActionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URogueActionComponent, Actions);
	DOREPLIFETIME(URogueActionComponent, AttributeSet);
}


void URogueActionComponent::OnRep_Actions() {}
