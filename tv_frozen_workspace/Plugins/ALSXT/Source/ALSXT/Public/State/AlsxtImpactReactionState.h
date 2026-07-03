#pragma once
#include "Settings/AlsxtImpactReactionSettings.h"
#include "Utility/AlsxtCollisionGameplayTags.h"
#include "AlsxtImpactReactionState.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtImpactReactionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	FAlsxtImpactReactionParameters ImpactReactionParameters;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtPhysicalAnimationState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	FGameplayTag Mode {AlsxtPhysicalAnimationModeTags::None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	FName ProfileName {"CharacterMesh"};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	TArray<FName> AffectedBonesBelow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	float Alpha { 0.0f };
};