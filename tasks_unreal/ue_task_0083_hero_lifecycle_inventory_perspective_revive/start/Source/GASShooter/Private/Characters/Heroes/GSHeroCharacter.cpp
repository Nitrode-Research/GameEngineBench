// Copyright 2020 Dan Kestranek.

#include "Characters/Heroes/GSHeroCharacter.h"

#include "AI/GSHeroAIController.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/GSWeapon.h"

AGSHeroCharacter::AGSHeroCharacter(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;
	bStartInFirstPersonPerspective = true;
	bIsFirstPersonPerspective = false;
	bWasInFirstPersonPerspectiveWhenKnockedDown = false;
	bASCInputBound = false;
	bChangedWeaponLocally = false;
	Default1PFOV = 90.0f;
	Default3PFOV = 80.0f;
	CurrentWeaponTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Equipped.None"));
	Inventory = FGSHeroInventory();
	ReviveDuration = 4.0f;
	UIFloatingStatusBar = nullptr;
	CurrentWeapon = nullptr;
	AmmoAttributeSet = nullptr;

	ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(FName("CameraBoom"));
	ThirdPersonCameraBoom->SetupAttachment(RootComponent);
	ThirdPersonCameraBoom->bUsePawnControlRotation = true;
	ThirdPersonCameraBoom->SetRelativeLocation(FVector(0, 50, 68.492264));

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(FName("FollowCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom);
	ThirdPersonCamera->FieldOfView = Default3PFOV;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(FName("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(RootComponent);
	FirstPersonCamera->bUsePawnControlRotation = true;

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->bReceivesDecals = false;
	FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetVisibility(false, true);

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(FName("NoCollision"));
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->bReceivesDecals = false;

	UIFloatingStatusBarComponent = CreateDefaultSubobject<UWidgetComponent>(FName("UIFloatingStatusBarComponent"));
	UIFloatingStatusBarComponent->SetupAttachment(RootComponent);
	UIFloatingStatusBarComponent->SetRelativeLocation(FVector(0, 0, 120));
	UIFloatingStatusBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	UIFloatingStatusBarComponent->SetDrawSize(FVector2D(500, 500));

	AutoPossessAI = EAutoPossessAI::PlacedInWorld;
	AIControllerClass = AGSHeroAIController::StaticClass();
}

void AGSHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSHeroCharacter, Inventory);
	DOREPLIFETIME_CONDITION(AGSHeroCharacter, CurrentWeapon, COND_SimulatedOnly);
}

void AGSHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AGSHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

UGSFloatingStatusBarWidget* AGSHeroCharacter::GetFloatingStatusBar()
{
	return UIFloatingStatusBar;
}

void AGSHeroCharacter::KnockDown()
{
}

void AGSHeroCharacter::PlayKnockDownEffects()
{
}

void AGSHeroCharacter::PlayReviveEffects()
{
}

void AGSHeroCharacter::FinishDying()
{
	Super::FinishDying();
}

bool AGSHeroCharacter::IsInFirstPersonPerspective() const
{
	return bIsFirstPersonPerspective;
}

USkeletalMeshComponent* AGSHeroCharacter::GetFirstPersonMesh() const
{
	return FirstPersonMesh;
}

USkeletalMeshComponent* AGSHeroCharacter::GetThirdPersonMesh() const
{
	return GetMesh();
}

AGSWeapon* AGSHeroCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

bool AGSHeroCharacter::AddWeaponToInventory(AGSWeapon* NewWeapon, bool bEquipWeapon)
{
	return false;
}

bool AGSHeroCharacter::RemoveWeaponFromInventory(AGSWeapon* WeaponToRemove)
{
	return false;
}

void AGSHeroCharacter::RemoveAllWeaponsFromInventory()
{
}

void AGSHeroCharacter::EquipWeapon(AGSWeapon* NewWeapon)
{
}

void AGSHeroCharacter::ServerEquipWeapon_Implementation(AGSWeapon* NewWeapon)
{
}

bool AGSHeroCharacter::ServerEquipWeapon_Validate(AGSWeapon* NewWeapon)
{
	return true;
}

void AGSHeroCharacter::NextWeapon()
{
}

void AGSHeroCharacter::PreviousWeapon()
{
}

FName AGSHeroCharacter::GetWeaponAttachPoint()
{
	return WeaponAttachPoint;
}

int32 AGSHeroCharacter::GetPrimaryClipAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetMaxPrimaryClipAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetPrimaryReserveAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetSecondaryClipAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetMaxSecondaryClipAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetSecondaryReserveAmmo() const
{
	return 0;
}

int32 AGSHeroCharacter::GetNumWeapons() const
{
	return Inventory.Weapons.Num();
}

bool AGSHeroCharacter::IsAvailableForInteraction_Implementation(UPrimitiveComponent* InteractionComponent) const
{
	return false;
}

float AGSHeroCharacter::GetInteractionDuration_Implementation(UPrimitiveComponent* InteractionComponent) const
{
	return 0.0f;
}

void AGSHeroCharacter::PreInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent)
{
}

void AGSHeroCharacter::PostInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent)
{
}

