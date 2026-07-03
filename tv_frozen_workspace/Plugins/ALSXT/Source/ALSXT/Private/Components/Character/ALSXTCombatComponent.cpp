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
	FGameplayTagContainer TargetableOverlayModes = IAlsxtCombatInterface::Execute_GetTargetableOverlayModes(GetOwner());

	if (Character && TargetableOverlayModes.HasTag(IAlsxtCharacterInterface::Execute_GetCharacterOverlayMode(GetOwner())) && Character->IsDesiredAiming() && IsValid(CurrentTarget.HitResult.GetActor()))
	{
		if (Character->GetDistanceTo(CurrentTarget.HitResult.GetActor()) < CombatSettings.MaxInitialLockDistance)
		{			
			if (CombatSettings.UnlockWhenTargetIsObstructed && !CombatSettings.ObstructionTraceObjectTypes.IsEmpty()) {
				if (CombatSettings.UnlockWhenTargetIsObstructed && !IsTartgetObstructed()) {
					if (CurrentTarget.Valid)
					{
						RotatePlayerToTarget(CurrentTarget);
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Obstructed");
					DisengageAllTargets();
					OnTargetObstructed();
				}
			}
			else
			{
				if (CurrentTarget.Valid)
				{
					RotatePlayerToTarget(CurrentTarget);
				}
			}			
		}
		else
		{
			DisengageAllTargets();
		}
	}
}

void UAlsxtCombatComponent::TraceForTargets(TArray<FTargetHitResultEntry>& Targets)
{
	FRotator ControlRotation = Cast<ACharacter>(GetOwner())->GetControlRotation();
	FVector CharLoc = GetOwner()->GetActorLocation();
	FVector ForwardVector = GetOwner()->GetActorForwardVector();
	FVector CameraLocation = IAlsxtCharacterInterface::Execute_GetCharacterCamera(GetOwner())->GetFirstPersonCameraLocation();
	FVector StartLocation = ForwardVector * 150 + CameraLocation;
	FVector EndLocation = ForwardVector * 200 + StartLocation;
	FVector CenterLocation = (StartLocation - EndLocation) / 8 + StartLocation;
	FCollisionShape CollisionShape = FCollisionShape::MakeBox(CombatSettings.TraceAreaHalfSize);
	TArray<FHitResult> OutHits;

	// Display Debug Shape
	if (CombatSettings.DebugMode)
	{
		DrawDebugBox(GetOwner()->GetWorld(), CenterLocation, CombatSettings.TraceAreaHalfSize, ControlRotation.Quaternion(), FColor::Yellow, false, CombatSettings.DebugDuration, 100, 2);
	}

	FCollisionObjectQueryParams ObjectQueryParameters;
	for (const auto ObjectType : CombatSettings.TargetTraceObjectTypes)
	{
		ObjectQueryParameters.AddObjectTypesToQuery(UCollisionProfile::Get()->ConvertToCollisionChannel(false, ObjectType));
	}

	bool isHit = GetOwner()->GetWorld()->SweepMultiByObjectType(OutHits, StartLocation, EndLocation, ControlRotation.Quaternion(), ObjectQueryParameters, CollisionShape);

	if (isHit)
	{
		for (auto& Hit : OutHits)
		{
			if (Hit.GetActor()->GetClass()->ImplementsInterface(UAlsxtTargetLockInterface::StaticClass()) && Hit.GetActor() != GetOwner() && GetOwner()->GetDistanceTo(Hit.GetActor()) < CombatSettings.MaxLockDistance)
			{
				FTargetHitResultEntry HitResultEntry;
				HitResultEntry.Valid = true;
				HitResultEntry.DistanceFromPlayer = FVector::Distance(GetOwner()->GetActorLocation(), Hit.Location);
				HitResultEntry.AngleFromCenter = GetAngle(Hit.Location);
				HitResultEntry.HitResult = Hit;
				Targets.Add(HitResultEntry);
			}
		}
	}
}

