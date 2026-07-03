#pragma once

#include "NativeGameplayTags.h"
#include "Engine/DataTable.h"
#include "TVGameplayTags.h"
#include "Misc/DateTime.h"
#include "TVModerationStructs.generated.h"

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FTeamKillPenalty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FGameplayTag TeamKillWarningThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int TeamKillPenaltyThreshold {0};
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FTeamKillSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int TeamKillWarningThreshold {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int TeamKillPenaltyThreshold {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FTeamKillPenalty> TeamKillPlayerPenalty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	TArray<FTeamKillPenalty> TeamKillUnitPenalty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int TeamKillKickThreshold {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int TeamKillKickLength {0};
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FProfaneWord
	: public FTableRowBase
{
		GENERATED_BODY()

	public:
		/** The language (for localization) */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		FName Language;

		/** Where 0 is the most severe */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int Severity {0};

		/** The word itself */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		FString Word;
};

USTRUCT(BlueprintType)
struct TARGETVECTORCODE_API FPlayerProfanityRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	FString TextCharacterLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess))
	int Severity {0};
};