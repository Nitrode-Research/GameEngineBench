// MIT

#include "Notify/AlsxtAnimNotify_CharacterMovementSound.h"
#include "AlsCharacter.h"
#include "AlsxtAnimationInstance.h"
#include "AlsxtCharacter.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "Interfaces/AlsxtCharacterSoundComponentInterface.h"


FString UAlsxtAnimNotify_CharacterMovementSound::GetNotifyName_Implementation() const
{
	return FString("ALSXT Character Movement Sound Notify");
}

void UAlsxtAnimNotify_CharacterMovementSound::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);

	if (!IsValid(Mesh))
	{
		return;
	}

	const auto* World{ Mesh->GetWorld() };

	if (World->WorldType != EWorldType::EditorPreview)
	{
		FGameplayTag WeightTag = IAlsxtCharacterInterface::Execute_GetWeightTag(Mesh->GetOwner());
		IAlsxtCharacterSoundComponentInterface::Execute_PlayCharacterMovementSound(Mesh->GetOwner(), EnableCharacterMovementAccentSound, EnableWeaponMovementSound, MovementType, WeightTag);
	}
}