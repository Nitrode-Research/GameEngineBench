// MIT

#include "Components/Character/AlsxtCombatComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Utility/AlsUtility.h"
#include "Utility/AlsMacros.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utility/AlsxtEnums.h"
#include "Settings/AlsxtCharacterSettings.h"
#include "Math/Vector.h"
#include "GameFramework/Character.h"
#include "AlsxtCharacter.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtTargetLockInterface.h"
#include "Interfaces/AlsxtHeldItemInterface.h"
#include "Interfaces/AlsxtCombatInterface.h"
#include "Interfaces/AlsxtCharacterSoundComponentInterface.h"
#include "Interfaces/AlsxtCharacterCustomizationComponentInterface.h"
#include "AlsxtBlueprintFunctionLibrary.h"
#include "Landscape.h"

// Sets default values for this component's properties
UAlsxtCombatComponent::UAlsxtCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// ...
}

// Called when the game starts
void UAlsxtCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());

	// UEnhancedInputComponent* EnhancedInput{ Cast<UEnhancedInputComponent>(GetOwner()) };
	// if (IsValid(EnhancedInput))
	// {
	// 	//FSetupPlayerInputComponentDelegate Del = Character->OnSetupPlayerInputComponentUpdated;
	// 	//Del.AddUniqueDynamic(this, &UAlsxtCombatComponent::SetupInputComponent(EnhancedInput));
	// }
	TargetTraceTimerDelegate.BindUFunction(this, "TryTraceForTargets");
	AttackTraceTimerDelegate.BindUFunction(this, "AttackCollisionTrace");
	MoveToTargetTimerDelegate.BindUFunction(this, "UpdateMoveToTarget");
}

// Called every frame
void UAlsxtCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshAttack(DeltaTime);
}

float UAlsxtCombatComponent::GetAngle(FVector Target)
{
	float resultAngleInRadians = 0.0f;
	FVector PlayerLocation = GetOwner()->GetActorLocation();
	PlayerLocation.Normalize();
	Target.Normalize();

	auto crossProduct = PlayerLocation.Cross(Target);
	auto dotProduct = PlayerLocation.Dot(Target);

	if (crossProduct.Z > 0)
	{
		resultAngleInRadians = acosf(dotProduct);
	}
	else
	{
		resultAngleInRadians = -1 * acosf(dotProduct);
	}

	auto resultAngleInDegrees = FMath::RadiansToDegrees(resultAngleInRadians);
	return resultAngleInDegrees;
}

bool UAlsxtCombatComponent::IsTartgetObstructed()
{
	FVector CharLoc = GetOwner()->GetActorLocation();
	TArray<FHitResult> OutHits;

	FCollisionObjectQueryParams ObjectQueryParameters;
	for (const auto ObjectType : CombatSettings.ObstructionTraceObjectTypes)
	{
		ObjectQueryParameters.AddObjectTypesToQuery(UCollisionProfile::Get()->ConvertToCollisionChannel(false, ObjectType));
	}
	FCollisionQueryParams CollisionQueryParameters;
	CollisionQueryParameters.AddIgnoredActor(GetOwner());
	CollisionQueryParameters.AddIgnoredActor(CurrentTarget.HitResult.GetActor());

	if (GetOwner()->GetWorld()->LineTraceMultiByObjectType(OutHits, CharLoc, CurrentTarget.HitResult.GetActor()->GetActorLocation(), ObjectQueryParameters, CollisionQueryParameters))
	{
		bool ValidObstruction = false;
		for (FHitResult Hit : OutHits)
		{		
			ValidObstruction = true;
			if (CombatSettings.UnlockWhenTargetIsObstructed)
			{
				ClearCurrentTarget();
			}
			return true;

		}
		return ValidObstruction;		
	}
	else
	{
		return false;
	}
}

void UAlsxtCombatComponent::TryTraceForTargets()
{
}

void UAlsxtCombatComponent::TraceForTargets(TArray<FTargetHitResultEntry>& Targets)
{
	Targets.Reset();
}

void UAlsxtCombatComponent::GetClosestTarget()
{
	DisengageAllTargets();
}

void UAlsxtCombatComponent::SetCurrentTarget(const FTargetHitResultEntry& NewTarget)
{
	ClearCurrentTarget();
}