void UAlsxtCombatComponent::GetClosestTarget()
{
	TArray<FTargetHitResultEntry> OutHits;
	TraceForTargets(OutHits);
	FTargetHitResultEntry FoundHit;
	AAlsxtCharacter* ALSXTChar = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
	FGameplayTagContainer TargetableOverlayModes = IAlsxtCombatInterface::Execute_GetTargetableOverlayModes(GetOwner());

	if (TargetableOverlayModes.HasTag(ALSXTChar->GetOverlayMode()) && ALSXTChar->IsDesiredAiming())
	{
		for (auto& Hit : OutHits)
		{
			if (Hit.HitResult.GetActor() != GetOwner())
			{
				FTargetHitResultEntry HitResultEntry;
				HitResultEntry.Valid = true;
				HitResultEntry.DistanceFromPlayer = Hit.DistanceFromPlayer;
				HitResultEntry.AngleFromCenter = Hit.AngleFromCenter;
				HitResultEntry.HitResult = Hit.HitResult;
				
				if (Hit.HitResult.GetActor() != CurrentTarget.HitResult.GetActor() && IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner())->TargetableCharacterStatuses.HasTag(IAlsxtCharacterInterface::Execute_GetCharacterStatus(Hit.HitResult.GetActor())) && (HitResultEntry.DistanceFromPlayer < CurrentTarget.DistanceFromPlayer))
				{
					if (!FoundHit.Valid)
					{
						FoundHit = Hit;
					}
					else
					{
						if (Hit.AngleFromCenter < FoundHit.AngleFromCenter)
						{
							FoundHit = Hit;
						}
					}
					GetWorld()->GetTimerManager().SetTimer(TargetTraceTimerHandle, TargetTraceTimerDelegate, 0.001f, true);
				}
			}
		}
		if (FoundHit.Valid && FoundHit.HitResult.GetActor())
		{
			SetCurrentTarget(FoundHit);
			if (GEngine && CombatSettings.DebugMode)
			{
				FString DebugMsg = FString::SanitizeFloat(FoundHit.AngleFromCenter);
				DebugMsg.Append(" Hit Result: ");
				DebugMsg.Append(FoundHit.HitResult.GetActor()->GetName());
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, DebugMsg);
			}
		}
	}
}

void UAlsxtCombatComponent::SetCurrentTarget(const FTargetHitResultEntry& NewTarget)
{
	ClearCurrentTarget();
	CurrentTarget = NewTarget;
	IAlsxtCombatInterface::Execute_OnNewTarget(GetOwner(), NewTarget);
	USkeletalMeshComponent* HitMesh;
	AAlsxtCharacter* ALSXTChar = IAlsxtCharacterInterface::Execute_GetCharacter(CurrentTarget.HitResult.GetActor());
	

	if (ALSXTChar)
	{
		HitMesh = ALSXTChar->GetMesh();
		TArray<UMaterialInterface*> CharMaterials = HitMesh->GetMaterials();	

		if (CharMaterials[0])
		{
			for (int m = 0; m < CharMaterials.Num(); m++)
			{
				UMaterialInstanceDynamic* CharDynMaterial = HitMesh->CreateAndSetMaterialInstanceDynamic(m);
				CharDynMaterial->SetScalarParameterValue(CombatSettings.HighlightMaterialParameterName, 1.0f);
				HitMesh->SetMaterial(m, CharDynMaterial);
				TargetDynamicMaterials.Add(CharDynMaterial);
			}
		}
		UMaterialInterface* HeadMaterial = ALSXTChar->Head->GetMaterial(0);
		UMaterialInstanceDynamic* HeadDynMaterial = ALSXTChar->Head->CreateAndSetMaterialInstanceDynamic(0);
		HeadDynMaterial->SetScalarParameterValue(CombatSettings.HighlightMaterialParameterName, 1.0f);
		ALSXTChar->Head->SetMaterial(0, HeadDynMaterial);
		TargetDynamicMaterials.Add(HeadDynMaterial);
	}	
}

void UAlsxtCombatComponent::ClearCurrentTarget()
{
	if (CurrentTarget.Valid == true && CurrentTarget.HitResult.GetActor())
	{
		CurrentTarget.Valid = false;
		CurrentTarget.DistanceFromPlayer = 340282346638528859811704183484516925440.0f;
		CurrentTarget.AngleFromCenter = 361.0f;

		if (Cast<AAlsxtCharacter>(CurrentTarget.HitResult.GetActor()))
		{
			USkeletalMeshComponent* HitMesh = Cast<AAlsxtCharacter>(CurrentTarget.HitResult.GetActor())->GetMesh();
			TArray<UMaterialInterface*> CharMaterials = HitMesh->GetMaterials();
			if (TargetDynamicMaterials[0])
			{
				for (int m = 0; m < TargetDynamicMaterials.Num(); m++)
				{
					TargetDynamicMaterials[m]->SetScalarParameterValue(CombatSettings.HighlightMaterialParameterName, 0.0f);
					// HitMesh->SetMaterial(m, CharDynMaterial);
				}
				TargetDynamicMaterials.Empty();
			}
		}
		CurrentTarget.HitResult = FHitResult(ForceInit);
	}
}

void UAlsxtCombatComponent::DisengageAllTargets()
{
	ClearCurrentTarget();

	// Clear Trace Timer
	GetWorld()->GetTimerManager().ClearTimer(TargetTraceTimerHandle);
}

