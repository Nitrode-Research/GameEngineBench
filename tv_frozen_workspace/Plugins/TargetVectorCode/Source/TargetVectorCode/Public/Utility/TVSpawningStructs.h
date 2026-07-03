#pragma once

#include "NativeGameplayTags.h"
#include "TVGameplayTags.h"
#include "TVSpawningStructs.generated.h"

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FDeathSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FGameplayTag DeathType;
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FRespawnSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	bool EnableRespawning {true};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int BaseRespawnCooldownLength {5};

	bool operator==(const FRespawnSettings& other) const
	{
		return (other.BaseRespawnCooldownLength == BaseRespawnCooldownLength);
	}
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FContestRound
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FGameplayTag ContestType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int AdditionalTickets{ 0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float AdditionalPoints{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float AdditionalCash{ 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float AdditionalXP{ 0.0f };
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FRespawnContestSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	bool EnableRespawnContest {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Tickets {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FContestRound> ContestRounds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))	
	bool Random {false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float RespawnContestDelay {0.0f};
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))	
	float PreRoundDelay {0.0f};
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	float PreRoundCountdownLength {0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))	
	float RespawnDelay {0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))	
	float RespawnCountdownLength {0.0f};

	bool operator==(const FRespawnContestSettings& other) const
	{
		return (other.EnableRespawnContest == EnableRespawnContest);
	}
};