void UAlsxtCombatComponent::ClearCurrentTarget()
{
	CurrentTarget.Valid = false;
	CurrentTarget.DistanceFromPlayer = 340282346638528859811704183484516925440.0f;
	CurrentTarget.AngleFromCenter = 361.0f;
	CurrentTarget.HitResult = FHitResult(ForceInit);
	TargetDynamicMaterials.Empty();
}

void UAlsxtCombatComponent::DisengageAllTargets()
{
	ClearCurrentTarget();

	// Clear Trace Timer
	GetWorld()->GetTimerManager().ClearTimer(TargetTraceTimerHandle);
}

void UAlsxtCombatComponent::GetTargetLeft()
{
}

void UAlsxtCombatComponent::GetTargetRight()
{
}

void UAlsxtCombatComponent::RotatePlayerToTarget(FTargetHitResultEntry Target)
{
}

void UAlsxtCombatComponent::DashToTarget()
{

}

// Combat State
void UAlsxtCombatComponent::SetCombatState(const FAlsxtCombatState& NewCombatState)
{
	const auto PreviousCombatState{ CombatState };

	CombatState = NewCombatState;

	OnCombatStateChanged(PreviousCombatState);

	if ((Character->GetLocalRole() == ROLE_AutonomousProxy) && Character->IsLocallyControlled())
	{
		ServerSetCombatState(NewCombatState);
	}
}

void UAlsxtCombatComponent::ServerSetCombatState_Implementation(const FAlsxtCombatState& NewCombatState)
{
	SetCombatState(NewCombatState);
}

void UAlsxtCombatComponent::ServerProcessNewCombatState_Implementation(const FAlsxtCombatState& NewCombatState)
{
	ProcessNewCombatState(NewCombatState);
}

void UAlsxtCombatComponent::OnReplicate_CombatState(const FAlsxtCombatState& PreviousCombatState)
{
	OnCombatStateChanged(PreviousCombatState);
}

void UAlsxtCombatComponent::OnCombatStateChanged_Implementation(const FAlsxtCombatState& PreviousCombatState) {}

// Attack

AActor* UAlsxtCombatComponent::TraceForPotentialAttackTarget(float Distance)
{
	return nullptr;
}

void UAlsxtCombatComponent::BeginMoveToTarget()
{

}

void UAlsxtCombatComponent::UpdateMoveToTarget()
{
}

void UAlsxtCombatComponent::EndMoveToTarget()
{

}

void UAlsxtCombatComponent::BeginAttackCollisionTrace(FAlsxtCombatAttackTraceSettings TraceSettings)
{
	CurrentAttackTraceSettings = TraceSettings;
	AttackTraceLastHitActors.Empty();
}

void UAlsxtCombatComponent::AttackCollisionTrace()
{
}

void UAlsxtCombatComponent::EndAttackCollisionTrace()
{
	// Clear Attack Trace Timer
	GetWorld()->GetTimerManager().ClearTimer(AttackTraceTimerHandle);

	// Reset Attack Trace Settings
	CurrentAttackTraceSettings.Start = { 0.0f, 0.0f, 0.0f };
	CurrentAttackTraceSettings.End = { 0.0f, 0.0f, 0.0f };
	CurrentAttackTraceSettings.Radius = { 0.0f };

	// Empty AttackTraceLastHitActors Array
	AttackTraceLastHitActors.Empty();
}

void UAlsxtCombatComponent::Attack(const FGameplayTag& ActionType, const FGameplayTag& AttackType, const FGameplayTag& Strength, const float BaseDamage, const float PlayRate)
{
}

void UAlsxtCombatComponent::SetupInputComponent(UEnhancedInputComponent* PlayerInputComponent)
{
	if (PrimaryAction)
	{
		PlayerInputComponent->BindAction(PrimaryAction, ETriggerEvent::Triggered, this, &UAlsxtCombatComponent::InputPrimaryAction);
	}
}

void UAlsxtCombatComponent::InputPrimaryAction()
{
	if ((Character->GetOverlayMode() == AlsOverlayModeTags::Default) && ((Character->GetCombatStance() == ALSXTCombatStanceTags::Ready) || (Character->GetCombatStance() == ALSXTCombatStanceTags::Aiming)) && IAlsxtCombatInterface::Execute_CanAttack(GetOwner()))
	{
		Attack(ALSXTActionTypeTags::Primary, ALSXTAttackTypeTags::RightFist, ALSXTActionStrengthTags::Medium, 0.10f, 1.3f);
	}
}

