// Copyleft: All rights reversed

#include "Gameplay/GAS/Executions/ECRDamageExecution.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Gameplay/ECRGameplayBlueprintLibrary.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "System/ECRLogChannels.h"
#include "System/Messages/ECRVerbMessage.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Reflect_Message, "ECR.Reflect.Message");

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;
	FGameplayEffectAttributeCaptureDefinition ToughnessDef;
	FGameplayEffectAttributeCaptureDefinition IncomingDamageMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition ArmorDef;
	FGameplayEffectAttributeCaptureDefinition OutgoingMeleeDamageMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition IncomingMeleeDamageMitigationDef;
	FGameplayEffectAttributeCaptureDefinition IncomingNonMeleeDamageMitigationDef;

	FDamageStatics()
	{
		// Common source attributes
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetBaseDamageAttribute(),
		                                                          EGameplayEffectAttributeCaptureSource::Source,
		                                                          true);
		ToughnessDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetToughnessAttribute(),
		                                                         EGameplayEffectAttributeCaptureSource::Target,
		                                                         false);
		IncomingDamageMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingDamageMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
		ArmorDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetArmorAttribute(),
		                                                     EGameplayEffectAttributeCaptureSource::Target,
		                                                     false);

		OutgoingMeleeDamageMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetOutgoingMeleeDamageMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Source,
			true);

		IncomingMeleeDamageMitigationDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingMeleeDamageMitigationAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
		IncomingNonMeleeDamageMitigationDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingNonMeleeDamageMitigationAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
	}
};

static FDamageStatics& DamageStatics()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	static static FDamageStatics StubValue{};
	return StubValue;
}



UECRDamageExecution::UECRDamageExecution()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRDamageExecution::SendReflectMessage(const FGameplayEffectSpec& Spec, AActor* Target,
                                             double Magnitude) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

