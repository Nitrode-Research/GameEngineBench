#pragma once

#include "Utility/ALSXTStructs.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "AlsxtBloodSpatterSettings.generated.h"

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtBloodSpatterParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FGameplayTag Location{FGameplayTag::EmptyTag};
};

UCLASS(Blueprintable, BlueprintType)
class ALSXT_API UAlsxtBloodSpatterSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (TitleProperty = "{Velocity} {Volume} {Distance} {MaxDripSpeed}", AllowPrivateAccess))
	TArray<FALSXTBloodSpatterType> ActionSounds;
};

USTRUCT(BlueprintType)
struct ALSXT_API FAlsxtGeneralBloodSpatterSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowBloodSpatters{ true };

};