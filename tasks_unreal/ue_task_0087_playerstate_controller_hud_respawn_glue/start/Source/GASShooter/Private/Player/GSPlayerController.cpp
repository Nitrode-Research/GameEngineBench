// Copyright 2020 Dan Kestranek.

#include "Player/GSPlayerController.h"

void AGSPlayerController::CreateHUD()
{
}

UGSHUDWidget* AGSPlayerController::GetGSHUD()
{
	return UIHUDWidget;
}

void AGSPlayerController::SetEquippedWeaponPrimaryIconFromSprite(UPaperSprite* InSprite)
{
}

void AGSPlayerController::SetEquippedWeaponStatusText(const FText& StatusText)
{
}

void AGSPlayerController::SetPrimaryClipAmmo(int32 ClipAmmo)
{
}

void AGSPlayerController::SetPrimaryReserveAmmo(int32 ReserveAmmo)
{
}

void AGSPlayerController::SetSecondaryClipAmmo(int32 SecondaryClipAmmo)
{
}

void AGSPlayerController::SetSecondaryReserveAmmo(int32 SecondaryReserveAmmo)
{
}

void AGSPlayerController::SetHUDReticle(TSubclassOf<UGSHUDReticle> ReticleClass)
{
}

void AGSPlayerController::ShowDamageNumber_Implementation(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags)
{
}

bool AGSPlayerController::ShowDamageNumber_Validate(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags)
{
	return true;
}

void AGSPlayerController::SetRespawnCountdown_Implementation(float RespawnTimeRemaining)
{
}

bool AGSPlayerController::SetRespawnCountdown_Validate(float RespawnTimeRemaining)
{
	return true;
}

void AGSPlayerController::ClientSetControlRotation_Implementation(FRotator NewRotation)
{
}

bool AGSPlayerController::ClientSetControlRotation_Validate(FRotator NewRotation)
{
	return true;
}

void AGSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AGSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void AGSPlayerController::Kill()
{
}

void AGSPlayerController::ServerKill_Implementation()
{
}

bool AGSPlayerController::ServerKill_Validate()
{
	return true;
}