void AGSHeroCharacter::GetPreInteractSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* InteractionComponent) const
{
	bShouldSync = false;
	Type = EAbilityTaskNetSyncType::OnlyServerWait;
}

void AGSHeroCharacter::CancelInteraction_Implementation(UPrimitiveComponent* InteractionComponent)
{
}

FSimpleMulticastDelegate* AGSHeroCharacter::GetTargetCancelInteractionDelegate(UPrimitiveComponent* InteractionComponent)
{
	return &InteractionCanceledDelegate;
}

void AGSHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AGSHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AGSHeroCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AGSHeroCharacter::LookUp(float Value)
{
}

void AGSHeroCharacter::LookUpRate(float Value)
{
}

void AGSHeroCharacter::Turn(float Value)
{
}

void AGSHeroCharacter::TurnRate(float Value)
{
}

void AGSHeroCharacter::MoveForward(float Value)
{
}

void AGSHeroCharacter::MoveRight(float Value)
{
}

void AGSHeroCharacter::TogglePerspective()
{
}

void AGSHeroCharacter::SetPerspective(bool Is1PPerspective)
{
	bIsFirstPersonPerspective = Is1PPerspective;
}

void AGSHeroCharacter::InitializeFloatingStatusBar()
{
}

void AGSHeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void AGSHeroCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void AGSHeroCharacter::BindASCInput()
{
}

void AGSHeroCharacter::SpawnDefaultInventory()
{
}

void AGSHeroCharacter::SetupStartupPerspective()
{
}

bool AGSHeroCharacter::DoesWeaponExistInInventory(AGSWeapon* InWeapon)
{
	return false;
}

void AGSHeroCharacter::SetCurrentWeapon(AGSWeapon* NewWeapon, AGSWeapon* LastWeapon)
{
	CurrentWeapon = NewWeapon;
}

void AGSHeroCharacter::UnEquipWeapon(AGSWeapon* WeaponToUnEquip)
{
}

void AGSHeroCharacter::UnEquipCurrentWeapon()
{
}

void AGSHeroCharacter::CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo)
{
}

void AGSHeroCharacter::CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo)
{
}

void AGSHeroCharacter::CurrentWeaponPrimaryReserveAmmoChanged(const FOnAttributeChangeData& Data)
{
}

void AGSHeroCharacter::CurrentWeaponSecondaryReserveAmmoChanged(const FOnAttributeChangeData& Data)
{
}

void AGSHeroCharacter::WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
}

void AGSHeroCharacter::OnRep_CurrentWeapon(AGSWeapon* LastWeapon)
{
}

void AGSHeroCharacter::OnRep_Inventory()
{
}

void AGSHeroCharacter::OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags)
{
}

void AGSHeroCharacter::ServerSyncCurrentWeapon_Implementation()
{
}

bool AGSHeroCharacter::ServerSyncCurrentWeapon_Validate()
{
	return true;
}

void AGSHeroCharacter::ClientSyncCurrentWeapon_Implementation(AGSWeapon* InWeapon)
{
	CurrentWeapon = InWeapon;
}

bool AGSHeroCharacter::ClientSyncCurrentWeapon_Validate(AGSWeapon* InWeapon)
{
	return true;
}
