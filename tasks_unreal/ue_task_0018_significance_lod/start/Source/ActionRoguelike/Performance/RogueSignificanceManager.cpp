// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueSignificanceManager.h"
#include "RogueSignificanceInterface.h"
#include "RogueSignificanceSettings.h"


void URogueSignificanceManager::Update(TArrayView<const FTransform> InViewpoints)
{
	Super::Update(InViewpoints);
}


void URogueSignificanceManager::RegisterObject(UObject* Object, FName Tag, FManagedObjectSignificanceFunction SignificanceFunction, EPostSignificanceType InPostSignificanceType,
	FManagedObjectPostSignificanceFunction InPostSignificanceFunction)
{
	Super::RegisterObject(Object, Tag, SignificanceFunction, InPostSignificanceType, InPostSignificanceFunction);
}


void URogueSignificanceManager::UnregisterObject(UObject* Object)
{
	Super::UnregisterObject(Object);
}