void UAlsxtCombatComponent::GetTargetLeft()
{
	TArray<FTargetHitResultEntry> OutHits;
	TraceForTargets(OutHits);
	FTargetHitResultEntry FoundHit;
	AAlsxtCharacter* ALSXTChar = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
	FGameplayTagContainer TargetableOverlayModes = IAlsxtCombatInterface::Execute_GetTargetableOverlayModes(GetOwner());
	// 
	// // IALSXTCharacterInterface::GetCharacter(this)->IsDesiredAiming();
	// IALSXTCharacterInterface::Execute_GetCharacterOverlayMode(GetOwner())
	// 
	if (TargetableOverlayModes.HasTag(ALSXTChar->GetOverlayMode()) && ALSXTChar->IsDesiredAiming())
	{
		if (OutHits.Num() == 0 || OutHits.Num() == 1 && OutHits[0].HitResult.GetActor() == CurrentTarget.HitResult.GetActor())
		{
			return;
		}
		
		for (auto& Hit : OutHits)
		{
			if (Hit.HitResult.GetActor() != CurrentTarget.HitResult.GetActor() && Hit.HitResult.GetActor() != GetOwner() && (Hit.AngleFromCenter < CurrentTarget.AngleFromCenter))
			{
				
				if (!FoundHit.Valid)
				{
					FoundHit = Hit;
				}
				else
				{
					if (Hit.AngleFromCenter > FoundHit.AngleFromCenter)
					{
						FoundHit = Hit;
					}
				}
			}
		}
		if (FoundHit.Valid && FoundHit.HitResult.GetActor())
		{
			SetCurrentTarget(FoundHit);
		}
	}
}

void UAlsxtCombatComponent::GetTargetRight()
{
	TArray<FTargetHitResultEntry> OutHits;
	TraceForTargets(OutHits);
	FTargetHitResultEntry FoundHit;
	AAlsxtCharacter* ALSXTChar = IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner());
	FGameplayTagContainer TargetableOverlayModes = IAlsxtCombatInterface::Execute_GetTargetableOverlayModes(GetOwner());
	
	if (TargetableOverlayModes.HasTag(ALSXTChar->GetOverlayMode()) && ALSXTChar->IsDesiredAiming())
	{
		if (OutHits.Num() == 0 || OutHits.Num() == 1 && OutHits[0].HitResult.GetActor() == CurrentTarget.HitResult.GetActor())
		{
			return;
		}
		
		for (auto& Hit : OutHits)
		{
			if (Hit.HitResult.GetActor() != CurrentTarget.HitResult.GetActor() && Hit.HitResult.GetActor() != GetOwner() && (Hit.AngleFromCenter > CurrentTarget.AngleFromCenter))
			{
				if (!FoundHit.Valid)
				{
					FoundHit = Hit;
				}
				else
				{
					if (Hit.AngleFromCenter < FoundHit.AngleFromCenter)
					{
						FoundHit = Hit;
					}
				}
			}
		}
		if (FoundHit.Valid && FoundHit.HitResult.GetActor())
		{
			SetCurrentTarget(FoundHit);
		}
	}
}

void UAlsxtCombatComponent::RotatePlayerToTarget(FTargetHitResultEntry Target)
{
	if (IsValid(Character) && IsValid(Character->GetController()) && Target.Valid && IsValid(Target.HitResult.GetActor()))
	{
		FRotator CurrentPlayerRotation = Character->GetActorRotation();
		FRotator CurrentPlayerControlRotation = Character->GetController()->GetControlRotation();
		FVector TargetActorLocation = Target.HitResult.GetActor()->GetActorLocation();
		FRotator NewPlayerRotation = UKismetMathLibrary::FindLookAtRotation(Character->GetActorLocation(), TargetActorLocation);
		NewPlayerRotation.Pitch = CurrentPlayerRotation.Pitch;
		NewPlayerRotation.Roll = CurrentPlayerRotation.Roll;

		if ((NewPlayerRotation != CurrentPlayerControlRotation) && IsValid(Target.HitResult.GetActor()))
		{
			Character->GetController()->SetControlRotation(NewPlayerRotation);
		}
	}
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
	TArray<FHitResult> OutHits;
	FVector SweepStart = GetOwner()->GetActorLocation();
	FVector SweepEnd = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * Distance;
	FCollisionShape MyColSphere = FCollisionShape::MakeSphere(50.0f);
	TArray<AActor*> InitialIgnoredActors;
	TArray<AActor*> OriginTraceIgnoredActors;
	InitialIgnoredActors.Add(Character);	// Add Self to Initial Trace Ignored Actors
	TArray<TEnumAsByte<EObjectTypeQuery>> AttackTraceObjectTypes;
	AttackTraceObjectTypes = CombatSettings.AttackTraceObjectTypes;
	EDrawDebugTrace::Type ShowDebugTrace {EDrawDebugTrace::None};	
	CombatSettings.DebugMode ? ShowDebugTrace = EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	bool isHit = UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), SweepStart, SweepEnd, 50, CombatSettings.AttackTraceObjectTypes, false, InitialIgnoredActors, ShowDebugTrace, OutHits, true, FLinearColor::White, FLinearColor::Blue, 0.0f);

	if (isHit)
	{
		// loop through TArray
		for (auto& Hit : OutHits)
		{
			if (Hit.GetActor() != Character && UKismetSystemLibrary::DoesImplementInterface(Hit.GetActor(), UAlsxtCombatInterface::StaticClass()))
			{
				if (GEngine && CombatSettings.DebugMode)
				{
					// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Potential Target: %s"), *Hit.GetActor()->GetName()));
				}
				return Hit.GetActor();
			}
		}
	}
	return nullptr;
}

