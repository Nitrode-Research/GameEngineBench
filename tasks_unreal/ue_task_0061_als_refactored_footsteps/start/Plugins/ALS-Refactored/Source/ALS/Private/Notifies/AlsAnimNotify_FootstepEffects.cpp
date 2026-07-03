#include "Notifies/AlsAnimNotify_FootstepEffects.h"

#include "AlsCharacter.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "Utility/AlsConstants.h"
#include "Utility/AlsDebugUtility.h"
#include "Utility/AlsEnumUtility.h"
#include "Utility/AlsMacros.h"
#include "Utility/AlsMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlsAnimNotify_FootstepEffects)

#if WITH_EDITOR
void FAlsFootstepDecalSettings::PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent)
{
	FootLeftRotationOffsetQuaternion = FootLeftRotationOffset.Quaternion();
	FootRightRotationOffsetQuaternion = FootRightRotationOffset.Quaternion();
}

void FAlsFootstepParticleSystemSettings::PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent)
{
	FootLeftRotationOffsetQuaternion = FootLeftRotationOffset.Quaternion();
	FootRightRotationOffsetQuaternion = FootRightRotationOffset.Quaternion();
}

void FAlsFootstepEffectSettings::PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent)
{
	Decal.PostEditChangeProperty(ChangedEvent);
	ParticleSystem.PostEditChangeProperty(ChangedEvent);
}

void UAlsFootstepEffectsSettings::PostEditChangeProperty(FPropertyChangedEvent& ChangedEvent)
{
	if (ChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_STRING_VIEW_CHECKED(ThisClass, DecalSpawnAngleThreshold))
	{
		DecalSpawnAngleThresholdCos = FMath::Cos(FMath::DegreesToRadians(DecalSpawnAngleThreshold));
	}
	else if (ChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_STRING_VIEW_CHECKED(ThisClass, Effects))
	{
		for (auto& [SurfaceType, EffectSettings] : Effects)
		{
			EffectSettings.PostEditChangeProperty(ChangedEvent);
		}
	}

	Super::PostEditChangeProperty(ChangedEvent);
}
#endif

FString UAlsAnimNotify_FootstepEffects::GetNotifyName_Implementation() const
{
	// For some reason editor cuts off some characters at the end of the string, so to avoid this we insert a bunch of spaces.
	// TODO Check the need for this hack in future engine versions.

	TStringBuilder<64> NotifyNameBuilder{
		InPlace, TEXTVIEW("Als Footstep Effects: "), AlsEnumUtility::GetNameStringByValue(FootBone), TEXTVIEW("        ")
	};

	return FString{NotifyNameBuilder};
}

#if WITH_EDITOR
void UAlsAnimNotify_FootstepEffects::OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& NotifyEvent)
{
	Super::OnAnimNotifyCreatedInEditor(NotifyEvent);

	NotifyEvent.bTriggerOnDedicatedServer = false;
}
#endif

void UAlsAnimNotify_FootstepEffects::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Sequence,
                                            const FAnimNotifyEventReference& NotifyEventReference)
{
	Super::Notify(Mesh, Sequence, NotifyEventReference);
}

void UAlsAnimNotify_FootstepEffects::SpawnSound(USkeletalMeshComponent* Mesh, const FAlsFootstepSoundSettings& SoundSettings,
                                                const FVector& FootstepLocation, const FQuat& FootstepRotation) const
{
}

void UAlsAnimNotify_FootstepEffects::SpawnDecal(USkeletalMeshComponent* Mesh, const FAlsFootstepDecalSettings& DecalSettings,
                                                const FVector& FootstepLocation, const FQuat& FootstepRotation,
                                                const FHitResult& FootstepHit, const FVector& FootUpAxis) const
{
}

void UAlsAnimNotify_FootstepEffects::SpawnParticleSystem(USkeletalMeshComponent* Mesh,
                                                         const FAlsFootstepParticleSystemSettings& ParticleSystemSettings,
                                                         const FVector& FootstepLocation, const FQuat& FootstepRotation) const
{
}