bool UAlsxtCombatComponent::IsAttackAllowedToStart(const UAnimMontage* Montage) const
{
	return !Character->GetLocomotionAction().IsValid() ||
		// ReSharper disable once CppRedundantParentheses
		(Character->GetLocomotionAction() == AlsLocomotionActionTags::PrimaryAction &&
			!Character->GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage));
}


void UAlsxtCombatComponent::StartAttack(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage, const float PlayRate, const float TargetYawAngle)
{
}

void UAlsxtCombatComponent::StartSyncedAttack(const FGameplayTag& Overlay, const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float BaseDamage, const float PlayRate, const float TargetYawAngle, int Index)
{
}

void UAlsxtCombatComponent::DetermineAttackMethod_Implementation(FGameplayTag& AttackMethod, const FGameplayTag& ActionType, const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage, const AActor* Target)
{
	if (UKismetSystemLibrary::DoesImplementInterface(GetCombatState().CombatParameters.Target, UAlsxtCombatInterface::StaticClass()))
	{
		if (ActionType == ALSXTActionTypeTags::Secondary)
		{
			// if (IALSXTCombatInterface::Execute_CanBeTakenDown(GetCombatState().CombatParameters.Target) && CanPerformTakedown())
			if (IAlsxtCombatInterface::Execute_CanPerformTakedown(GetOwner()))
			{
				AttackMethod = ALSXTAttackMethodTags::TakeDown;
				return;
			}
			else if (IAlsxtCombatInterface::Execute_CanGrapple(GetOwner()))
			{
				if (IAlsxtCombatInterface::Execute_CanThrow(GetOwner()))
				{
					AttackMethod = ALSXTAttackMethodTags::Throw;
					return;
				}
				else
				{
					AttackMethod = ALSXTAttackMethodTags::Grapple;
					return;
				}
			}
			else
			{
				AttackMethod = ALSXTAttackMethodTags::Cancelled;
				return;
			}
		}
		else
		{
			if (LastTargets.Num() > 0)
			{
				for (auto& LastTarget : LastTargets)
				{
					if (LastTarget.Target == Target)
					{
						if (LastTarget.LastBlockedAttack < 2.0f)
						{
							AttackMethod = ALSXTAttackMethodTags::Riposte;
							return;
						}
						else if (LastTarget.ConsecutiveHits >= IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner())->ConsecutiveHitsForSpecialAttack)
						{
							if (IAlsxtCombatInterface::Execute_CanPerformUniqueAttack(GetOwner()))
							{
								AttackMethod = ALSXTAttackMethodTags::Unique;
								return;
							}
							else if (IAlsxtCombatInterface::Execute_CanPerformTakedown(GetOwner()))
							{
								AttackMethod = ALSXTAttackMethodTags::TakeDown;
								return;
							}
							else if (IAlsxtCombatInterface::Execute_CanGrapple(GetOwner()))
							{
								if (IAlsxtCombatInterface::Execute_CanThrow(GetOwner()))
								{
									AttackMethod = ALSXTAttackMethodTags::Throw;
									return;
								}
								else
								{
									AttackMethod = ALSXTAttackMethodTags::Grapple;
									return;
								}
							}
							else
							{
								AttackMethod = ALSXTAttackMethodTags::Regular;
								return;
							}
						}
						else
						{
							AttackMethod = ALSXTAttackMethodTags::Regular;
							return;
						}
					}
				}
			}
			else
			{
				AttackMethod = ALSXTAttackMethodTags::Regular;
				return;
			}
		}
	}
	else
	{
		AttackMethod = ALSXTAttackMethodTags::Regular;
		return;
	}
}

