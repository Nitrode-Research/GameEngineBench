// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Characters/GSCharacterBase.h"
#include "Characters/Abilities/GSInteractable.h"
#include "GameplayEffectTypes.h"
#include "GSHeroCharacter.generated.h"

class AGSWeapon;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EGSHeroWeaponState : uint8
{
	Rifle					UMETA(DisplayName = "Rifle"),
	RifleAiming				UMETA(DisplayName = "Rifle Aiming"),
	RocketLauncher			UMETA(DisplayName = "Rocket Launcher"),
	RocketLauncherAiming	UMETA(DisplayName = "Rocket Launcher Aiming")
};

USTRUCT()
struct GASSHOOTER_API FGSHeroInventory
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<AGSWeapon*> Weapons;
};

UCLASS()
class GASSHOOTER_API AGSHeroCharacter : public AGSCharacterBase, public IGSInteractable
{
	GENERATED_BODY()

public:
	AGSHeroCharacter(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|GSHeroCharacter")
	bool bStartInFirstPersonPerspective;

	FGameplayTag CurrentWeaponTag;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	class UGSFloatingStatusBarWidget* GetFloatingStatusBar();

	virtual void KnockDown();
	virtual void PlayKnockDownEffects();
	virtual void PlayReviveEffects();
	virtual void FinishDying() override;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSHeroCharacter")
	virtual bool IsInFirstPersonPerspective() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter")
	USkeletalMeshComponent* GetFirstPersonMesh() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter")
	USkeletalMeshComponent* GetThirdPersonMesh() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	AGSWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	bool AddWeaponToInventory(AGSWeapon* NewWeapon, bool bEquipWeapon = false);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	bool RemoveWeaponFromInventory(AGSWeapon* WeaponToRemove);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	void RemoveAllWeaponsFromInventory();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	void EquipWeapon(AGSWeapon* NewWeapon);

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AGSWeapon* NewWeapon);
	void ServerEquipWeapon_Implementation(AGSWeapon* NewWeapon);
	bool ServerEquipWeapon_Validate(AGSWeapon* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	virtual void NextWeapon();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	virtual void PreviousWeapon();

	FName GetWeaponAttachPoint();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetMaxPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetPrimaryReserveAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetMaxSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetSecondaryReserveAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetNumWeapons() const;

	virtual bool IsAvailableForInteraction_Implementation(UPrimitiveComponent* InteractionComponent) const override;
	virtual float GetInteractionDuration_Implementation(UPrimitiveComponent* InteractionComponent) const override;
	virtual void PreInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent) override;
	virtual void PostInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent) override;
	virtual void GetPreInteractSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* InteractionComponent) const override;
	virtual void CancelInteraction_Implementation(UPrimitiveComponent* InteractionComponent) override;
	FSimpleMulticastDelegate* GetTargetCancelInteractionDelegate(UPrimitiveComponent* InteractionComponent) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	FVector StartingThirdPersonMeshLocation;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	FVector StartingFirstPersonMeshLocation;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|Abilities")
	float ReviveDuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|Camera")
	float BaseTurnRate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|Camera")
	float BaseLookUpRate;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float StartingThirdPersonCameraBoomArmLength;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	FVector StartingThirdPersonCameraBoomLocation;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	bool bIsFirstPersonPerspective;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	bool bWasInFirstPersonPerspectiveWhenKnockedDown;

	bool bASCInputBound;
	bool bChangedWeaponLocally;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float Default1PFOV;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float Default3PFOV;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|GSHeroCharacter")
	FName WeaponAttachPoint;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class USpringArmComponent* ThirdPersonCameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class UCameraComponent* ThirdPersonCamera;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class UCameraComponent* FirstPersonCamera;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSFloatingStatusBarWidget> UIFloatingStatusBarClass;

	UPROPERTY()
	class UGSFloatingStatusBarWidget* UIFloatingStatusBar;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|UI")
	class UWidgetComponent* UIFloatingStatusBarComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	FGSHeroInventory Inventory;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|Inventory")
	TArray<TSubclassOf<AGSWeapon>> DefaultInventoryWeaponClasses;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AGSWeapon* CurrentWeapon;

	UPROPERTY()
	class UGSAmmoAttributeSet* AmmoAttributeSet;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> KnockDownEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> ReviveEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> DeathEffect;

	FSimpleMulticastDelegate InteractionCanceledDelegate;

	FGameplayTag NoWeaponTag;
	FGameplayTag WeaponChangingDelayReplicationTag;
	FGameplayTag WeaponAmmoTypeNoneTag;
	FGameplayTag WeaponAbilityTag;
	FGameplayTag KnockedDownTag;
	FGameplayTag InteractingTag;

	FDelegateHandle PrimaryReserveAmmoChangedDelegateHandle;
	FDelegateHandle SecondaryReserveAmmoChangedDelegateHandle;
	FDelegateHandle WeaponChangingDelayReplicationTagChangedDelegateHandle;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

	void LookUp(float Value);
	void LookUpRate(float Value);
	void Turn(float Value);
	void TurnRate(float Value);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TogglePerspective();
	void SetPerspective(bool Is1PPerspective);

	UFUNCTION()
	void InitializeFloatingStatusBar();

	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	void BindASCInput();
	void SpawnDefaultInventory();
	void SetupStartupPerspective();
	bool DoesWeaponExistInInventory(AGSWeapon* InWeapon);
	void SetCurrentWeapon(AGSWeapon* NewWeapon, AGSWeapon* LastWeapon);
	void UnEquipWeapon(AGSWeapon* WeaponToUnEquip);
	void UnEquipCurrentWeapon();

	UFUNCTION()
	virtual void CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo);

	UFUNCTION()
	virtual void CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo);

	virtual void CurrentWeaponPrimaryReserveAmmoChanged(const FOnAttributeChangeData& Data);
	virtual void CurrentWeaponSecondaryReserveAmmoChanged(const FOnAttributeChangeData& Data);
	virtual void WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void OnRep_CurrentWeapon(AGSWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_Inventory();

	void OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags);

	UFUNCTION(Server, Reliable)
	void ServerSyncCurrentWeapon();
	void ServerSyncCurrentWeapon_Implementation();
	bool ServerSyncCurrentWeapon_Validate();

	UFUNCTION(Client, Reliable)
	void ClientSyncCurrentWeapon(AGSWeapon* InWeapon);
	void ClientSyncCurrentWeapon_Implementation(AGSWeapon* InWeapon);
	bool ClientSyncCurrentWeapon_Validate(AGSWeapon* InWeapon);
};
