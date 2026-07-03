// Copyright 2020 Dan Kestranek.

#include "GASShooterGameModeBase.h"

AGASShooterGameModeBase::AGASShooterGameModeBase()
{
	RespawnDelay = 5.0f;
	HeroClass = nullptr;
	EnemySpawnPoint = nullptr;
}

void AGASShooterGameModeBase::HeroDied(AController* Controller)
{
}

void AGASShooterGameModeBase::BeginPlay()
{
	Super::BeginPlay();
}

void AGASShooterGameModeBase::RespawnHero(AController* Controller)
{
}