FAttackAnimation UAlsxtCombatComponent::SelectAttackMontage_Implementation(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage)
{
	FAttackAnimation SelectedAttackAnimation;
	UAlsxtCombatSettings* Settings = IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner());
	TArray<FAttackAnimation> Montages = Settings->AttackAnimations;
	TArray<FAttackAnimation> FilteredMontages;
	TArray<FGameplayTag> TagsArray = { AttackType, Stance, Strength };
	FGameplayTagContainer TagsContainer = FGameplayTagContainer::CreateFromArray(TagsArray);
	
	// Return is there are no Montages
	if (Montages.Num() < 1 || !Montages[0].Montage.Montage)
	{
		return SelectedAttackAnimation;
	}

	// Filter sounds based on Tag parameters
	for (auto Montage : Montages)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Montage.AttackStrengths);
		CurrentTagsContainer.AppendTags(Montage.AttackStances);
		CurrentTagsContainer.AppendTags(Montage.AttackType);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredMontages.Add(Montage);
		}
	}

	// Return if there are no filtered sounds
	if (FilteredMontages.Num() < 1 || !FilteredMontages[0].Montage.Montage)
	{
		return SelectedAttackAnimation;
	}

	// If more than one result, avoid duplicates
	if (FilteredMontages.Num() > 1)
	{
		// If FilteredMontages contains LastAttackAnimation, remove it from FilteredMontages array to avoid duplicates
		if (FilteredMontages.Contains(LastAttackAnimation))
		{
			int IndexToRemove = FilteredMontages.Find(LastAttackAnimation);
			FilteredMontages.RemoveAt(IndexToRemove, 1, EAllowShrinking::Yes);
		}

		//Shuffle Array
		for (int m = FilteredMontages.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredMontages.Swap(m, n);
		}
		
		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredMontages.Num() - 1));
		SelectedAttackAnimation = FilteredMontages[RandIndex];
		LastAttackAnimation = SelectedAttackAnimation;
		return SelectedAttackAnimation;
	}
	else
	{
		SelectedAttackAnimation = FilteredMontages[0];
		LastAttackAnimation = SelectedAttackAnimation;
		return SelectedAttackAnimation;
	}
	return SelectedAttackAnimation;
}

FSyncedAttackAnimation UAlsxtCombatComponent::SelectSyncedAttackMontage_Implementation(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage, int& Index)
{
	FSyncedAttackAnimation SelectedSyncedAttackAnimation;
	UAlsxtCombatSettings* Settings = IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner());
	TArray<FSyncedAttackAnimation> Montages = Settings->SyncedAttackAnimations;
	TArray<FSyncedAttackAnimation> FilteredMontages;
	TArray<FGameplayTag> TagsArray = { AttackType, Stance, Strength };
	FGameplayTagContainer TagsContainer = FGameplayTagContainer::CreateFromArray(TagsArray);

	// Return is there are no Montages
	if (Montages.Num() < 1 || !Montages[0].SyncedMontage.InstigatorSyncedMontage.Montage)
	{
		return SelectedSyncedAttackAnimation;
	}

	// Filter sounds based on Tag parameters
	for (auto Montage : Montages)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Montage.AttackStrength);
		CurrentTagsContainer.AppendTags(Montage.AttackStance);
		CurrentTagsContainer.AppendTags(Montage.AttackType);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredMontages.Add(Montage);
		}
	}

	// Return if there are no filtered sounds
	if (FilteredMontages.Num() < 1 || !FilteredMontages[0].SyncedMontage.InstigatorSyncedMontage.Montage)
	{
		return SelectedSyncedAttackAnimation;
	}

	// If more than one result, avoid duplicates
	if (FilteredMontages.Num() > 1)
	{
		// If FilteredMontages contains LastAttackAnimation, remove it from FilteredMontages array to avoid duplicates
		if (FilteredMontages.Contains(LastSyncedAttackAnimation))
		{
			int IndexToRemove = FilteredMontages.Find(LastSyncedAttackAnimation);
			FilteredMontages.RemoveAt(IndexToRemove, 1, EAllowShrinking::Yes);
		}

		//Shuffle Array
		for (int m = FilteredMontages.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredMontages.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredMontages.Num() - 1));
		SelectedSyncedAttackAnimation = FilteredMontages[RandIndex];
		Index = Montages.Find(SelectedSyncedAttackAnimation);
		LastSyncedAttackAnimation = SelectedSyncedAttackAnimation;
		return SelectedSyncedAttackAnimation;
	}
	else
	{
		SelectedSyncedAttackAnimation = FilteredMontages[0];
		Index = Montages.Find(SelectedSyncedAttackAnimation);
		LastSyncedAttackAnimation = SelectedSyncedAttackAnimation;
		return SelectedSyncedAttackAnimation;
	}
	return SelectedSyncedAttackAnimation;
}

FAnticipationPose UAlsxtCombatComponent::SelectBlockingkMontage_Implementation(const FGameplayTag& Strength, const FGameplayTag& Side, const FGameplayTag& Form, const FGameplayTag& Health)
{
	FAnticipationPose SelectedAnticipationPose;
	return SelectedAnticipationPose;
}

