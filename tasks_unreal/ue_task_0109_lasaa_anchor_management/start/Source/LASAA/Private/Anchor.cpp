#include "Anchor.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "OculusXRAnchors.h"
#include "OculusXRAnchorComponent.h"
#include "OculusXRAnchorBPFunctionLibrary.h"

// Sets default values
AAnchor::AAnchor()
{
}

// Called when the game starts or when spawned
void AAnchor::BeginPlay()
{
	Super::BeginPlay();
}

void AAnchor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool AAnchor::createAnchor(FTransform extTrans)
{
	return false;
}

bool AAnchor::save()
{
	return false;
}

void AAnchor::eraseFromList()
{
}

void AAnchor::addToList()
{
}

void AAnchor::writeToJson()
{
}

void AAnchor::readFromJson()
{
}

void AAnchor::readFromJsonBuffer(const FString& jsonString)
{
}

void AAnchor::erase()
{
}

int AAnchor::loadAnchorsInternal(UClass* extClass, UClass* anchorClass, AActor* newOwner)
{
	return 0;
}

int AAnchor::loadAnchors(UClass* extClass, UClass* anchorClass, AActor* newOwner)
{
	return 0;
}

int AAnchor::loadAnchorsFromBuffer(const FString& jsonString, UClass* extClass, UClass* anchorClass, AActor* newOwner)
{
	return 0;
}

void AAnchor::resetAnchors()
{
}

void AAnchor::setUuid(FString id)
{
}

void AAnchor::addCalibrationOffset()
{
}

int AAnchor::getNumAnchors()
{
	return 0;
}

void AAnchor::setCalibrationOffset(const FTransform& calibration)
{
}

double AAnchor::getAvgPairDistance()
{
	return 0.0;
}

FString AAnchor::getUuid()
{
	return FString();
}

void AAnchor::setGtExtPose(FTransform extTrans)
{
}

FVector AAnchor::getCurrentExtLocation()
{
	return FVector::Zero();
}

FTransform AAnchor::getCurrentExtPose()
{
	return FTransform::Identity;
}

void AAnchor::setExtPairAndCalibrationOffset(AActor* extActor)
{
}

AActor* AAnchor::getExtPair()
{
	return nullptr;
}
