// MIT

#pragma once

#include "Utility/TVMusicStructs.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "GeneralMusicSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODE_API UGeneralMusicSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music|Music Sets")
	TArray<FMusicSet> MusicSets;
 
	UGeneralMusicSettings();
	
};