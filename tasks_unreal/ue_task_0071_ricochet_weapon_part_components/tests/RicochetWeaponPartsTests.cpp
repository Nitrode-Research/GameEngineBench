// Copyright 2026 GameDevBench. Ricochet weapon-part component source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace RicochetWeaponPartsTests
{
	static bool LoadComponentSource(const TCHAR* Name, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Plugins/Ricochet/Source/Ricochet/Private/Components") / Name);
		return FFileHelper::LoadFileToString(Out, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRicochetWeaponParts_BarrelAndMuzzleSource,
	"TargetVector.RicochetWeaponParts.barrel_and_muzzle_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRicochetWeaponParts_BarrelAndMuzzleSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates barrel/muzzle ability-system setup, replication parameters, and public accessors.
	FString BarrelSource;
	FString MuzzleSource;
	if (!TestTrue(TEXT("Ricochet barrel source readable"), RicochetWeaponPartsTests::LoadComponentSource(TEXT("RicochetBarrel.cpp"), BarrelSource))
		|| !TestTrue(TEXT("Ricochet muzzle source readable"), RicochetWeaponPartsTests::LoadComponentSource(TEXT("RicochetMuzzleAttachment.cpp"), MuzzleSource)))
	{
		return true;
	}

	TestTrue(TEXT("Barrel must create replicated ASC with minimal replication"),
		BarrelSource.Contains(TEXT("CreateDefaultSubobject<UAbilitySystemComponent>")) && BarrelSource.Contains(TEXT("SetIsReplicated(true)"))
		&& BarrelSource.Contains(TEXT("SetReplicationMode(EGameplayEffectReplicationMode::Minimal)")));
	TestTrue(TEXT("Barrel must replicate compatible calibers and muzzle attachments with skip-owner push params"),
		BarrelSource.Contains(TEXT("FDoRepLifetimeParams")) && BarrelSource.Contains(TEXT("bIsPushBased = true")) && BarrelSource.Contains(TEXT("COND_SkipOwner"))
		&& BarrelSource.Contains(TEXT("DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CompatibleCalibers"))
		&& BarrelSource.Contains(TEXT("DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CompatibleMuzzleAttachments")));
	TestTrue(TEXT("Barrel must expose its ability system component"),
		BarrelSource.Contains(TEXT("GetAbilitySystemComponent")) && BarrelSource.Contains(TEXT("return AbilitySystemComponent")));
	TestTrue(TEXT("Muzzle attachment must create replicated ASC with minimal replication and expose accessors"),
		MuzzleSource.Contains(TEXT("CreateDefaultSubobject<UAbilitySystemComponent>")) && MuzzleSource.Contains(TEXT("SetIsReplicated(true)"))
		&& MuzzleSource.Contains(TEXT("SetReplicationMode(EGameplayEffectReplicationMode::Minimal)"))
		&& MuzzleSource.Contains(TEXT("GetMuzzleAttachmentWeight")) && MuzzleSource.Contains(TEXT("GetAbilitySystemComponent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRicochetWeaponParts_MagazineSource,
	"TargetVector.RicochetWeaponParts.magazine_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRicochetWeaponParts_MagazineSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates magazine ability-system setup and detachable magazine weight aggregation.
	FString BaseSource;
	FString DetachableSource;
	if (!TestTrue(TEXT("Ricochet magazine base source readable"), RicochetWeaponPartsTests::LoadComponentSource(TEXT("RicochetMagazineBase.cpp"), BaseSource))
		|| !TestTrue(TEXT("Ricochet detachable magazine source readable"), RicochetWeaponPartsTests::LoadComponentSource(TEXT("RicochetMagazineDetachable.cpp"), DetachableSource)))
	{
		return true;
	}

	TestTrue(TEXT("Magazine base must create replicated ASC with minimal replication"),
		BaseSource.Contains(TEXT("CreateDefaultSubobject<UAbilitySystemComponent>")) && BaseSource.Contains(TEXT("SetIsReplicated(true)"))
		&& BaseSource.Contains(TEXT("SetReplicationMode(EGameplayEffectReplicationMode::Minimal)")));
	TestTrue(TEXT("Magazine base must expose its ability system component"),
		BaseSource.Contains(TEXT("GetAbilitySystemComponent")) && BaseSource.Contains(TEXT("return AbilitySystemComponent")));
	TestTrue(TEXT("Detachable magazine must tick and return configured magazine weight"),
		DetachableSource.Contains(TEXT("PrimaryComponentTick.bCanEverTick = true")) && DetachableSource.Contains(TEXT("return MagazineWeight")));
	TestTrue(TEXT("Detachable magazine total weight must include round weights when ammunition is loaded"),
		DetachableSource.Contains(TEXT("Ammunition.IsEmpty()")) && DetachableSource.Contains(TEXT("RoundsWeight"))
		&& DetachableSource.Contains(TEXT("Round.Get()->GetRoundWeight()")) && DetachableSource.Contains(TEXT("RoundsWeight + GetMagazineWeight()")));
	return true;
}
