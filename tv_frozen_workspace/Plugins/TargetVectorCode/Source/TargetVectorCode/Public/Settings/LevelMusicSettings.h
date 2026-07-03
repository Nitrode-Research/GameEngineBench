// MIT

#pragma once

#include "Utility/TVMusicStructs.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "LevelMusicSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TARGETVECTORCODE_API ULevelMusicSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music|Music Sets")
	TArray<FLevelMusicSet> LevelMusicSets;

	ULevelMusicSettings();

};