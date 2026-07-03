// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRVehicleHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UECRVehicleHealthSet::UECRVehicleHealthSet()
	: Engine_Health(100.0f),
	  Transmission_Health(100.0f),
	  CannonBreech_Health(100.0f),
	  CannonBarrel_Health(100.0f),
	  Radiator_Health(100.0f),
	  FuelTank_Health(100.0f),
	  Track1_Health(100.0f),
	  Track2_Health(100.0f),
	  AmmoRack_Health(100.0f)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



bool UECRVehicleHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRVehicleHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_Engine_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_Transmission_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_CannonBreech_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_CannonBarrel_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_Radiator_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_FuelTank_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_Track1_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_Track2_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRVehicleHealthSet::OnRep_AmmoRack_Health(const FGameplayAttributeData& OldValue) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

