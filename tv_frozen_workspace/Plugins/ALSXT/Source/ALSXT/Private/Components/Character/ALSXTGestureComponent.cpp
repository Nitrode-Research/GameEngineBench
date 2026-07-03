// MIT

#include "Components/Character/AlsxtGestureComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UAlsxtGestureComponent::UAlsxtGestureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// ...
}

void UAlsxtGestureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentGestureMontage, Parameters)
}

// Called when the game starts
void UAlsxtGestureComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
	AlsCharacter = Cast<AAlsCharacter>(GetOwner());
	
}


// Called every frame
void UAlsxtGestureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// Gesture

void UAlsxtGestureComponent::AddDesiredGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand)
{
	if (IAlsxtCharacterInterface::Execute_CanGesture(GetOwner()))
	{
		if (Character->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerAddDesiredGesture(Gesture, GestureHand);
		}
		else
		{
			AddGesture(Gesture, GestureHand);
		}
	}
}

void UAlsxtGestureComponent::ExitGesture(bool Immediate)
{
	if (Immediate)
	{
		Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.5f, CurrentGestureMontage);
	}
	else
	{
		Character->GetMesh()->GetAnimInstance()->Montage_JumpToSection(GestureSettings->ExitSectionName, CurrentGestureMontage);
	}
}

void UAlsxtGestureComponent::ServerAddDesiredGesture_Implementation(const FGameplayTag& Gesture, const FGameplayTag& GestureHand)
{
	AddDesiredGesture(Gesture, GestureHand);
}

void UAlsxtGestureComponent::AddGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand)
{
	if (IsValid(GestureSettings) && IAlsxtCharacterInterface::Execute_CanGesture(GetOwner()))
	{
		FAlsxtGestureMontages* FoundMontages = GestureSettings->Gestures.Find(Gesture);
		if (GestureHand == ALSXTHandTags::Left && IsValid(FoundMontages->LeftMontage))
		{
			Character->GetMesh()->GetAnimInstance()->Montage_Play(FoundMontages->LeftMontage);
			OnGesture(Gesture, GestureHand);
		}
		if (GestureHand == ALSXTHandTags::Right && IsValid(FoundMontages->RightMontage))
		{
			Character->GetMesh()->GetAnimInstance()->Montage_Play(FoundMontages->RightMontage);
			OnGesture(Gesture, GestureHand);
		}
	}
}

void UAlsxtGestureComponent::OnGesture_Implementation(const FGameplayTag& Gesture, const FGameplayTag& GestureHand) {}