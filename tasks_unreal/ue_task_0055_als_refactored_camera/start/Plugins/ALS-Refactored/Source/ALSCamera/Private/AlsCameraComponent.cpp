#include "AlsCameraComponent.h"

#include "AlsCameraSettings.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/WorldSettings.h"
#include "Utility/AlsCameraConstants.h"
#include "Utility/AlsDebugUtility.h"
#include "Utility/AlsMacros.h"
#include "Utility/AlsRotation.h"
#include "Utility/AlsUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlsCameraComponent)

UAlsCameraComponent::UAlsCameraComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	bTickInEditor = false;
	bHiddenInGame = true;
}

void UAlsCameraComponent::PostLoad()
{
	Super::PostLoad();

	// This is required for the camera to work properly, as its mesh is never rendered.
	// We change the tick option here to override the value that comes from the config file.
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void UAlsCameraComponent::OnRegister()
{
	Character = Cast<ACharacter>(GetOwner());

	Super::OnRegister();
}

void UAlsCameraComponent::RegisterComponentTickFunctions(const bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	// Tick after the owner to have access to the most up-to-date character state.

	AddTickPrerequisiteActor(GetOwner());
}

void UAlsCameraComponent::Activate(const bool bReset)
{
	if (bReset || ShouldActivate())
	{
		TickCamera(0.0f, false);
	}

	Super::Activate(bReset);
}

void UAlsCameraComponent::InitAnim(const bool bForceReinitialize)
{
	Super::InitAnim(bForceReinitialize);

	AnimationInstance = GetAnimInstance();
}

void UAlsCameraComponent::BeginPlay()
{
	ALS_ENSURE(IsValid(GetAnimInstance()));
	ALS_ENSURE(IsValid(Settings));
	ALS_ENSURE(IsValid(Character));

	Super::BeginPlay();
}

void UAlsCameraComponent::TickComponent(float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAlsCameraComponent::CompleteParallelAnimationEvaluation(const bool bDoPostAnimationEvaluation)
{
	Super::CompleteParallelAnimationEvaluation(bDoPostAnimationEvaluation);
}

FVector UAlsCameraComponent::GetFirstPersonCameraLocation() const
{
	return Character->GetMesh()->GetSocketLocation(Settings->FirstPerson.CameraSocketName);
}

FVector UAlsCameraComponent::GetThirdPersonPivotLocation() const
{
	const auto* Mesh{Character->GetMesh()};

	FVector FirstPivotLocation;

	if (!IsValid(Mesh->GetAttachParent()) && Settings->ThirdPerson.FirstPivotSocketName == UAlsConstants::RootBoneName())
	{
		// The root bone location usually remains fixed when the mesh is detached, so use the capsule's bottom location here as a fallback.

		FirstPivotLocation = Character->GetRootComponent()->GetComponentLocation();
		FirstPivotLocation.Z -= Character->GetRootComponent()->Bounds.BoxExtent.Z;
	}
	else
	{
		FirstPivotLocation = Mesh->GetSocketLocation(Settings->ThirdPerson.FirstPivotSocketName);
	}

	return (FirstPivotLocation + Mesh->GetSocketLocation(Settings->ThirdPerson.SecondPivotSocketName)) * 0.5f;
}

FVector UAlsCameraComponent::GetThirdPersonTraceStartLocation() const
{
	return Character->GetMesh()->GetSocketLocation(bRightShoulder
		                                               ? Settings->ThirdPerson.TraceShoulderRightSocketName
		                                               : Settings->ThirdPerson.TraceShoulderLeftSocketName);
}

void UAlsCameraComponent::GetViewInfo(FMinimalViewInfo& ViewInfo) const
{
	ViewInfo.Location = CameraLocation;
	ViewInfo.Rotation = CameraRotation;
	ViewInfo.FOV = CameraFieldOfView;

	ViewInfo.PostProcessBlendWeight = IsValid(Settings) ? PostProcessWeight : 0.0f;

	if (ViewInfo.PostProcessBlendWeight > UE_SMALL_NUMBER)
	{
		ViewInfo.PostProcessSettings = Settings->PostProcess;
	}
}

void UAlsCameraComponent::TickCamera(const float DeltaTime, bool bAllowLag)
{
	if (!IsValid(GetAnimInstance()) || !IsValid(Settings) || !IsValid(Character))
	{
		return;
	}

	PivotTargetLocation = GetThirdPersonPivotLocation();
	PivotLagLocation = PivotTargetLocation;
	PivotLocation = PivotTargetLocation;
	CameraRotation = Character->GetViewRotation();
	CameraLocation = GetFirstPersonCameraLocation();
	CameraFieldOfView = Settings->ThirdPerson.FieldOfView;
}

FRotator UAlsCameraComponent::CalculateCameraRotation(const FRotator& CameraTargetRotation,
                                                      const float DeltaTime, const bool bAllowLag) const
{
	if (!bAllowLag)
	{
		return CameraTargetRotation;
	}

	const auto RotationLag{GetAnimInstance()->GetCurveValue(UAlsCameraConstants::RotationLagCurveName())};

	return UAlsRotation::DamperExactRotation(CameraRotation, CameraTargetRotation, DeltaTime, RotationLag);
}

FVector UAlsCameraComponent::CalculatePivotLagLocation(const FQuat& CameraYawRotation, const float DeltaTime, const bool bAllowLag) const
{
	return PivotTargetLocation;
}

FVector UAlsCameraComponent::CalculatePivotOffset() const
{
	return FVector::ZeroVector;
}

FVector UAlsCameraComponent::CalculateCameraOffset() const
{
	return FVector::ZeroVector;
}

float UAlsCameraComponent::CalculateFovOffset() const
{
	return GetAnimInstance()->GetCurveValue(UAlsCameraConstants::FovOffsetCurveName());
}

FVector UAlsCameraComponent::CalculateCameraTrace(const FVector& CameraTargetLocation, const FVector& PivotOffset,
                                                  const float DeltaTime, const bool bAllowLag, float& NewTraceDistanceRatio) const
{
	NewTraceDistanceRatio = 1.0f;
	return CameraTargetLocation;
}

bool UAlsCameraComponent::TryAdjustLocationBlockedByGeometry(FVector& Location, const bool bDisplayDebugCameraTraces) const
{
	// Based on ComponentEncroachesBlockingGeometry_WithAdjustment().

	const auto MeshScale{UE_REAL_TO_FLOAT(Character->GetMesh()->GetComponentScale().Z)};
	const auto CollisionShape{FCollisionShape::MakeSphere((Settings->ThirdPerson.TraceRadius + 1.0f) * MeshScale)};

	static TArray<FOverlapResult> Overlaps;
	check(Overlaps.IsEmpty())

	ON_SCOPE_EXIT
	{
		Overlaps.Reset();
	};

	static const FName OverlapMultiTraceTag{FString::Printf(TEXT("%hs (Overlap Multi)"), __FUNCTION__)};

	if (!GetWorld()->OverlapMultiByChannel(Overlaps, Location, FQuat::Identity, Settings->ThirdPerson.TraceChannel,
	                                       CollisionShape, {OverlapMultiTraceTag, false, GetOwner()}))
	{
		return false;
	}

	auto Adjustment{FVector::ZeroVector};
	auto bAnyValidBlock{false};

	FMTDResult MtdResult;

	for (const auto& Overlap : Overlaps)
	{
		if (!Overlap.Component.IsValid() ||
		    Overlap.Component->GetCollisionResponseToChannel(Settings->ThirdPerson.TraceChannel) != ECR_Block)
		{
			continue;
		}

		const auto* OverlapBody{Overlap.Component->GetBodyInstance(NAME_None, true, Overlap.GetItemIndex())};

		if (OverlapBody == nullptr || !OverlapBody->OverlapTest(Location, FQuat::Identity, CollisionShape, &MtdResult))
		{
			return false;
		}

		if (!FMath::IsNearlyZero(MtdResult.Distance))
		{
			Adjustment += MtdResult.Direction * MtdResult.Distance;
			bAnyValidBlock = true;
		}
	}

	if (!bAnyValidBlock)
	{
		return false;
	}

	auto AdjustmentDirection{Adjustment};

	if (!AdjustmentDirection.Normalize() ||
	    ((GetOwner()->GetActorLocation() - Location).GetSafeNormal() | AdjustmentDirection) < -UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

#if ENABLE_DRAW_DEBUG
	if (bDisplayDebugCameraTraces)
	{
		DrawDebugLine(GetWorld(), Location, Location + Adjustment,
		              FLinearColor{0.0f, 0.75f, 1.0f}.ToFColor(true),
		              false, 5.0f, 0, UAlsDebugUtility::DrawLineThickness);
	}
#endif

	Location += Adjustment;

	static const FName FreeSpaceTraceTag{FString::Printf(TEXT("%hs (Free Space Overlap)"), __FUNCTION__)};

	return !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, Settings->ThirdPerson.TraceChannel,
	                                                 FCollisionShape::MakeSphere(Settings->ThirdPerson.TraceRadius * MeshScale),
	                                                 {FreeSpaceTraceTag, false, GetOwner()});
}
