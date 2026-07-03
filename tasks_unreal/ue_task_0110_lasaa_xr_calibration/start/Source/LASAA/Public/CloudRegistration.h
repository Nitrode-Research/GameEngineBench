// Copyright Expertise centre for Digital Media, 2024. All Rights Reserved.

#pragma once


#include "CoreMinimal.h"
#include "Chaos/DenseMatrix.h"
#include "Chaos/ImplicitQRSVD.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CloudRegistration.generated.h"

using namespace Chaos;

/**
 * Unreal function library to find the transformation that aligns the point clouds
 */
UCLASS()
class LASAA_API UCloudRegistration : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static PMatrix<double,3,3> calculateICPCovarianceMtx(int length, TArray<FVector> centerPCL, TArray<FVector> centerMCL)
	{
		return PMatrix<double,3,3>();
	}

	static FMatrix calculateTransformationMatrix(TArray<FVector> fromPCL3, TArray<FVector> toPCL3, TArray<double> weights)
	{
		return FMatrix::Identity;
	}
};
