#pragma once

#include "NativeGameplayTags.h"
#include "TVGameplayTags.h"
#include "TVSpectatingStructs.generated.h"

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FSpectatingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	bool EnableSpectating{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float SpectatingDelay{ 2.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FName SpectatingTag{ "Spectate" };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Game.Lobby Mode", AllowPrivateAccess))
	FGameplayTagContainer AllowedSpectatorViewModes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	bool EnableKillCamTransition{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories = "Game.Lobby Mode", AllowPrivateAccess))
	FGameplayTagContainer AllowedViewModes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int InitialDelay {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int DeathDelay {0};

	bool operator==(const FSpectatingSettings& other) const
	{
		return (other.EnableSpectating == EnableSpectating) && (other.SpectatingDelay == SpectatingDelay);
	}
};