void UAlsxtCombatComponent::BeginMoveToTarget()
{

}

void UAlsxtCombatComponent::UpdateMoveToTarget()
{
	Character->GetMovementComponent()->Velocity = FMath::VInterpTo(GetOwner()->GetVelocity(), CurrentTarget.HitResult.Location, GetOwner()->GetWorld()->DeltaTimeSeconds, 1.0f);	
}

void UAlsxtCombatComponent::EndMoveToTarget()
{

}

void UAlsxtCombatComponent::BeginAttackCollisionTrace(FAlsxtCombatAttackTraceSettings TraceSettings)
{
	if (TraceSettings.UnarmedAttackType == FGameplayTag::EmptyTag)
	{
		TraceSettings.UnarmedAttackType = ALSXTUnarmedAttackTypeTags::RightFist;
	}

	CurrentAttackTraceSettings = TraceSettings;

	GetWorld()->GetTimerManager().SetTimer(AttackTraceTimerHandle, AttackTraceTimerDelegate, 0.001f, true);
}

void UAlsxtCombatComponent::AttackCollisionTrace()
{
	if (!UKismetSystemLibrary::DoesImplementInterface(GetOwner(), UAlsxtCombatInterface::StaticClass()))
	{
		return;
	}

	// Setup Initial Trace
	IAlsxtCombatInterface::Execute_GetCombatUnarmedTraceLocations(GetOwner(), CurrentAttackTraceSettings.AttackType, CurrentAttackTraceSettings.Start, CurrentAttackTraceSettings.End, CurrentAttackTraceSettings.Radius);
	TArray<TEnumAsByte<EObjectTypeQuery>> AttackTraceObjectTypes = CombatSettings.AttackTraceObjectTypes;
	
	// TArray<AActor*> Landscape;
	// UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALandscape::StaticClass(), Landscape);

	TArray<AActor*> InitialIgnoredActors;
	TArray<AActor*> OriginTraceIgnoredActors;
	TArray<FHitResult> HitResults;
	// InitialIgnoredActors.Append(Landscape);
	InitialIgnoredActors.Add(GetOwner());	// Add Self to Initial Trace Ignored Actors
	EDrawDebugTrace::Type ShowDebugTrace {EDrawDebugTrace::None};
	CombatSettings.DebugMode ? ShowDebugTrace = EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	// Initial Trace
	bool isHit = UKismetSystemLibrary::SphereTraceMultiForObjects(GetOwner()->GetWorld(), CurrentAttackTraceSettings.Start, CurrentAttackTraceSettings.End, CurrentAttackTraceSettings.Radius, AttackTraceObjectTypes, false, InitialIgnoredActors, ShowDebugTrace, HitResults, true, FLinearColor::Green, FLinearColor::Yellow, 0.0f);
	if (isHit)
	{
		// Loop through HitResults Array
		for (auto& HitResult : HitResults)
		{
			// Check if not in AttackedActors Array
			if (!AttackTraceLastHitActors.Contains(HitResult.GetActor()))
			{
				AttackTraceLastHitActors.AddUnique(HitResult.GetActor());
				// Declare Local Vars
				FAttackDoubleHitResult CurrentHitResult;
				FGameplayTag ImpactLoc;
				FGameplayTag ImpactStrength;
				// FGameplayTag ImpactSide;
				FGameplayTag ImpactForm = GetCombatState().CombatParameters.Form;
				AActor* HitActor = HitResult.GetActor();
				FString HitActorname;
				FVector HitActorVelocity{ FVector::ZeroVector };
				float HitActorMass{ 0.0f };
				float HitActorAttackVelocity{ 0.0f };
				float HitActorAttackMass{ 0.0f };

				// Populate Hit
				CurrentHitResult.DoubleHitResult.HitResult.HitResult = HitResult;
				CurrentHitResult.Type = CurrentAttackTraceSettings.AttackType;
				CurrentHitResult.DoubleHitResult.ImpactForm = CurrentAttackTraceSettings.ImpactForm;
				CurrentHitResult.DoubleHitResult.HitResult.ImpactForm = CurrentAttackTraceSettings.ImpactForm;
				CurrentHitResult.Strength = CurrentAttackTraceSettings.AttackStrength;
				CurrentHitResult.DoubleHitResult.HitResult.ImpactStrength = CurrentAttackTraceSettings.AttackStrength;
				HitActor = CurrentHitResult.DoubleHitResult.HitResult.HitResult.GetActor();
				HitActorname = HitActor->GetName();
				
				
				// Physics
				if (UKismetSystemLibrary::DoesImplementInterface(HitActor, UAlsxtCharacterInterface::StaticClass()))
				{
					IAlsxtCharacterInterface::Execute_GetCombatAttackPhysics(HitActor, HitActorAttackMass, HitActorAttackVelocity);
				}
				else
				{
					if (UKismetSystemLibrary::DoesImplementInterface(HitActor, UAlsxtCollisionInterface::StaticClass()))
					{
						IAlsxtCollisionInterface::Execute_GetActorVelocity(HitActor, HitActorVelocity);
						IAlsxtCollisionInterface::Execute_GetActorMass(HitActor, HitActorMass);
					}
				}
				// TotalImpactEnergy = 50.0f + (HitActorVelocity * HitActorMass) + (HitActorAttackVelocity * HitActorAttackMass);
				FVector TotalImpactEnergy = (HitActorVelocity * HitActorMass) + (HitActorAttackVelocity * HitActorAttackMass);
				FVector HitDirection = HitResult.ImpactPoint - GetOwner()->GetActorLocation();
				HitDirection.Normalize();
				CurrentHitResult.DoubleHitResult.HitResult.Direction = HitDirection;
				CurrentHitResult.DoubleHitResult.HitResult.Impulse = HitResult.Normal * TotalImpactEnergy;

				// Impact Location
				if (UKismetSystemLibrary::DoesImplementInterface(HitActor, UAlsxtCollisionInterface::StaticClass()))
				{
					IAlsxtCollisionInterface::Execute_GetLocationFromBoneName(HitActor, CurrentHitResult.DoubleHitResult.HitResult.HitResult.BoneName, ImpactLoc);
				}
				CurrentHitResult.DoubleHitResult.HitResult.ImpactLocation = ImpactLoc;
				// CurrentHitResult.DoubleHitResult.HitResult.ImpactSide = ImpactSide;

				// Setup Origin Trace
				FHitResult OriginHitResult;
				OriginTraceIgnoredActors.Add(HitResult.GetActor());	// Add Hit Actor to Origin Trace Ignored Actors
				// OriginTraceIgnoredActors.Append(Landscape);

				// Perform Origin Hit Trace to get PhysMat etc for ImpactLocation
				bool isOriginHit {false};
				isOriginHit = UKismetSystemLibrary::SphereTraceSingleForObjects(GetOwner()->GetWorld(), HitResult.Location, HitResult.TraceStart, CurrentAttackTraceSettings.Radius, AttackTraceObjectTypes, false, OriginTraceIgnoredActors, EDrawDebugTrace::None, OriginHitResult, true, FLinearColor::Green, FLinearColor::Yellow, 4.0f);
				if (isOriginHit)
				{
					// Populate Origin Hit
					CurrentHitResult.DoubleHitResult.OriginHitResult.HitResult = OriginHitResult;
					UAlsxtBlueprintFunctionLibrary::GetSideFromHit(CurrentHitResult.DoubleHitResult, CurrentHitResult.DoubleHitResult.HitResult.ImpactSide);
					UAlsxtBlueprintFunctionLibrary::GetStrengthFromHit(CurrentHitResult.DoubleHitResult, CurrentHitResult.Strength);
					CurrentHitResult.DoubleHitResult.HitResult.ImpactStrength = CurrentAttackTraceSettings.AttackStrength;
					FString OriginHitActorname = OriginHitResult.GetActor()->GetName();

					// Populate Values based if Holding Item
					if (IAlsxtHeldItemInterface::Execute_IsHoldingItem(GetOwner()))
					{
						IAlsxtCombatInterface::Execute_GetCombatHeldItemAttackDamageInfo(GetOwner(), CurrentHitResult.Type, CurrentHitResult.Strength, CurrentHitResult.BaseDamage, CurrentHitResult.DoubleHitResult.HitResult.ImpactForm, CurrentHitResult.DoubleHitResult.HitResult.DamageType);
					}
					else
					{
						IAlsxtCombatInterface::Execute_GetCombatUnarmedAttackDamageInfo(GetOwner(), CurrentHitResult.Type, CurrentHitResult.Strength, CurrentHitResult.BaseDamage, CurrentHitResult.DoubleHitResult.HitResult.ImpactForm, CurrentHitResult.DoubleHitResult.HitResult.DamageType);
					}
					// GetFormFromHit(CurrentHitResult.DoubleHitResult, CurrentHitResult.DoubleHitResult.ImpactForm);
					// CurrentHitResult.DoubleHitResult.ImpactForm = 		
				}

				if (UKismetSystemLibrary::DoesImplementInterface(HitActor, UAlsxtCollisionInterface::StaticClass()))
				{
					IAlsxtCollisionInterface::Execute_OnActorAttackCollision(HitActor, CurrentHitResult);
				}
				else
				{
					FGameplayTagContainer UnarmedAttackTypesLansdcapeIgnore;
					UnarmedAttackTypesLansdcapeIgnore.AddTag(ALSXTUnarmedAttackTypeTags::LeftFoot);
					UnarmedAttackTypesLansdcapeIgnore.AddTag(ALSXTUnarmedAttackTypeTags::RightFoot);
					if (!HitActor->IsA(ALandscape::StaticClass()) && (!CurrentAttackTraceSettings.UnarmedAttackType.MatchesAny(UnarmedAttackTypesLansdcapeIgnore)))
					{
						IAlsxtCollisionInterface::Execute_OnStaticMeshAttackCollision(GetOwner(), CurrentHitResult);
					}					
				}

				// Play Effects for Attacker 
				IAlsxtCombatInterface::Execute_OnAttackCollision(GetOwner(), CurrentHitResult);
			}
		}
	}
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
	if (Character->GetLocomotionAction() == AlsLocomotionActionTags::PrimaryAction)
	{
		return;
	}
	
	FGameplayTag NewStance = ALSXTActionStanceTags::Standing;

	if (IAlsxtCharacterInterface::Execute_GetCharacterLocomotionMode(GetOwner()) == AlsLocomotionModeTags::InAir)
	{
		NewStance = ALSXTActionStanceTags::InAir;
	}
	else
	{
		if (Character->GetDesiredStance() == AlsStanceTags::Standing)
		{
			NewStance = ALSXTActionStanceTags::Standing;
		}
		else
		{
			NewStance = ALSXTActionStanceTags::Crouched;
		}
	}

	AActor* PotentialAttackTarget = TraceForPotentialAttackTarget(100.0f);
	FAlsxtCombatState CurrentCombatState = GetCombatState();
	CurrentCombatState.CombatParameters.Target = PotentialAttackTarget;
	CurrentCombatState.CombatParameters.Form = ALSXTImpactFormTags::Blunt;
	CurrentCombatState.CombatParameters.AttackType = AttackType;
	CurrentCombatState.CombatParameters.Strength = Strength;
	CurrentCombatState.CombatParameters.BaseDamage = BaseDamage;
	CurrentCombatState.CombatParameters.PlayRate = PlayRate;
	FGameplayTag AttackMethod;
	if (IsValid(PotentialAttackTarget))
	{
		DetermineAttackMethod(AttackMethod, ActionType, AttackType, NewStance, Strength, BaseDamage, PotentialAttackTarget);
	}
	else
	{
		AttackMethod = ALSXTAttackMethodTags::Regular;
	}

	if (AttackMethod == ALSXTAttackMethodTags::Regular || AttackMethod == ALSXTAttackMethodTags::Riposte)
	{
		StartAttack(AttackType, NewStance, Strength, BaseDamage, PlayRate, CombatSettings.bRotateToInputOnStart && Character->GetLocomotionState().bHasInput
			? Character->GetLocomotionState().InputYawAngle
			: UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)));
	}
	else if (AttackMethod == ALSXTAttackMethodTags::Unique || AttackMethod == ALSXTAttackMethodTags::TakeDown)
	{
		StartSyncedAttack(Character->GetOverlayMode(), AttackType, NewStance, Strength, AttackMethod, BaseDamage, PlayRate, CombatSettings.bRotateToInputOnStart && Character->GetLocomotionState().bHasInput
			? Character->GetLocomotionState().InputYawAngle
			: UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)), 0);
	}
	CurrentCombatState.CombatParameters.AttackMethod = AttackMethod;
	SetCombatState(CurrentCombatState);
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
	if (Character->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	FAttackAnimation Montage{ SelectAttackMontage(AttackType, Stance, Strength, BaseDamage) };

	if (!IsValid(Montage.Montage.Montage) || !IsAttackAllowedToStart(Montage.Montage.Montage))
	{
		return;
	}

	const auto StartYawAngle{ UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)) };

	// Clear the character movement mode and set the locomotion action to mantling.

	Character->SetMovementModeLocked(true);
	// Character->GetCharacterMovement()->SetMovementMode(MOVE_Custom);

	if (Character->GetLocalRole() >= ROLE_Authority)
	{
		// Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
		// Character->GetMesh()->SetRelativeLocationAndRotation(BaseTranslationOffset, BaseRotationOffset);
		CombatParameters.BaseDamage = BaseDamage;
		CombatParameters.PlayRate = PlayRate;
		CombatParameters.TargetYawAngle = TargetYawAngle;
		CombatParameters.AttackType = AttackType;
		CombatParameters.Stance = Stance;
		CombatParameters.Strength = Strength;
		CombatParameters.CombatAnimation = Montage.Montage;

		FAlsxtCombatState NewCombatState;
		NewCombatState.CombatParameters = CombatParameters;
		SetCombatState(NewCombatState);
		MulticastStartAttack(Montage.Montage.Montage, PlayRate, StartYawAngle, TargetYawAngle);
	}
	else
	{
		IAlsxtCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->FlushServerMoves();

		CombatParameters.BaseDamage = BaseDamage;
		CombatParameters.PlayRate = PlayRate;
		CombatParameters.TargetYawAngle = TargetYawAngle;
		CombatParameters.AttackType = AttackType;
		CombatParameters.Stance = Stance;
		CombatParameters.Strength = Strength;
		CombatParameters.CombatAnimation = Montage.Montage;

		FAlsxtCombatState NewCombatState;
		NewCombatState.CombatParameters = CombatParameters;
		SetCombatState(NewCombatState);

		ServerStartAttack(Montage.Montage.Montage, PlayRate, StartYawAngle, TargetYawAngle);
		// OnAttackStarted(AttackType, Stance, Strength, BaseDamage);
		OnAttackStartedDelegate.Broadcast(AttackType, Stance, Strength, BaseDamage);
	}
}

