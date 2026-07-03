// Copyright Expertise centre for Digital Media, 2024. All Rights Reserved.


#include "XRCalibration.h"
#include <Eigen/Core>
#include <Eigen/SVD>
#include <unsupported/Eigen/NonLinearOptimization>
#include "Anchor.h"

using namespace Eigen;

UXRCalibration::UXRCalibration()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UXRCalibration::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UXRCalibration::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UXRCalibration::calibrate()
{
}


void UXRCalibration::resetCalibration()
{
}

void UXRCalibration::changeCalibration()
{
}

void UXRCalibration::loadFromFile()
{
}

void UXRCalibration::writeToFile(int i)
{
}
