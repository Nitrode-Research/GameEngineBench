#include "Notify/AlsxtAnimNotify_HandPlantEffects.h"

#if WITH_EDITOR
void FALSXTHandPlantEffectSettings::PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent)
{
}

void UALSXTHandPlantEffectsSettings::PostEditChangeProperty(FPropertyChangedEvent& ChangedEvent)
{
	Super::PostEditChangeProperty(ChangedEvent);
}
#endif

FString UAlsxtAnimNotify_HandPlantEffects::GetNotifyName_Implementation() const
{
	return FString("ALSXT Hand Plant Effects");
}

#if WITH_EDITOR
void UAlsxtAnimNotify_HandPlantEffects::OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& NotifyEvent)
{
	Super::OnAnimNotifyCreatedInEditor(NotifyEvent);
}
#endif

void UAlsxtAnimNotify_HandPlantEffects::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Sequence, const FAnimNotifyEventReference& NotifyEventReference)
{
	Super::Notify(Mesh, Sequence, NotifyEventReference);
}

void UAlsxtAnimNotify_HandPlantEffects::SpawnSound(USkeletalMeshComponent* Mesh, const FALSXTHandPlantSoundSettings& SoundSettings, const FVector& FootstepLocation, const FQuat& FootstepRotation) const
{
}
