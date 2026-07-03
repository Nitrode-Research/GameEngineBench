// MIT

#include "Components/Character/AlsxtAcrobaticActionComponent.h"
#include "Interfaces/AlsxtAcrobaticActionComponentInterface.h"
#include "Components/CapsuleComponent.h"
#include "Utility/AlsxtGameplayTags.h"
#include "AlsxtAnimationInstance.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UAlsxtAcrobaticActionComponent::UAlsxtAcrobaticActionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAlsxtAcrobaticActionComponent::BeginPlay()
{
	Super::BeginPlay();	
}


// Called every frame
void UAlsxtAcrobaticActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAlsxtAcrobaticActionComponent::TryAcrobaticAction()
{
	if (!GeneralAcrobaticActionSettings.bAcrobaticActions || !Settings->bAcrobaticActions || IAlsxtCharacterInterface::Execute_GetCharacterLocomotionMode(GetOwner()) == AlsLocomotionModeTags::Grounded || IAlsxtCharacterInterface::Execute_GetCharacterLocomotionAction(GetOwner()) == AlsLocomotionActionTags::Acrobatic)
	{
		return;
	}

	FGameplayTag AcrobaticActionType;
	DetermineAcrobaticActionType(AcrobaticActionType);

	if (AcrobaticActionType == ALSXTAcrobaticActionTypeTags::Flip)
	{
		BeginFlip();
	}
	else if (AcrobaticActionType == ALSXTAcrobaticActionTypeTags::WallJump && GeneralAcrobaticActionSettings.bEnableWallJump && Settings->bEnableWallJump)
	{
		BeginWallJump();
	}
	else if (AcrobaticActionType == ALSXTAcrobaticActionTypeTags::WallRun && GeneralAcrobaticActionSettings.bEnableWallRun && Settings->bEnableWallRun)
	{
		BeginWallRun();
	}
	else
	{
		return;
	}
}

