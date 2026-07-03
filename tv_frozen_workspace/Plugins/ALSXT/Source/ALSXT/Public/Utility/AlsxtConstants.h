#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsxtConstants.generated.h"

UCLASS(Meta = (BlueprintThreadSafe))
class ALSXT_API UAlsxtConstants : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Pose Animation Curves

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseLeanLeftCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseLeanRightCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseCantedCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseVaultingCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseSlidingCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseWallJumpCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseWallRunCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseStationaryCurveName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants|Animation Curves", Meta = (ReturnDisplayName = "Curve Name"))
	static const FName& PoseSeatedCurveName();

	// Debug

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& ALSXTCurvesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& ALSXTStateDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& ALSXTShapesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& ALSXTTracesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& VaultingDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& SlidingDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& WallJumpDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "ALS|Constants", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& WallRunDebugDisplayName();
};

inline const FName& UAlsxtConstants::PoseLeanLeftCurveName()
{
	static const FName Name{TEXTVIEW("PoseLeanLeft")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseLeanRightCurveName()
{
	static const FName Name{TEXTVIEW("PoseLeanRight")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseCantedCurveName()
{
	static const FName Name{TEXTVIEW("PoseCanted")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseVaultingCurveName()
{
	static const FName Name{TEXTVIEW("PoseVaulting")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseSlidingCurveName()
{
	static const FName Name{TEXTVIEW("PoseSliding")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseWallJumpCurveName()
{
	static const FName Name{TEXTVIEW("PoseWallJump")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseWallRunCurveName()
{
	static const FName Name{TEXTVIEW("PoseWallRun")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseStationaryCurveName()
{
	static const FName Name{TEXTVIEW("PoseStationary")};
	return Name;
}

inline const FName& UAlsxtConstants::PoseSeatedCurveName()
{
	static const FName Name{TEXTVIEW("PoseSeated")};
	return Name;
}

inline const FName& UAlsxtConstants::ALSXTCurvesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.Curves")};
	return Name;
}

inline const FName& UAlsxtConstants::ALSXTStateDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.State")};
	return Name;
}

inline const FName& UAlsxtConstants::ALSXTShapesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.Shapes")};
	return Name;
}

inline const FName& UAlsxtConstants::ALSXTTracesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.Traces")};
	return Name;
}

inline const FName& UAlsxtConstants::VaultingDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.Vaulting")};
	return Name;
}

inline const FName& UAlsxtConstants::SlidingDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.Sliding")};
	return Name;
}

inline const FName& UAlsxtConstants::WallJumpDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.WallJump")};
	return Name;
}

inline const FName& UAlsxtConstants::WallRunDebugDisplayName()
{
	static const FName Name{TEXTVIEW("ALS.WallRun")};
	return Name;
}
