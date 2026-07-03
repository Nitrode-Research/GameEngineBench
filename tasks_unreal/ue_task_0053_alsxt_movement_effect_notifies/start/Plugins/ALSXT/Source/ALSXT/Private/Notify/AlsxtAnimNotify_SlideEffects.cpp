#include "Notify/AlsxtAnimNotify_SlideEffects.h"

FString UAlsxtAnimNotify_SlideEffects::GetNotifyName_Implementation() const
{
	return FString("ALSXT Slide Effects");
}

void UAlsxtAnimNotify_SlideEffects::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);
}