void UAlsxtAcrobaticActionComponent::DetermineAcrobaticActionType(FGameplayTag& AcrobaticActionType)
{
	if (IAlsxtCharacterInterface::Execute_GetCharacterLocomotionAction(GetOwner()) != FGameplayTag::EmptyTag)
	{
		return;
	}
	const auto* Capsule{ IAlsxtCharacterInterface::Execute_GetCharacterCapsuleComponent(GetOwner()) };
	FVector ActorLocation = GetOwner()->GetActorLocation();
	float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	float Velocity = GetOwner()->GetVelocity().Size();
	TArray<AActor*> ActorsToIgnore;

	FVector ForwardTraceHalfSize {CapsuleRadius, (CapsuleRadius*1.25), CapsuleHalfHeight};
	FVector LateralTraceHalfSize{ (CapsuleRadius * 1.25), CapsuleRadius, CapsuleHalfHeight };
	FVector ForwardVector = IAlsxtCharacterInterface::Execute_GetCharacterCapsuleComponent(GetOwner())->GetForwardVector();
	FVector UpVector = GetOwner()->GetActorUpVector();
	FVector DownTraceStartPoint = ActorLocation + (UpVector * -CapsuleHalfHeight * 2.0f);
	FVector DownTraceEndPoint = DownTraceStartPoint + (UpVector * -GeneralAcrobaticActionSettings.DownTraceDistance);
	FVector ForwardTraceStartLocation = ActorLocation + (ForwardVector * 50.0f);
	FVector ForwardTraceEndLocation = ForwardTraceStartLocation + (ForwardVector * GeneralAcrobaticActionSettings.ForwardTraceDistance);

	FVector RightVector = IAlsxtCharacterInterface::Execute_GetCharacterCapsuleComponent(GetOwner())->GetRightVector();

	FVector RightTraceStartLocation = ActorLocation + (RightVector * 50.0f) + (ForwardVector * -25.0f);
	FVector RightTraceEndLocation = RightTraceStartLocation + (RightVector * GeneralAcrobaticActionSettings.LateralTraceDistance);
	FVector LeftTraceStartLocation = ActorLocation + (RightVector * -50.0f) + (ForwardVector * -25.0f);
	FVector LeftTraceEndLocation = LeftTraceStartLocation + (RightVector * -GeneralAcrobaticActionSettings.LateralTraceDistance);
	
	TArray<FHitResult> ForwardOutHits;
	TArray<FHitResult> DownOutHits;
	TArray<FHitResult> LeftOutHits;
	TArray<FHitResult> RightOutHits;
	
	//IALSXTCharacterInterface::Execute_GetCharacterGait(GetOwner()) == AlsGaitTags::Sprinting

	bool DownwardHit = UKismetSystemLibrary::BoxTraceMultiForObjects(GetWorld(), DownTraceStartPoint, DownTraceEndPoint, ForwardTraceHalfSize, GetOwner()->GetActorRotation(), GeneralAcrobaticActionSettings.TraceObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, DownOutHits, true, FLinearColor::Red, FLinearColor::Green, GeneralAcrobaticActionSettings.DebugTraceDuration);
	bool Falling = GetOwner()->GetVelocity().Z < 0.0;
	bool HasEnoughSpaceForFlip = !DownwardHit || DownwardHit && !Falling;
	bool ForwardHit = UKismetSystemLibrary::BoxTraceMultiForObjects(GetWorld(), ForwardTraceStartLocation, ForwardTraceEndLocation, ForwardTraceHalfSize, GetOwner()->GetActorRotation(), GeneralAcrobaticActionSettings.TraceObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, ForwardOutHits, true, FLinearColor::Red, FLinearColor::Green, GeneralAcrobaticActionSettings.DebugTraceDuration);
	bool LeftHit = UKismetSystemLibrary::BoxTraceMultiForObjects(GetWorld(), LeftTraceStartLocation, LeftTraceEndLocation, LateralTraceHalfSize, GetOwner()->GetActorRotation(), GeneralAcrobaticActionSettings.TraceObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, LeftOutHits, true, FLinearColor::Red, FLinearColor::Green, GeneralAcrobaticActionSettings.DebugTraceDuration);
	bool RightHit = UKismetSystemLibrary::BoxTraceMultiForObjects(GetWorld(), RightTraceStartLocation, RightTraceEndLocation, LateralTraceHalfSize, GetOwner()->GetActorRotation(), GeneralAcrobaticActionSettings.TraceObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, RightOutHits, true, FLinearColor::Red, FLinearColor::Green, GeneralAcrobaticActionSettings.DebugTraceDuration);

	if (DownwardHit)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "DownwardHit");
	}
	
	if (HasEnoughSpaceForFlip && (!ForwardHit && !LeftHit && !RightHit) || Velocity < GeneralAcrobaticActionSettings.MinimumSpeedForWallJump)
	{
		AcrobaticActionType = ALSXTAcrobaticActionTypeTags::Flip;
		return;
	}

	if (HasEnoughSpaceForFlip && (LeftHit || RightHit) && Velocity < GeneralAcrobaticActionSettings.MinimumSpeedForWallRun)
	{
		AcrobaticActionType = ALSXTAcrobaticActionTypeTags::Flip;
		return;
	}
	
	if ((ForwardHit & (!RightHit && !LeftHit)) && (Velocity > GeneralAcrobaticActionSettings.MinimumSpeedForWallJump) && IAlsxtAcrobaticActionComponentInterface::Execute_CanWallJump(GetOwner()))
	{
		AcrobaticActionType = ALSXTAcrobaticActionTypeTags::WallJump;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Wall Jump");
		return;

	}
	
	if ((LeftHit || (LeftHit && ForwardHit)) && (Velocity > GeneralAcrobaticActionSettings.MinimumSpeedForWallRun) && IAlsxtAcrobaticActionComponentInterface::Execute_CanWallRun(GetOwner()))
	{
		AcrobaticActionType = ALSXTAcrobaticActionTypeTags::WallRunLeft;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Wall Run Left");
	}

	if ((RightHit || (RightHit && ForwardHit)) && (Velocity > GeneralAcrobaticActionSettings.MinimumSpeedForWallRun) && IAlsxtAcrobaticActionComponentInterface::Execute_CanWallRun(GetOwner()))
	{
		AcrobaticActionType = ALSXTAcrobaticActionTypeTags::WallRunRight;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Wall Run Right");
	}

	// TODO
	// If Wall Run (Forward and 1 Lateral Hit)
	// Check if Forward and Lateral is the same Angle

}

void UAlsxtAcrobaticActionComponent::BeginFlip()
{
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerBeginFlip();
	}
}


void UAlsxtAcrobaticActionComponent::ServerBeginFlip_Implementation()
{
	MulticastBeginFlip();
}

void UAlsxtAcrobaticActionComponent::MulticastBeginFlip_Implementation()
{
	float Velocity = GetOwner()->GetVelocity().Size();
	FVector VNorm = GetOwner()->GetVelocity();

	VNorm.Normalize(0.0001f);
	float Direction =  FVector::DotProduct(GetOwner()->GetActorForwardVector(), VNorm);

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{

		if (Direction < 0.0)
		{
			// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Back"));
			IAlsxtCharacterInterface::Execute_GetCharacterAnimInstance(GetOwner())->Montage_Play(Settings->BackflipMontage, 1.0, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
		else if (Velocity < GeneralAcrobaticActionSettings.MaximumVelocityForBackflip)
		{
			// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Back"));
			IAlsxtCharacterInterface::Execute_GetCharacterAnimInstance(GetOwner())->Montage_Play(Settings->BackflipMontage, 1.0, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
		else
		{
			// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Front"));
			IAlsxtCharacterInterface::Execute_GetCharacterAnimInstance(GetOwner())->Montage_Play(Settings->FlipMontage, 1.0, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
	}
}

void UAlsxtAcrobaticActionComponent::BeginWallJump()
{

}

void UAlsxtAcrobaticActionComponent::BeginWallRun()
{

}

void UAlsxtAcrobaticActionComponent::UpdateWallRun()
{

}

void UAlsxtAcrobaticActionComponent::EndWallRun()
{

}
