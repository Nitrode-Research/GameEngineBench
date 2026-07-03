// Copyright 2026 GameDevBench. GASShooter character base source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGSCharacterBase_SourceContract,
	"GASShooter.CharacterBase.source_contract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGSCharacterBase_SourceContract::RunTest(const FString& Parameters)
{
	// REQUIRED: validates shared character construction, attributes, ability grants, death lifecycle, startup effects, and damage-number UI.
	const FString SourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("Source/GASShooter/Private/Characters/GSCharacterBase.cpp"));

	FString Source;
	if (!TestTrue(TEXT("GSCharacterBase source readable"), FFileHelper::LoadFileToString(Source, *SourcePath)))
	{
		return true;
	}

	TestTrue(TEXT("Constructor must configure movement, collision, relevancy, tags, and damage widget class"),
		Source.Contains(TEXT("SetDefaultSubobjectClass<UGSCharacterMovementComponent>"))
		&& Source.Contains(TEXT("SetCollisionResponseToChannel")) && Source.Contains(TEXT("ECC_Visibility"))
		&& Source.Contains(TEXT("ECC_Camera")) && Source.Contains(TEXT("bAlwaysRelevant = true"))
		&& Source.Contains(TEXT("State.Dead")) && Source.Contains(TEXT("Effect.RemoveOnDeath"))
		&& Source.Contains(TEXT("StaticLoadClass")) && Source.Contains(TEXT("WC_DamageText")));
	TestTrue(TEXT("Attribute accessors must read GSAttributeSetBase values"),
		Source.Contains(TEXT("AttributeSetBase->GetHealth")) && Source.Contains(TEXT("AttributeSetBase->GetMaxHealth"))
		&& Source.Contains(TEXT("AttributeSetBase->GetMana")) && Source.Contains(TEXT("AttributeSetBase->GetStamina"))
		&& Source.Contains(TEXT("AttributeSetBase->GetShield")) && Source.Contains(TEXT("AttributeSetBase->GetMoveSpeed"))
		&& Source.Contains(TEXT("GetMoveSpeedAttribute().GetGameplayAttributeData(AttributeSetBase)->GetBaseValue"))
		&& Source.Contains(TEXT("return GetHealth() > 0.0f")));
	TestTrue(TEXT("Ability grants must be authority-only, duplicate guarded, and use ability input/source data"),
		Source.Contains(TEXT("GetLocalRole() != ROLE_Authority")) && Source.Contains(TEXT("bCharacterAbilitiesGiven"))
		&& Source.Contains(TEXT("for (TSubclassOf<UGSGameplayAbility>& StartupAbility : CharacterAbilities)"))
		&& Source.Contains(TEXT("GiveAbility")) && Source.Contains(TEXT("GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityID)"))
		&& Source.Contains(TEXT("static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID)")) && Source.Contains(TEXT(", this)")));
	TestTrue(TEXT("Ability removal must clear only character-sourced startup abilities and reset the flag"),
		Source.Contains(TEXT("GetActivatableAbilities")) && Source.Contains(TEXT("Spec.SourceObject == this"))
		&& Source.Contains(TEXT("CharacterAbilities.Contains(Spec.Ability->GetClass())")) && Source.Contains(TEXT("ClearAbility"))
		&& Source.Contains(TEXT("bCharacterAbilitiesGiven = false")));
	TestTrue(TEXT("Death must clean abilities, movement/collision, effects, tags, audio, and montage state"),
		Source.Contains(TEXT("RemoveCharacterAbilities")) && Source.Contains(TEXT("SetCollisionEnabled(ECollisionEnabled::NoCollision)"))
		&& Source.Contains(TEXT("GravityScale = 0")) && Source.Contains(TEXT("Velocity = FVector(0)"))
		&& Source.Contains(TEXT("OnCharacterDied.Broadcast(this)")) && Source.Contains(TEXT("CancelAllAbilities"))
		&& Source.Contains(TEXT("RemoveActiveEffectsWithTags")) && Source.Contains(TEXT("AddLooseGameplayTag(DeadTag)"))
		&& Source.Contains(TEXT("PlaySoundAtLocation")) && Source.Contains(TEXT("PlayAnimMontage(DeathMontage)")) && Source.Contains(TEXT("FinishDying()")));
	TestTrue(TEXT("Default attributes must apply a sourced outgoing gameplay effect to self"),
		Source.Contains(TEXT("MakeEffectContext")) && Source.Contains(TEXT("EffectContext.AddSourceObject(this)"))
		&& Source.Contains(TEXT("MakeOutgoingSpec(DefaultAttributes")) && Source.Contains(TEXT("ApplyGameplayEffectSpecToSelf")));
	TestTrue(TEXT("Startup effects must be authority-only one-shot effects applied to the ASC"),
		Source.Contains(TEXT("bStartupEffectsApplied")) && Source.Contains(TEXT("for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)"))
		&& Source.Contains(TEXT("MakeOutgoingSpec(GameplayEffect")) && Source.Contains(TEXT("ApplyGameplayEffectSpecToTarget"))
		&& Source.Contains(TEXT("AbilitySystemComponent->bStartupEffectsApplied = true")));
	TestTrue(TEXT("Damage number UI must queue, timer, create/register/attach components, set text, and drain the queue"),
		Source.Contains(TEXT("DamageNumberQueue.Add")) && Source.Contains(TEXT("SetTimer")) && Source.Contains(TEXT("ShowDamageNumber"))
		&& Source.Contains(TEXT("NewObject<UGSDamageTextWidgetComponent>")) && Source.Contains(TEXT("RegisterComponent"))
		&& Source.Contains(TEXT("AttachToComponent")) && Source.Contains(TEXT("SetDamageText"))
		&& Source.Contains(TEXT("ClearTimer")) && Source.Contains(TEXT("DamageNumberQueue.RemoveAt(0)")));
	TestTrue(TEXT("Attribute setters must update the attribute set"),
		Source.Contains(TEXT("AttributeSetBase->SetHealth")) && Source.Contains(TEXT("AttributeSetBase->SetMana"))
		&& Source.Contains(TEXT("AttributeSetBase->SetStamina")) && Source.Contains(TEXT("AttributeSetBase->SetShield")));

	return true;
}
