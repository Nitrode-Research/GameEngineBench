#pragma once

#include "State/AlsxtCombatState.h"
#include "Settings/AlsxtCombatSettings.h"
#include "GameplayTagContainer.h"
#include "AlsxtCombatInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtCombatInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtCombatInterface {
	GENERATED_BODY()

public:
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
  FAlsxtGeneralCombatSettings GetGeneralCombatSettings();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  UAlsxtCombatSettings* SelectCombatSettings();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
  FAlsxtCombatAttackTraceSettings GetCombatAttackTraceSettings();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  FGameplayTagContainer GetTargetableOverlayModes();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Target Lock")
  void OnNewTarget(const FTargetHitResultEntry& NewTarget);

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
  bool CanDashToTarget();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
  bool ShouldDashToTarget();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  bool CanAttack();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  bool CanGrapple();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  bool CanThrow();  

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  bool CanPerformTakedown();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  bool CanPerformUniqueAttack();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  bool Attacking();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  AActor* GetCurrentTarget();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  FAlsxtCombatState GetCombatState();



  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Held Item Interface")
  void GetHeldItemTraceLocations(bool& Found, FVector& Start, FVector& End, float& Radius) const;


  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  void GetCombatUnarmedAttackDamageInfo(const FGameplayTag& UnarmedAttackType, const FGameplayTag& UnarmedAttackStrength, float& BaseDamage, FGameplayTag& ImpactForm, FGameplayTag& DamageType) const;

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  void GetCombatHeldItemAttackDamageInfo(const FGameplayTag& HeldItemAttackType, const FGameplayTag& HeldItemAttackStrength, float& BaseDamage, FGameplayTag& ImpactForm, FGameplayTag& DamageType) const;



  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  void GetCombatUnarmedTraceLocations(FGameplayTag UnarmedAttackType, FVector& Start, FVector& End, float& Radius) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  void GetCombatHeldItemTraceLocations(bool& Found, FVector& Start, FVector& End, float& Radius) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
  void BeginCombatAttackCollisionTrace(FAlsxtCombatAttackTraceSettings TraceSettings);

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
  void EndCombatAttackCollisionTrace();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  void AnticipateSyncedAttack(int32 Index);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  void SyncedAttackReaction(int32 Index);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat")
  void StopAttack();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  void GetUnarmedTraceLocations(const FGameplayTag& UnarmedAttackType, FVector& Start, FVector& End, float& Radius) const;

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
  float GetAttackBaseStaminaCost(UPARAM(meta = (Categories = "Als.Attack Method")) const FGameplayTag& AttackMethod, UPARAM(meta = (Categories = "Als.Action Strength")) const FGameplayTag& ActionStrength) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
  void OnAttackCollision(FAttackDoubleHitResult Hit);

};