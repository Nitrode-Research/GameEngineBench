// MIT

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "InputAction.h"
#include "AlsxtCharacter.h"
#include "AlsCharacter.h"
#include "AlsCameraComponent.h"
#include "Components/ActorComponent.h"
#include "State/AlsxtCombatState.h"
#include "AlsxtCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNewTargetSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashToTargetStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashToTargetEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttackStartedSignature, FGameplayTag, AttackType, FGameplayTag, Stance, FGameplayTag, Strength, float, BaseDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackCollisionSignature, FAttackDoubleHitResult, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackMissedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackBlockedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackParrySignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackRiposteSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEndedSignature);

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSXT_API UAlsxtCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAlsxtCombatComponent();

	UFUNCTION()
		virtual void SetupInputComponent(class UEnhancedInputComponent* PlayerInputComponent);
	//virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	UEnhancedInputComponent* EnhancedInputComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input", Meta = (AllowPrivateAccess))
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input", Meta = (AllowPrivateAccess))
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Input", Meta = (AllowPrivateAccess))
	TObjectPtr<UInputAction> BlockAction;

	UPROPERTY(BlueprintAssignable)
	FOnNewTargetSignature OnNewTarget;

	UPROPERTY(BlueprintAssignable)
	FOnDashToTargetStartedSignature OnDashToTargetStarted;

	UPROPERTY(BlueprintAssignable)
	FOnDashToTargetEndedSignature OnDashToTargetEnded;

	UPROPERTY(BlueprintAssignable)
	FOnAttackStartedSignature OnAttackStartedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttackCollisionSignature OnAttackCollisionDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnAttackEndedSignature OnAttackEndedDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnAttackMissedSignature OnAttackMissed;

	UPROPERTY(BlueprintAssignable)
	FOnAttackBlockedSignature OnAttackBlocked;

	UPROPERTY(BlueprintAssignable)
	FOnAttackParrySignature OnAttackParry;

	UPROPERTY(BlueprintAssignable)
	FOnAttackRiposteSignature OnAttackRiposte;

	// void OnAttackStarted(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage);
	// void OnAttackCollision(FAttackDoubleHitResult Hit);
	// void OnAttackEnded();


	virtual void InputPrimaryAction();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Parameters")
	UPARAM(meta = (Categories = "Als.Impact Form")) FGameplayTag GetCurrentAttackForm();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Parameters")
	FGameplayTag GetVelocityFromAttackStrength(UPARAM(meta = (Categories = "Als.Action Strength")) FGameplayTag AttackStrength);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "Character", Meta = (AllowPrivateAccess))
	AAlsxtCharacter* Character{ Cast<AAlsxtCharacter>(GetOwner()) };

	UPROPERTY(BlueprintReadOnly, Category = "Character", Meta = (AllowPrivateAccess))
	AAlsCharacter* AlsCharacter{ Cast<AAlsCharacter>(GetOwner()) };

	UPROPERTY(BlueprintReadOnly, Category = "Character", Meta = (AllowPrivateAccess))
	UAlsCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Lock", Meta = (AllowPrivateAccess))
	FTargetHitResultEntry CurrentTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtGeneralCombatSettings CombatSettings;

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	float GetAngle(FVector Target);

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void TryTraceForTargets();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void TraceForTargets(TArray<FTargetHitResultEntry>& Targets);

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	bool IsTartgetObstructed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Target Lock")
	void OnTargetObstructed();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void GetClosestTarget();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void SetCurrentTarget(const FTargetHitResultEntry& NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void ClearCurrentTarget();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void DisengageAllTargets();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void GetTargetLeft();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void GetTargetRight();

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void RotatePlayerToTarget(FTargetHitResultEntry Target);	

	UFUNCTION(BlueprintCallable, Category = "Target Lock")
	void DashToTarget();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	FAlsxtCombatState CombatState;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	AActor* TraceForPotentialAttackTarget(float Distance);

	UFUNCTION(BlueprintCallable, Category = "Attack", Meta = (AutoCreateRefTerm = "ActionType, AttackType, Strength"))
	void Attack(UPARAM(meta = (Categories = "Als.Action Type"))const FGameplayTag& ActionType, UPARAM(meta = (Categories = "Als.Attack Type"))const FGameplayTag& AttackType, UPARAM(meta = (Categories = "Als.Action Strength"))const FGameplayTag& Strength, float BaseDamage, float PlayRate = 1.0f);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	int32 CombatRootMotionSourceId;

public:
	FAlsxtCombatParameters CombatParameters;

	UFUNCTION(BlueprintCallable, Category = "ALS|Movement System")
	const FAlsxtCombatState& GetCombatState() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewCombatState"))
	void SetCombatState(const FAlsxtCombatState& NewCombatState);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewCombatState"))
	FAlsxtCombatState ProcessNewCombatState(const FAlsxtCombatState& NewCombatState);

	UFUNCTION(Server, Unreliable)
	void ServerProcessNewCombatState(const FAlsxtCombatState& NewCombatState);

private:
	UFUNCTION(Server, Unreliable)
	void ServerSetCombatState(const FAlsxtCombatState& NewCombatState);

	UFUNCTION()
	void OnReplicate_CombatState(const FAlsxtCombatState& PreviousCombatState);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnCombatStateChanged(const FAlsxtCombatState& PreviousCombatState);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Parameters")
	void DetermineAttackMethod(FGameplayTag& AttackMethod, const FGameplayTag& ActionType, const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float BaseDamage, const AActor* Target);

	UFUNCTION(BlueprintNativeEvent, Category = "Parameters")
	FAttackAnimation SelectAttackMontage(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, float BaseDamage);

	UFUNCTION(BlueprintNativeEvent, Category = "Parameters")
	FSyncedAttackAnimation SelectSyncedAttackMontage(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, float BaseDamage, int& Index);

	UFUNCTION(BlueprintNativeEvent, Category = "Parameters")
	FAnticipationPose SelectBlockingkMontage(const FGameplayTag& Strength, const FGameplayTag& Side, const FGameplayTag& Form, const FGameplayTag& Health);	

	UFUNCTION(BlueprintNativeEvent, Category = "Parameters")
	FSyncedActionAnimation GetSyncedAttackMontage(int32 Index);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Hooks")
	void OnSyncedAttackStarted(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const float& BaseDamage);

	// Desired Attack

private:
	bool IsAttackAllowedToStart(const UAnimMontage* Montage) const;
	
	void StartAttack(const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, float BaseDamage, float PlayRate, float TargetYawAngle);

	void StartSyncedAttack(const FGameplayTag& Overlay, const FGameplayTag& AttackType, const FGameplayTag& Stance, const FGameplayTag& Strength, const FGameplayTag& AttackMode, const float BaseDamage, const float PlayRate, const float TargetYawAngle, int Index);

	FTimerHandle TargetTraceTimerHandle;
	FTimerDelegate TargetTraceTimerDelegate;

	// Move To Target Timer
	FTimerHandle MoveToTargetTimerHandle;	// Timer Handle for MoveToTarget
	FTimerDelegate MoveToTargetTimerDelegate; // Delegate to bind function with parameters

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Meta = (AllowPrivateAccess))
	FAlsxtCombatAttackTraceSettings CurrentAttackTraceSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Meta = (AllowPrivateAccess))
	TArray<AActor*> AttackTraceLastHitActors;

