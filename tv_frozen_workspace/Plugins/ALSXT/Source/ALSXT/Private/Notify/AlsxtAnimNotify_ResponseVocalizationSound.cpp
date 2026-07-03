// MIT

#include "Notify/AlsxtAnimNotify_ResponseVocalizationSound.h"
#include "AlsCharacter.h"
#include "AlsxtAnimationInstance.h"
#include "AlsxtCharacter.h"
#include "Interfaces/AlsxtCharacterInterface.h"

FString UAlsxtAnimNotify_ResponseVocalizationSound::GetNotifyName_Implementation() const
{
	return FString("ALSXT Impact Response Vocalization Sound Notify");
}

void UAlsxtAnimNotify_ResponseVocalizationSound::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);

	if (!IsValid(Mesh))
	{
		return;
	}

	const auto* World{ Mesh->GetWorld() };
	const auto* Character{ Cast<AAlsCharacter>(Mesh->GetOwner()) };
	AAlsxtCharacter* ALSXTCharacter{ Cast<AAlsxtCharacter>(Mesh->GetOwner()) };
	FGameplayTag Status{ IAlsxtCharacterInterface::Execute_GetCharacterStatus(Mesh->GetOwner()) };

	if (Status == ALSXTStatusTags::Dead)
	{
		return;
	}

	if (IsValid(ALSXTCharacter) && IAlsxtCharacterInterface::Execute_GetCharacterLocomotionMode(Mesh->GetOwner()) == AlsLocomotionModeTags::InAir)
	{
		return;
	}

	if (World->WorldType != EWorldType::EditorPreview)
	{
		// IALSXTCharacterInterface::Execute_PlayCharacterMovementSound(ALSXTCharacter, EnableCharacterMovementAccentSound, EnableWeaponMovementSound, MovementType, WeightTag);
	}
}