void UAlsxtCombatComponent::StartSyncedAttack(const FGameplayTag& Overlay, const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float BaseDamage, const float PlayRate, const float TargetYawAngle, int Index)
{
	if (Character->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	int SelectSyncedAttackMontageIndex{ -1 };
	FSyncedAttackAnimation Montage{ SelectSyncedAttackMontage(AttackType, Stance, Strength, BaseDamage, SelectSyncedAttackMontageIndex) };

	if (!ALS_ENSURE(IsValid(Montage.SyncedMontage.InstigatorSyncedMontage.Montage)) || !IsAttackAllowedToStart(Montage.SyncedMontage.InstigatorSyncedMontage.Montage))
	{
		return;
	}

	// GetSyncedAttackMontage();

	const auto StartYawAngle{ UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)) };

	// Clear the character movement mode and set the locomotion action to mantling.

	Character->SetMovementModeLocked(true);
	// Character->GetCharacterMovement()->SetMovementMode(MOVE_Custom);

	if (Character->GetLocalRole() >= ROLE_Authority)
	{
		// Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
		// Character->GetMesh()->SetRelativeLocationAndRotation(BaseTranslationOffset, BaseRotationOffset);
		CombatParameters.BaseDamage = BaseDamage;
		CombatParameters.PlayRate = PlayRate;
		CombatParameters.TargetYawAngle = TargetYawAngle;
		CombatParameters.AttackType = AttackType;
		CombatParameters.Stance = Stance;
		CombatParameters.Strength = Strength;
		CombatParameters.CombatAnimation = Montage.SyncedMontage.InstigatorSyncedMontage;

		FAlsxtCombatState NewCombatState;
		NewCombatState.CombatParameters = CombatParameters;
		SetCombatState(NewCombatState);
		MulticastStartSyncedAttack(Montage.SyncedMontage.InstigatorSyncedMontage.Montage, SelectSyncedAttackMontageIndex, PlayRate, StartYawAngle, TargetYawAngle);
	}
	else
	{
		IAlsxtCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->FlushServerMoves();

		StartSyncedAttackImplementation(Montage.SyncedMontage.InstigatorSyncedMontage.Montage, SelectSyncedAttackMontageIndex, PlayRate, StartYawAngle, TargetYawAngle);
		ServerStartSyncedAttack(Montage.SyncedMontage.InstigatorSyncedMontage.Montage, SelectSyncedAttackMontageIndex, PlayRate, StartYawAngle, TargetYawAngle);
		OnSyncedAttackStarted(AttackType, Stance, Strength, BaseDamage);
	}
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
	
	if (IsAttackAllowedToStart(Montage) && Character->GetMesh()->GetAnimInstance()->Montage_Play(Montage, PlayRate))
	{
		CombatState.CombatParameters.TargetYawAngle = TargetYawAngle;
		Character->ALSXTRefreshRotationInstant(StartYawAngle, ETeleportType::None);
		Character->SetLocomotionAction(AlsLocomotionActionTags::PrimaryAction);

		// Check if Player is moving forward or strafing
		float ForwardSpeed = FVector::DotProduct(Character->GetVelocity(), Character->GetActorForwardVector());
		float RightSpeed = FVector::DotProduct(Character->GetVelocity(), Character->GetActorRightVector());
		bool IsPlayerMoving = (ForwardSpeed > 200.0f || ForwardSpeed > 200.0f && ((RightSpeed > 200.0f) || (RightSpeed < 200.0f)));

		if (CombatSettings.bEnableMoveToTarget && IsValid(CurrentTarget.HitResult.GetActor()) && IsPlayerMoving)
		{
			float DistanceToTarget = FVector::Dist(CurrentTarget.HitResult.GetActor()->GetActorLocation(), Character->GetActorLocation());
			
			if (CombatSettings.DebugMode)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::SanitizeFloat(DistanceToTarget));
			}	

			if ((DistanceToTarget < IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner())->MoveToTargetMaxDistance && DistanceToTarget > 60.0f) && !IsTartgetObstructed())
			{
				//Get Direction from Target to Player
				FVector Direction = CurrentTarget.HitResult.GetActor()->GetActorLocation() - Character->GetActorLocation();
				Direction.Normalize();

				//Calculate the new location depending on how far we are allowed to travel away.
				FVector NewLocation = CurrentTarget.HitResult.GetActor()->GetActorLocation() + IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner())->MoveToTargetMaxDistance * Direction;

				//Calculate Speed of MoveSmooth based on Min and Max values
				float MovementSpeed = ((DistanceToTarget - 60.0f) / (IAlsxtCombatInterface::Execute_SelectCombatSettings(GetOwner())->MoveToTargetMaxDistance - 60.0f)) * 0.5f;

				//vector from our current location to the target which is slightly further away from the target
				FVector SmoothMoveVector = NewLocation - Character->GetActorLocation();
				FStepDownResult StepdownResult;
				// IALSXTCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->MoveSmooth(SmoothMoveVector, MovementSpeed, &StepdownResult);
				IAlsxtCharacterInterface::Execute_GetCharacter(GetOwner())->LaunchCharacter(SmoothMoveVector * 1.25, true, true);
			}
		}
		
		// Crouch(); //Hack
		FALSXTCharacterVoiceParameters VoiceParameters = IAlsxtCharacterCustomizationComponentInterface::Execute_GetVoiceParameters(GetOwner());
		IAlsxtCharacterSoundComponentInterface::Execute_PlayAttackSound(GetOwner(), true, true, true, VoiceParameters.Sex, VoiceParameters.Variant, Character->GetOverlayMode(), CombatState.CombatParameters.Strength, CombatState.CombatParameters.AttackType, IAlsxtCharacterInterface::Execute_GetStamina(GetOwner()));
	}
}