protected:

	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void BeginMoveToTarget();

	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void UpdateMoveToTarget();

	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void EndMoveToTarget();

	// Attack Trace Timer
	FTimerHandle AttackTraceTimerHandle;	// Timer Handle for Attack Trace
	FTimerDelegate AttackTraceTimerDelegate; // Delegate to bind function with parameters

public:
	// Attack Collision Trace
	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void BeginAttackCollisionTrace(FAlsxtCombatAttackTraceSettings TraceSettings);

	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void AttackCollisionTrace();

	UFUNCTION(BlueprintCallable, Category = "ALSXT|Combat")
	void EndAttackCollisionTrace();

private:

	// UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	// void OnAttackCollision(FAttackDoubleHitResult Hit);

	TArray<UMaterialInstanceDynamic*> TargetDynamicMaterials;

	FTimerHandle LastTargetsTimerHandle;

	TArray<FLastTargetEntry> LastSeenTargets;

	TArray<FLastTargetEntry> LastTargets;

	FAttackAnimation LastAttackAnimation;

	FSyncedAttackAnimation LastSyncedAttackAnimation;

	FTimerHandle TimeSinceLastBlockTimerHandle;
	float TimeSinceLastBlock;

	FTimerHandle ConsecutiveHitsTimerHandle;
	int ConsecutiveHits;

	UFUNCTION(Server, Reliable)
	void ServerStartAttack(UAnimMontage* Montage, float PlayRate, float StartYawAngle, float TargetYawAngle);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartAttack(UAnimMontage* Montage, float PlayRate, float StartYawAngle, float TargetYawAngle);

	void StartAttackImplementation(UAnimMontage* Montage, float PlayRate, float StartYawAngle, float TargetYawAngle);

	void RefreshAttack(float DeltaTime);

	void RefreshAttackPhysics(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerStartSyncedAttack(UAnimMontage* Montage, int32 Index, float PlayRate, float StartYawAngle, float TargetYawAngle);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartSyncedAttack(UAnimMontage* Montage, int32 Index, float PlayRate, float StartYawAngle, float TargetYawAngle);

	void StartSyncedAttackImplementation(UAnimMontage* Montage, int32 Index, float PlayRate, float StartYawAngle, float TargetYawAngle);

	void RefreshSyncedAttack(float DeltaTime);

	void RefreshSyncedAttackPhysics(float DeltaTime);

public:
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void StopAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void StopSyncedAttack();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Hooks")
	void OnSyncedAttackEnded();
};

inline const FAlsxtCombatState& UAlsxtCombatComponent::GetCombatState() const
{
	return CombatState;
}