FSyncedActionAnimation UAlsxtCombatComponent::GetSyncedAttackMontage_Implementation(int32 Index)
{
	UAlsxtCombatSettings* Settings = IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner());
	TArray<FSyncedAttackAnimation> Montages = Settings->SyncedAttackAnimations;
	return Montages[Index].SyncedMontage;
}

void UAlsxtCombatComponent::ServerStartAttack_Implementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	if (IsAttackAllowedToStart(Montage))
	{
		MulticastStartAttack(Montage, PlayRate, StartYawAngle, TargetYawAngle);
		Character->ForceNetUpdate();
	}
}

void UAlsxtCombatComponent::MulticastStartAttack_Implementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	StartAttackImplementation(Montage, PlayRate, StartYawAngle, TargetYawAngle);
}

void UAlsxtCombatComponent::StartAttackImplementation(UAnimMontage* Montage, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
}

void UAlsxtCombatComponent::RefreshAttack(const float DeltaTime)
{
	StopAttack();
}

void UAlsxtCombatComponent::RefreshAttackPhysics(const float DeltaTime)
{
	// float Offset = CombatSettings->Combat.RotationOffset;
	auto ComponentRotation{ Character->GetCharacterMovement()->UpdatedComponent->GetComponentRotation() };
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	auto TargetRotation{ PlayerController->GetControlRotation() };
	// TargetRotation.Yaw = TargetRotation.Yaw + Offset;
	// TargetRotation.Yaw = TargetRotation.Yaw;
	// TargetRotation.Pitch = ComponentRotation.Pitch;
	// TargetRotation.Roll = ComponentRotation.Roll;

	// if (CombatSettings.RotationInterpolationSpeed <= 0.0f)
	// {
	// 	TargetRotation.Yaw = CombatState.TargetYawAngle;
	// 
	// 	Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	// }
	// else
	// {
	// 	TargetRotation.Yaw = UAlsMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(TargetRotation.Yaw)),
	// 		CombatState.TargetYawAngle, DeltaTime,
	// 		CombatSettings->Combat.RotationInterpolationSpeed);
	// 
	// 	Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false);
	// }
}

void UAlsxtCombatComponent::StopAttack()
{
	IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
	OnAttackEndedDelegate.Broadcast();
}

void UAlsxtCombatComponent::ServerStartSyncedAttack_Implementation(UAnimMontage* Montage, int32 Index, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	if (IsAttackAllowedToStart(Montage))
	{
		MulticastStartSyncedAttack(Montage, Index, PlayRate, StartYawAngle, TargetYawAngle);
		GetOwner()->ForceNetUpdate();
	}
}

void UAlsxtCombatComponent::MulticastStartSyncedAttack_Implementation(UAnimMontage* Montage, int32 Index, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
	StartSyncedAttackImplementation(Montage, Index, PlayRate, StartYawAngle, TargetYawAngle);
}

void UAlsxtCombatComponent::StartSyncedAttackImplementation(UAnimMontage* Montage, int32 Index, const float PlayRate,
	const float StartYawAngle, const float TargetYawAngle)
{
}

void UAlsxtCombatComponent::RefreshSyncedAttack(const float DeltaTime)
{
	StopSyncedAttack();
}

void UAlsxtCombatComponent::RefreshSyncedAttackPhysics(const float DeltaTime)
{
	// float Offset = CombatSettings->Combat.RotationOffset;
	auto ComponentRotation{ IAlsxtCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->UpdatedComponent->GetComponentRotation() };
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	auto TargetRotation{ PlayerController->GetControlRotation() };
	// TargetRotation.Yaw = TargetRotation.Yaw + Offset;
	// TargetRotation.Yaw = TargetRotation.Yaw;
	// TargetRotation.Pitch = ComponentRotation.Pitch;
	// TargetRotation.Roll = ComponentRotation.Roll;

	// if (CombatSettings.RotationInterpolationSpeed <= 0.0f)
	// {
	// 	TargetRotation.Yaw = CombatState.TargetYawAngle;
	// 
	// 	Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	// }
	// else
	// {
	// 	TargetRotation.Yaw = UAlsMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(TargetRotation.Yaw)),
	// 		CombatState.TargetYawAngle, DeltaTime,
	// 		CombatSettings->Combat.RotationInterpolationSpeed);
	// 
	// 	Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false);
	// }
}

void UAlsxtCombatComponent::StopSyncedAttack()
{
	IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
	OnSyncedAttackEnded();
}

void UAlsxtCombatComponent::OnSyncedAttackEnded_Implementation() {}