void UAlsxtCombatComponent::RefreshAttack(const float DeltaTime)
{
	if (Character->GetLocomotionAction() != AlsLocomotionActionTags::PrimaryAction)
	{
		StopAttack();
		Character->ForceNetUpdate();
	}
	else
	{
		RefreshAttackPhysics(DeltaTime);
	}
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
	if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	{
		// Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}

	if (IAlsxtCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->MovementMode == EMovementMode::MOVE_Custom)
	{
		IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
		// Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		// OnAttackEnded();
		OnAttackEndedDelegate.Broadcast();
	}
	IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
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
	if (IsAttackAllowedToStart(Montage) && Character->GetMesh()->GetAnimInstance()->Montage_Play(Montage, PlayRate))
	{
		if (UKismetSystemLibrary::DoesImplementInterface(GetCombatState().CombatParameters.Target, UAlsxtCombatInterface::StaticClass()))
		{
			if (!IAlsxtCollisionInterface::Execute_Blocking(GetCombatState().CombatParameters.Target))
			{
				IAlsxtCombatInterface::Execute_AnticipateSyncedAttack(GetCombatState().CombatParameters.Target, Index);
			}
		}
		CombatState.CombatParameters.TargetYawAngle = TargetYawAngle;
		Character->ALSXTRefreshRotationInstant(StartYawAngle, ETeleportType::None);
		Character->SetLocomotionAction(AlsLocomotionActionTags::PrimaryAction);
		// Crouch(); //Hack
	}
}

void UAlsxtCombatComponent::RefreshSyncedAttack(const float DeltaTime)
{
	if (Character->GetLocomotionAction() != AlsLocomotionActionTags::PrimaryAction)
	{
		StopAttack();
		Character->ForceNetUpdate();
	}
	else
	{
		RefreshSyncedAttackPhysics(DeltaTime);
	}
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
	if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	{
		// Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}

	if (IAlsxtCharacterInterface::Execute_GetCharacterMovementComponent(GetOwner())->MovementMode == EMovementMode::MOVE_Custom)
	{
		IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
		// Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		OnSyncedAttackEnded();
	}
	IAlsxtCharacterInterface::Execute_SetCharacterMovementModeLocked(GetOwner(), false);
}

void UAlsxtCombatComponent::OnSyncedAttackEnded_Implementation() {}