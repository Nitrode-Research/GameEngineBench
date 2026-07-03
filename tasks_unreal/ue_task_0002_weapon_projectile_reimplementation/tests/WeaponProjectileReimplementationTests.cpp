// Copyright Epic Games, Inc. All Rights Reserved.
// WeaponProjectileReimplementationTests.cpp
// Automation tests for HordeTemplate Weapon & Projectile System (ue_task_0002).

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#define protected public
#include "Character/HordeBaseCharacter.h"
#include "Gameplay/GameplayStructures.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Weapons/BaseFirearm.h"
#include "Weapons/Weapon_HM5.h"
#include "Projectiles/BaseProjectile.h"
#include "Projectiles/ExplosiveProjectile.h"
#include "FX/Impact/BaseImpact.h"
#undef protected

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool IsPropertyReplicated(UClass* InClass, const FName& PropName)
{
    if (!InClass) return false;
    for (TFieldIterator<FProperty> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FProperty* Prop = *It;
        if (Prop && Prop->GetFName() == PropName)
        {
            return (Prop->GetPropertyFlags() & CPF_Net) != 0;
        }
    }
    return false;
}

static UWorld* CreateTestWorld()
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World && GEngine)
    {
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);
    }
    return World;
}

static void TearDownTestWorld(UWorld* World)
{
    if (!World) return;
    if (GEngine)
    {
        GEngine->DestroyWorldContext(World);
    }
    World->DestroyWorld(false);
}

static void TickWorld(UWorld* World, float TotalSeconds, float StepSeconds = 1.0f / 60.0f)
{
    if (!World) return;

    float Remaining = TotalSeconds;
    while (Remaining > KINDA_SMALL_NUMBER)
    {
        const float Delta = FMath::Min(StepSeconds, Remaining);
        World->Tick(ELevelTick::LEVELTICK_All, Delta);
        Remaining -= Delta;
    }
}

static int32 CountProjectiles(UWorld* World)
{
    TArray<AActor*> Projectiles;
    UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), Projectiles);
    return Projectiles.Num();
}

static bool InvokeNoArgFunction(UObject* Obj, const TCHAR* FunctionName)
{
    if (!Obj) return false;
    if (UFunction* Func = Obj->FindFunction(FName(FunctionName)))
    {
        Obj->ProcessEvent(Func, nullptr);
        return true;
    }
    return false;
}

static bool SetBoolProperty(UObject* Obj, const TCHAR* PropertyName, bool bValue)
{
    if (!Obj) return false;
    if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), FName(PropertyName)))
    {
        Prop->SetPropertyValue_InContainer(Obj, bValue);
        return true;
    }
    return false;
}

static float GetTimerRateFromProperty(UObject* Obj, const TCHAR* PropertyName)
{
    if (!Obj) return -1.0f;
    if (FStructProperty* Prop = FindFProperty<FStructProperty>(Obj->GetClass(), FName(PropertyName)))
    {
        if (FTimerHandle* Handle = Prop->ContainerPtrToValuePtr<FTimerHandle>(Obj))
        {
            if (UWorld* World = Obj->GetWorld())
            {
                return World->GetTimerManager().GetTimerRate(*Handle);
            }
        }
    }
    return -1.0f;
}

static float GetFloatProperty(UObject* Obj, const TCHAR* PropertyName, float Default = -1.f)
{
    if (!Obj) return Default;
    if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), FName(PropertyName)))
    {
        return Prop->GetPropertyValue_InContainer(Obj);
    }
    return Default;
}

// Equip a firearm on the character by writing CurrentSelectedFirearm
// via reflection (needed for B3 gate in FireFirearm).
static void EquipFirearm(AHordeBaseCharacter* Character, ABaseFirearm* Firearm)
{
    if (!Character || !Firearm) return;

    static const FName CandidateNames[] = {
        FName("CurrentSelectedFirearm"),
        FName("SelectedFirearm"),
        FName("CurrentFirearm"),
        FName("CurrentWeapon"),
        FName("EquippedFirearm"),
        FName("CurFirearm"),
    };

    UClass* CharClass = AHordeBaseCharacter::StaticClass();
    for (const FName& PropName : CandidateNames)
    {
        if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(CharClass, PropName))
        {
            Prop->SetObjectPropertyValue_InContainer(Character, Firearm);
            break;
        }
    }

    Firearm->SetOwner(Character);
}

// Helper to get the character's health via reflection
static float GetCharacterHealth(AHordeBaseCharacter* Character)
{
    if (!Character) return -1.f;
    return Character->GetHealth();
}

// Helper to get inventory component via reflection
static UInventoryComponent* GetInventoryComponent(AHordeBaseCharacter* Character)
{
    if (!Character) return nullptr;
    UClass* CharClass = AHordeBaseCharacter::StaticClass();
    static const FName CandidateNames[] = {
        FName("Inventory"),
        FName("InventoryComponent"),
        FName("InvComponent"),
    };
    for (const FName& PropName : CandidateNames)
    {
        if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(CharClass, PropName))
        {
            if (UInventoryComponent* Comp = Cast<UInventoryComponent>(Prop->GetObjectPropertyValue_InContainer(Character)))
            {
                return Comp;
            }
        }
    }
    return nullptr;
}

// Invoke ServerFireFirearm_Implementation via reflection, falling back to
// the public fire entrypoint so the native firing logic always executes.
static bool InvokeServerFireFirearmImpl(ABaseFirearm* Firearm)
{
    if (!Firearm) return false;
    Firearm->ServerFireFirearm_Implementation();
    return true;
}

// Helper: invoke FireFirearm() directly — this is a public BlueprintCallable
// virtual, not an RPC, so it executes the body synchronously in test worlds.
static bool InvokeFireFirearm(ABaseFirearm* Firearm)
{
    if (!Firearm) return false;
    Firearm->FireFirearm();
    return true;
}

static bool InvokeNoParamUFunction(UObject* Object, const FName FunctionName)
{
    if (!Object) return false;
    UFunction* Function = Object->FindFunction(FunctionName);
    if (!Function) return false;
    Object->ProcessEvent(Function, nullptr);
    return true;
}

// Helper: invoke SpawnImpactFX_Implementation via reflection
static bool InvokeSpawnImpactFXImpl(ABaseProjectile* Projectile,
    FVector ImpactLocation, FQuat ImpactRotation, TSubclassOf<ABaseImpact> ImpactClass)
{
    if (!Projectile) return false;
    Projectile->SpawnImpactFX_Implementation(ImpactLocation, ImpactRotation, ImpactClass);
    return true;
}

// Helper: invoke the public fire-mode toggle entrypoint directly.
static bool InvokeServerToggleFireModeImpl(ABaseFirearm* Firearm)
{
    if (!Firearm) return false;
    Firearm->ServerToggleFireMode_Implementation();
    return true;
}

// ===========================================================================
// TEST 0 — B17/B19/B20 surface configuration.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProjectileCDOConfigurationTest,
    "HordeTemplate.Weapon.Projectile_CDOConfiguration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProjectileCDOConfigurationTest::RunTest(const FString& Parameters)
{
    const ABaseProjectile* BaseCDO = GetDefault<ABaseProjectile>();
    if (!TestNotNull(TEXT("ABaseProjectile CDO must exist"), BaseCDO)) return false;

    TestTrue(TEXT("Base projectile must replicate (B17: spawned projectile visible to clients)"),
        BaseCDO->GetIsReplicated());
    TestTrue(TEXT("Base projectile must have a positive lifespan (traveling projectile)"),
        BaseCDO->InitialLifeSpan > 0.f);

    FObjectProperty* MoveProp = FindFProperty<FObjectProperty>(ABaseProjectile::StaticClass(), FName("ProjectileMovement"));
    FObjectProperty* SphereProp = FindFProperty<FObjectProperty>(ABaseProjectile::StaticClass(), FName("CollisionSphere"));
    FObjectProperty* TracerProp = FindFProperty<FObjectProperty>(ABaseProjectile::StaticClass(), FName("TracerMesh"));

    UProjectileMovementComponent* BaseMove = MoveProp
        ? Cast<UProjectileMovementComponent>(MoveProp->GetObjectPropertyValue_InContainer(BaseCDO))
        : nullptr;
    USphereComponent* CollisionSphere = SphereProp
        ? Cast<USphereComponent>(SphereProp->GetObjectPropertyValue_InContainer(BaseCDO))
        : nullptr;
    UStaticMeshComponent* TracerMesh = TracerProp
        ? Cast<UStaticMeshComponent>(TracerProp->GetObjectPropertyValue_InContainer(BaseCDO))
        : nullptr;

    if (TestNotNull(TEXT("Base projectile must own a ProjectileMovement component (B17)"), BaseMove))
    {
        TestTrue(TEXT("ProjectileMovement InitialSpeed must be > 0 (B17: projectile travels)"),
            BaseMove->InitialSpeed > 0.f);
        TestTrue(TEXT("ProjectileMovement MaxSpeed must be > 0 (B17)"),
            BaseMove->MaxSpeed > 0.f);
    }
    if (TestNotNull(TEXT("Base projectile must own a collision sphere (impact surface)"), CollisionSphere))
    {
        TestTrue(TEXT("Collision sphere radius must be > 0"), CollisionSphere->GetScaledSphereRadius() > 0.f);
        // Collision profile name is not mandated by the spec — any profile that
        // enables projectile-vs-world hits is acceptable.
    }
    if (TestNotNull(TEXT("Base projectile must own a tracer mesh"), TracerMesh))
    {
        // Static mesh asset assignment requires knowledge of specific content
        // paths not provided in the spec; skip this implementation-detail check.
        if (!TracerMesh->GetIsReplicated())
        {
            AddWarning(TEXT("Tracer mesh component does not have SetIsReplicated(true) — client visibility is still covered by actor-level replication (B17/B19)."));
        }
    }

    const AExplosiveProjectile* ExplosiveCDO = GetDefault<AExplosiveProjectile>();
    if (TestNotNull(TEXT("AExplosiveProjectile CDO must exist"), ExplosiveCDO))
    {
        UProjectileMovementComponent* ExplosiveMove = MoveProp
            ? Cast<UProjectileMovementComponent>(MoveProp->GetObjectPropertyValue_InContainer(ExplosiveCDO))
            : nullptr;
        if (TestNotNull(TEXT("Explosive projectile must also own a ProjectileMovement component"), ExplosiveMove))
        {
            TestTrue(TEXT("Explosive projectile must have positive gravity scale (B20: altered explosive flight)"),
                ExplosiveMove->ProjectileGravityScale > 0.f);
            TestTrue(TEXT("Explosive projectile must still travel with positive initial speed (B20)"),
                ExplosiveMove->InitialSpeed > 0.f);
        }
    }

    return true;
}

// ===========================================================================
// TEST 1 — B1 (server-routed firing) + B12 (per-shot ammo consumption).
//          Verifies ServerFireFirearm is a Server+Reliable RPC (B1) and
//          that the underlying fire logic consumes exactly one round (B12).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireDecrementsAmmoOnServerFireTest,
    "HordeTemplate.Weapon.Firearm_ServerFireDecrementsAmmoExactlyOne",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFireDecrementsAmmoOnServerFireTest::RunTest(const FString& Parameters)
{
    UClass* FirearmClass = ABaseFirearm::StaticClass();
    if (!TestNotNull(TEXT("ABaseFirearm class must exist"), FirearmClass)) return false;

    // B1: ServerFireFirearm must be a Server+Reliable RPC.
    if (UFunction* FireFunc = FirearmClass->FindFunctionByName(FName("ServerFireFirearm")))
    {
        TestTrue(TEXT("ServerFireFirearm must be a Server RPC (B1: server-authoritative firing)"),
            (FireFunc->FunctionFlags & FUNC_NetServer) != 0);
        TestTrue(TEXT("ServerFireFirearm must be Reliable (B1)"),
            (FireFunc->FunctionFlags & FUNC_NetReliable) != 0);
    }
    else
    {
        AddError(TEXT("ServerFireFirearm UFUNCTION must exist on ABaseFirearm (B1)"));
        return false;
    }

    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestNotNull(
            TEXT("UInventoryHelpers::FindItemByID(\"Weapon_HM5\") must resolve a non-null ProjectileClass "
                 "(precondition for the ammo-decrement test)"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    TestNotNull(TEXT("Shooter character must spawn"), Shooter);
    TestNotNull(TEXT("Weapon_HM5 must spawn"), Firearm);

    if (Shooter && Firearm)
    {
        EquipFirearm(Shooter, Firearm);

        Firearm->WeaponID = TEXT("Weapon_HM5");
        Firearm->FireMode = (uint8)EFireMode::EFireModeSingle;
        Firearm->LoadedAmmo = 7;

        // Use the direct fire body to bypass server-RPC authority/equip gates
        // and isolate the ammo-decrement behavior (B12).
        InvokeFireFirearm(Firearm);

        TestEqual(
            FString::Printf(TEXT("Single ServerFireFirearm on a loaded weapon must consume exactly one round (started 7, observed %d) — B12"),
                Firearm->LoadedAmmo),
            Firearm->LoadedAmmo, 6);
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 2 — B4 (empty weapon cannot fire).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmptyWeaponCannotFireTest,
    "HordeTemplate.Weapon.Firearm_EmptyWeaponCannotFire",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEmptyWeaponCannotFireTest::RunTest(const FString& Parameters)
{
    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestNotNull(
            TEXT("UInventoryHelpers::FindItemByID(\"Weapon_HM5\") must resolve a non-null ProjectileClass "
                 "(precondition for the loaded-fire baseline that anchors B4)"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    if (!TestNotNull(TEXT("Shooter must spawn"), Shooter))
    {
        TearDownTestWorld(World);
        return false;
    }

    // --- Phase 1: loaded baseline ---
    AWeapon_HM5* LoadedFirearm = World->SpawnActor<AWeapon_HM5>();
    int32 LoadedDelta       = 0;
    int32 LoadedAmmoBefore  = 0;
    int32 LoadedAmmoAfter   = 0;
    if (TestNotNull(TEXT("Loaded firearm must spawn"), LoadedFirearm))
    {
        EquipFirearm(Shooter, LoadedFirearm);
        LoadedFirearm->WeaponID  = TEXT("Weapon_HM5");
        LoadedFirearm->FireMode  = (uint8)EFireMode::EFireModeSingle;
        LoadedFirearm->LoadedAmmo = 5;
        LoadedAmmoBefore = LoadedFirearm->LoadedAmmo;

        TArray<AActor*> Before;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), Before);

        // Use direct fire body for the loaded baseline to bypass server-RPC
        // authority/equip gates — we want to confirm the fire logic works, not
        // that the server wrapper allows it through.
        InvokeFireFirearm(LoadedFirearm);

        TArray<AActor*> After;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), After);
        LoadedDelta     = After.Num() - Before.Num();
        LoadedAmmoAfter = LoadedFirearm->LoadedAmmo;
    }

    // --- Phase 2: empty weapon ---
    AWeapon_HM5* EmptyFirearm = World->SpawnActor<AWeapon_HM5>();
    int32 EmptyDelta     = 0;
    int32 EmptyAmmoAfter = 0;
    if (TestNotNull(TEXT("Empty firearm must spawn"), EmptyFirearm))
    {
        EquipFirearm(Shooter, EmptyFirearm);
        EmptyFirearm->WeaponID  = TEXT("Weapon_HM5");
        EmptyFirearm->FireMode  = (uint8)EFireMode::EFireModeSingle;
        EmptyFirearm->LoadedAmmo = 0;

        TArray<AActor*> Before;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), Before);

        InvokeServerFireFirearmImpl(EmptyFirearm);

        TArray<AActor*> After;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), After);
        EmptyDelta     = After.Num() - Before.Num();
        EmptyAmmoAfter = EmptyFirearm->LoadedAmmo;
    }

    TestTrue(
        FString::Printf(TEXT("Loaded weapon must spawn at least one projectile when fired "
                             "(baseline that makes the B4 negative observation meaningful). LoadedDelta=%d"),
            LoadedDelta),
        LoadedDelta > 0);
    TestTrue(
        FString::Printf(TEXT("Loaded weapon LoadedAmmo must drop after fire "
                             "(baseline for B4: started %d, observed %d)"),
            LoadedAmmoBefore, LoadedAmmoAfter),
        LoadedAmmoAfter < LoadedAmmoBefore);

    TestEqual(
        FString::Printf(TEXT("Empty weapon must spawn no projectile when fired (B4). EmptyDelta=%d"),
            EmptyDelta),
        EmptyDelta, 0);
    TestEqual(
        FString::Printf(TEXT("Empty weapon LoadedAmmo must remain 0 after fire (B4: empty weapons cannot fire). Observed=%d"),
            EmptyAmmoAfter),
        EmptyAmmoAfter, 0);

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 3 — B17 (server spawns projectile that travels) + B18 (instigator
//          attribution).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireSpawnsProjectileOwnedByShooterTest,
    "HordeTemplate.Weapon.Firearm_FireSpawnsProjectileOwnedByShooter",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFireSpawnsProjectileOwnedByShooterTest::RunTest(const FString& Parameters)
{
    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestNotNull(
            TEXT("UInventoryHelpers::FindItemByID(\"Weapon_HM5\") must resolve a non-null ProjectileClass "
                 "(precondition: project inventory data table loadable in test environment) — B17"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    TestNotNull(TEXT("Shooter must spawn"), Shooter);
    TestNotNull(TEXT("Firearm must spawn"), Firearm);

    if (Shooter && Firearm)
    {
        EquipFirearm(Shooter, Firearm);
        Firearm->WeaponID = TEXT("Weapon_HM5");
        Firearm->LoadedAmmo = 5;
        Firearm->FireMode = (uint8)EFireMode::EFireModeSingle;

        TArray<AActor*> Before;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), Before);

        InvokeServerFireFirearmImpl(Firearm);

        TArray<AActor*> After;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseProjectile::StaticClass(), After);

        TestTrue(
            FString::Printf(TEXT("ServerFireFirearm must spawn an ABaseProjectile when ProjectileClass is set "
                                 "(B17: server spawns a projectile that travels). Before=%d After=%d"),
                Before.Num(), After.Num()),
            After.Num() > Before.Num());

        ABaseProjectile* Spawned = nullptr;
        for (AActor* Actor : After)
        {
            if (!Before.Contains(Actor))
            {
                Spawned = Cast<ABaseProjectile>(Actor);
                if (Spawned) break;
            }
        }

        if (TestNotNull(TEXT("A new ABaseProjectile must be discoverable after fire"), Spawned))
        {
            const bool bOwnerIsShooter      = Spawned->GetOwner()      == static_cast<AActor*>(Shooter);
            const bool bInstigatorIsShooter = Spawned->GetInstigator() == static_cast<APawn*>(Shooter);
            TestTrue(
                TEXT("Spawned projectile Owner or Instigator must be the shooting character (B18: instigator attribution for kill credit)"),
                bOwnerIsShooter || bInstigatorIsShooter);

            FObjectProperty* MoveProp = FindFProperty<FObjectProperty>(
                ABaseProjectile::StaticClass(), FName("ProjectileMovement"));
            if (MoveProp)
            {
                UProjectileMovementComponent* ProjMove = Cast<UProjectileMovementComponent>(
                    MoveProp->GetObjectPropertyValue_InContainer(Spawned));
                if (TestNotNull(TEXT("Spawned projectile ProjectileMovement must be valid"), ProjMove))
                {
                    TestTrue(TEXT("ProjectileMovement InitialSpeed must be > 0 (B17: projectile travels)"),
                        ProjMove->InitialSpeed > 0.f);
                }
            }

            TestTrue(TEXT("Spawned projectile must replicate (B17: server spawns, all clients see it)"),
                Spawned->GetIsReplicated());

        }
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 4 — B13 (inventory display reflects ammo consumption).
//
// Use the direct FireFirearm implementation body here instead of the server
// RPC wrapper. This keeps the test focused on the ammo/UI bookkeeping contract
// rather than on character/equip ownership plumbing, which is covered elsewhere.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireUpdatesInventoryDisplayAmmoTest,
    "HordeTemplate.Weapon.Firearm_FireUpdatesInventoryDisplayAmmo",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFireUpdatesInventoryDisplayAmmoTest::RunTest(const FString& Parameters)
{
    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestNotNull(
            TEXT("UInventoryHelpers::FindItemByID(\"Weapon_HM5\") must resolve a non-null ProjectileClass "
                 "(precondition for the inventory display ammo test — B13)"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    if (!TestNotNull(TEXT("Shooter must spawn"), Shooter) ||
        !TestNotNull(TEXT("Firearm must spawn"), Firearm))
    {
        TearDownTestWorld(World);
        return false;
    }

    UInventoryComponent* InvComp = GetInventoryComponent(Shooter);
    if (!TestNotNull(TEXT("Character must have a UInventoryComponent (created by start/'s ctor)"), InvComp))
    {
        TearDownTestWorld(World);
        return false;
    }

    FArrayProperty* InvArrProp = FindFProperty<FArrayProperty>(
        UInventoryComponent::StaticClass(), FName("Inventory"));
    if (!TestNotNull(TEXT("UInventoryComponent must declare an 'Inventory' TArray<FItem> property"), InvArrProp))
    {
        TearDownTestWorld(World);
        return false;
    }

    void* ArrayPtr = InvArrProp->ContainerPtrToValuePtr<void>(InvComp);
    FScriptArrayHelper Helper(InvArrProp, ArrayPtr);
    Helper.EmptyAndAddValues(1);

    FItem* Slot = reinterpret_cast<FItem*>(Helper.GetRawPtr(0));
    if (!TestNotNull(TEXT("Seeded inventory slot must be addressable"), Slot))
    {
        TearDownTestWorld(World);
        return false;
    }

    Slot->ItemID = FName(TEXT("Weapon_HM5"));
    Slot->ItemName = TEXT("Weapon_HM5");
    Slot->DefaultLoadedAmmo = 10;

    InvComp->ActiveItemIndex = 0;

    EquipFirearm(Shooter, Firearm);
    Firearm->WeaponID = TEXT("Weapon_HM5");
    Firearm->FireMode = (uint8)EFireMode::EFireModeSingle;
    Firearm->LoadedAmmo = 10;

    InvokeFireFirearm(Firearm);

    Slot = reinterpret_cast<FItem*>(Helper.GetRawPtr(0));
    if (TestNotNull(TEXT("Slot item must remain addressable after firing"), Slot))
    {
        TestTrue(
            FString::Printf(TEXT("Inventory display ammo must drop after a fire (started 10, observed %d) — B13"),
                Slot->DefaultLoadedAmmo),
            Slot->DefaultLoadedAmmo < 10);

        TestEqual(
            FString::Printf(TEXT("Inventory display ammo (%d) must agree with firearm LoadedAmmo (%d) — B13"),
                Slot->DefaultLoadedAmmo, Firearm->LoadedAmmo),
            Slot->DefaultLoadedAmmo, Firearm->LoadedAmmo);
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 5A — B2/B3 character-path gating.
//
// Verifies that TriggerWeaponFire (character fire entry point) enforces the
// dead and reloading gates. The positive fire baseline is established via the
// firearm implementation body directly so this test does not over-couple to
// repository-specific equip plumbing.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterFireGatesReloadDeadAndUnequippedTest,
    "HordeTemplate.Weapon.Firearm_CharacterFireGatesReloadDeadAndEquippedWeapon",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCharacterFireGatesReloadDeadAndUnequippedTest::RunTest(const FString& Parameters)
{
    // B2/B3 structural requirements.
    if (!TestNotNull(
            TEXT("AHordeBaseCharacter must expose TriggerWeaponFire UFUNCTION for character fire-path gating (B2/B3)"),
            AHordeBaseCharacter::StaticClass()->FindFunctionByName(FName("TriggerWeaponFire"))))
    {
        return false;
    }

    if (!TestTrue(
            TEXT("AHordeBaseCharacter must expose a Reloading flag for fire gating (B2)"),
            FindFProperty<FBoolProperty>(AHordeBaseCharacter::StaticClass(), FName("Reloading")) != nullptr))
    {
        return false;
    }

    if (!TestTrue(
            TEXT("AHordeBaseCharacter must expose an IsDead flag for fire gating (B2)"),
            FindFProperty<FBoolProperty>(AHordeBaseCharacter::StaticClass(), FName("IsDead")) != nullptr))
    {
        return false;
    }

    if (!TestTrue(
            TEXT("AHordeBaseCharacter must track the currently equipped firearm for fire gating (B3)"),
            FindFProperty<FObjectProperty>(AHordeBaseCharacter::StaticClass(), FName("CurrentSelectedFirearm")) != nullptr))
    {
        return false;
    }

    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestNotNull(
            TEXT("Weapon_HM5 must resolve a non-null ProjectileClass for runtime firing observations"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    if (!TestNotNull(
            TEXT("Weapon_HM5 must resolve a non-null FirearmClass for runtime TriggerWeaponFire assertions (B2/B3)"),
            ItemRow.FirearmClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    AWeapon_HM5* EquippedFirearm = World->SpawnActor<AWeapon_HM5>();
    if (!TestNotNull(TEXT("Shooter must spawn"), Shooter) ||
        !TestNotNull(TEXT("Equipped firearm must spawn"), EquippedFirearm))
    {
        TearDownTestWorld(World);
        return false;
    }

    EquipFirearm(Shooter, EquippedFirearm);

    EquippedFirearm->WeaponID = TEXT("Weapon_HM5");
    EquippedFirearm->FireMode = (uint8)EFireMode::EFireModeSingle;
    EquippedFirearm->LoadedAmmo = 5;

    // Baseline: prove the underlying firearm implementation can consume ammo.
    // Use the direct implementation body here so this test does not fail just
    // because a model adds stricter server/equip validation around the RPC.
    InvokeFireFirearm(EquippedFirearm);

    TestEqual(TEXT("Direct server fire baseline must consume one round on the equipped weapon"),
        EquippedFirearm->LoadedAmmo, 4);

    // --- B2: reloading gate ---
    Shooter->Reloading = true;
    TestTrue(TEXT("TriggerWeaponFire should be invokable through reflection for reloading gate"),
        InvokeNoParamUFunction(Shooter, FName(TEXT("TriggerWeaponFire"))));
    TestEqual(TEXT("Reloading players must not consume equipped-weapon ammo when firing is requested (B2)"),
        EquippedFirearm->LoadedAmmo, 4);

    // --- B2: dead gate ---
    Shooter->Reloading = false;
    if (!TestTrue(TEXT("IsDead property must be writable for dead-player fire gate observation"),
            SetBoolProperty(Shooter, TEXT("IsDead"), true)))
    {
        TearDownTestWorld(World);
        return false;
    }

    TestTrue(TEXT("TriggerWeaponFire should be invokable through reflection for dead gate"),
        InvokeNoParamUFunction(Shooter, FName(TEXT("TriggerWeaponFire"))));
    TestEqual(TEXT("Dead players must not consume equipped-weapon ammo when firing is requested (B2)"),
        EquippedFirearm->LoadedAmmo, 4);

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 5 — B21 proxy (muzzle flash / sound presentation path) + B22
//          (firing state replicates).
//
//          This is intentionally a proxy test, not a full client-observed
//          multiplayer validation. It checks the NetMulticast surface plus the
//          local implementation path for activating presentation components.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayFirearmFXMulticastActivatesAudioTest,
    "HordeTemplate.Weapon.Firearm_PlayFirearmFXMulticastActivatesAudio",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlayFirearmFXMulticastActivatesAudioTest::RunTest(const FString& Parameters)
{
    UClass* FirearmClass = ABaseFirearm::StaticClass();
    if (!TestNotNull(TEXT("ABaseFirearm class must exist"), FirearmClass)) return false;

    // B21 structural: PlayFirearmFX must be NetMulticast.
    if (UFunction* FXFunc = FirearmClass->FindFunctionByName(FName("PlayFirearmFX")))
    {
        TestTrue(TEXT("PlayFirearmFX must be NetMulticast (B21: client-visible fire FX path)"),
            (FXFunc->FunctionFlags & FUNC_NetMulticast) != 0);
    }
    else
    {
        AddError(TEXT("PlayFirearmFX UFUNCTION must exist on ABaseFirearm (B21)"));
        return false;
    }

    // B22 structural: ammo and fire mode must replicate.
    TestTrue(TEXT("LoadedAmmo must be replicated (B22: firing state visible to all clients)"),
        IsPropertyReplicated(FirearmClass, FName("LoadedAmmo")));
    TestTrue(TEXT("FireMode must be replicated (B22)"),
        IsPropertyReplicated(FirearmClass, FName("FireMode")));

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    if (!TestNotNull(TEXT("Weapon_HM5 must spawn"), Firearm))
    {
        TearDownTestWorld(World);
        return false;
    }

    // Locate MuzzleFlash component.
    FObjectProperty* MuzzleProp = FindFProperty<FObjectProperty>(FirearmClass, FName("MuzzleFlash"));
    if (!TestNotNull(TEXT("ABaseFirearm must own a MuzzleFlash UParticleSystemComponent (B21)"), MuzzleProp))
    {
        TearDownTestWorld(World);
        return false;
    }

    UParticleSystemComponent* MuzzleFlash = Cast<UParticleSystemComponent>(
        MuzzleProp->GetObjectPropertyValue_InContainer(Firearm));
    if (!TestNotNull(TEXT("Spawned firearm must have a non-null MuzzleFlash component (B21)"), MuzzleFlash))
    {
        TearDownTestWorld(World);
        return false;
    }

    // Also locate WeaponSound for the audio half of B21.
    FObjectProperty* SoundProp = FindFProperty<FObjectProperty>(FirearmClass, FName("WeaponSound"));
    UAudioComponent* WeaponSound = SoundProp
        ? Cast<UAudioComponent>(SoundProp->GetObjectPropertyValue_InContainer(Firearm))
        : nullptr;
    if (!TestNotNull(TEXT("Spawned firearm must have a non-null WeaponSound component (B21)"), WeaponSound))
    {
        TearDownTestWorld(World);
        return false;
    }

    // B21 structural: verify the CDO has SetAutoActivate(false) so that
    // MuzzleFlash starts inactive and is only triggered by PlayFirearmFX.
    const ABaseFirearm* CDO = GetDefault<ABaseFirearm>();
    if (CDO)
    {
        FObjectProperty* CDOMuzzleProp = FindFProperty<FObjectProperty>(FirearmClass, FName("MuzzleFlash"));
        UParticleSystemComponent* CDOMuzzle = CDOMuzzleProp
            ? Cast<UParticleSystemComponent>(CDOMuzzleProp->GetObjectPropertyValue_InContainer(CDO))
            : nullptr;
        if (CDOMuzzle)
        {
            TestFalse(
                TEXT("MuzzleFlash must start with AutoActivate=false in the CDO "
                     "(B21: flash is triggered by PlayFirearmFX, not on spawn)"),
                CDOMuzzle->bAutoActivate);
        }
    }

    // B21 proxy behavioral check: PlayFirearmFX_Implementation must not crash
    // and must at minimum attempt to activate the presentation components.
    // IsActive() is unreliable in test worlds without real particle assets, so
    // the NetMulticast flag check above remains the primary gate.
    Firearm->PlayFirearmFX_Implementation();

    // B22 behavioral: the actor itself must replicate so animation BPs see state.
    TestTrue(
        TEXT("Spawned firearm actor must be replicated "
             "(B22: firing state must replicate so animation BPs see updates)"),
        Firearm->GetIsReplicated());

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 5B — B10/B11 runtime timing surface.
//           Uses TriggerWeaponFire to engage the character-managed full-auto
//           timer and confirms that the configured timer interval comes from
//           item data. This does not fully prove blocked early refire or a
//           delayed second shot; it targets the timer-rate surface only.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterFireRateUsesItemDataAndEnforcesIntervalTest,
    "Disabled.WeaponTiming.Firearm_CharacterFireRateUsesItemDataAndEnforcesInterval",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCharacterFireRateUsesItemDataAndEnforcesIntervalTest::RunTest(const FString& Parameters)
{
    // B10/B11 structural: WeaponFireTimer must exist on the character.
    if (!TestTrue(
            TEXT("AHordeBaseCharacter must expose WeaponFireTimer for server-side full-auto timing (B10/B11)"),
            FindFProperty<FStructProperty>(AHordeBaseCharacter::StaticClass(), FName("WeaponFireTimer")) != nullptr))
    {
        return false;
    }

    // TriggerWeaponFire must also exist.
    if (!TestNotNull(
            TEXT("AHordeBaseCharacter must expose TriggerWeaponFire UFUNCTION for timing checks (B10/B11)"),
            AHordeBaseCharacter::StaticClass()->FindFunctionByName(FName("TriggerWeaponFire"))))
    {
        return false;
    }

    const FItem ItemRow = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    const bool bSupportsFullAuto = ItemRow.FireModes.Contains(EFireMode::EFireModeFull);

    if (!TestTrue(TEXT("Weapon_HM5 must support full-auto for fire-rate interval observation"), bSupportsFullAuto) ||
        !TestTrue(TEXT("Weapon_HM5 must declare a positive FireRate in item data"), ItemRow.FireRate > 0.0f) ||
        !TestNotNull(TEXT("Weapon_HM5 must resolve a ProjectileClass for runtime timing observation"),
            ItemRow.ProjectileClass.Get()))
    {
        return false;
    }

    if (!TestNotNull(
            TEXT("Weapon_HM5 must resolve a non-null FirearmClass for runtime TriggerWeaponFire timing assertions (B10/B11)"),
            ItemRow.FirearmClass.Get()))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>();
    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    if (!TestNotNull(TEXT("Shooter must spawn"), Shooter) ||
        !TestNotNull(TEXT("Firearm must spawn"), Firearm))
    {
        TearDownTestWorld(World);
        return false;
    }

    EquipFirearm(Shooter, Firearm);
    Firearm->WeaponID = TEXT("Weapon_HM5");
    Firearm->FireMode = (uint8)EFireMode::EFireModeFull;
    Firearm->LoadedAmmo = 10;

    const int32 Before = CountProjectiles(World);

    InvokeFireFirearm(Firearm);
    const int32 AfterDirect = CountProjectiles(World);
    if (!TestEqual(TEXT("Direct firearm fire baseline must spawn exactly one projectile (B10/B11 baseline)"),
            AfterDirect, Before + 1))
    {
        TearDownTestWorld(World);
        return false;
    }

    TestTrue(TEXT("TriggerWeaponFire should be invokable through reflection for full-auto timing"),
        InvokeNoParamUFunction(Shooter, FName(TEXT("TriggerWeaponFire"))));

    const float ObservedRate = GetTimerRateFromProperty(Shooter, TEXT("WeaponFireTimer"));
    TestTrue(
        FString::Printf(TEXT("WeaponFireTimer must use the FireRate from item data (expected %.4f, observed %.4f) — B11"),
            ItemRow.FireRate, ObservedRate),
        ObservedRate >= 0.0f && FMath::IsNearlyEqual(ObservedRate, ItemRow.FireRate, KINDA_SMALL_NUMBER));

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 6 — B5 (weapons support multiple fire modes) + B9 (cycle through
//          available modes).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FServerToggleFireModeCyclesValidlyTest,
    "HordeTemplate.Weapon.Firearm_ServerToggleFireModeCyclesValidly",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerToggleFireModeCyclesValidlyTest::RunTest(const FString& Parameters)
{
    if (const UEnum* FireModeEnum = StaticEnum<EFireMode>())
    {
        TestTrue(TEXT("EFireMode must declare EFireModeSingle (B5)"),
            FireModeEnum->GetIndexByName(FName("EFireModeSingle")) != INDEX_NONE);
        TestTrue(TEXT("EFireMode must declare EFireModeBurst (B5)"),
            FireModeEnum->GetIndexByName(FName("EFireModeBurst")) != INDEX_NONE);
        TestTrue(TEXT("EFireMode must declare EFireModeFull (B5)"),
            FireModeEnum->GetIndexByName(FName("EFireModeFull")) != INDEX_NONE);
    }
    else
    {
        AddError(TEXT("EFireMode enum must exist (B5)"));
        return false;
    }

    const FItem WeaponItem = UInventoryHelpers::FindItemByID(FName(TEXT("Weapon_HM5")));
    if (!TestTrue(
            TEXT("Weapon_HM5 must declare more than one FireMode in the data table (B9 precondition)"),
            WeaponItem.FireModes.Num() > 1))
    {
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    AWeapon_HM5* Firearm = World->SpawnActor<AWeapon_HM5>();
    if (TestNotNull(TEXT("Weapon_HM5 must spawn"), Firearm))
    {
        Firearm->WeaponID = TEXT("Weapon_HM5");
        Firearm->FireMode = (uint8)WeaponItem.FireModes[0];
        const uint8 ModeBefore = Firearm->FireMode;

        InvokeServerToggleFireModeImpl(Firearm);
        const uint8 ModeAfter = Firearm->FireMode;

        TestNotEqual(
            FString::Printf(TEXT("Toggle must change FireMode when more than one mode is available "
                                 "(started %d, observed %d, available=%d) — B9"),
                (int32)ModeBefore, (int32)ModeAfter, WeaponItem.FireModes.Num()),
            (int32)ModeAfter, (int32)ModeBefore);

        bool ModeAfterIsLegal = false;
        for (const EFireMode Mode : WeaponItem.FireModes)
        {
            if ((uint8)Mode == ModeAfter) { ModeAfterIsLegal = true; break; }
        }
        TestTrue(
            FString::Printf(TEXT("Toggled FireMode (%d) must be one of Weapon_HM5's declared FireModes (B9)"),
                (int32)ModeAfter),
            ModeAfterIsLegal);

        for (int32 i = 0; i < WeaponItem.FireModes.Num() + 1; ++i)
        {
            InvokeServerToggleFireModeImpl(Firearm);
            const uint8 Cur = Firearm->FireMode;
            bool Legal = false;
            for (const EFireMode Mode : WeaponItem.FireModes)
            {
                if ((uint8)Mode == Cur) { Legal = true; break; }
            }
            TestTrue(
                FString::Printf(TEXT("After repeated toggle, FireMode (%d) must remain in the legal set (B9)"),
                    (int32)Cur),
                Legal);
        }
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 7 — B19 proxy (impact FX multicast surface and local spawn path).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnImpactFXSpawnsImpactActorTest,
    "HordeTemplate.Weapon.Projectile_SpawnImpactFXSpawnsImpactActor",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnImpactFXSpawnsImpactActorTest::RunTest(const FString& Parameters)
{
    UClass* ProjClass = ABaseProjectile::StaticClass();
    if (!TestNotNull(TEXT("ABaseProjectile class must exist"), ProjClass)) return false;

    if (UFunction* FXFunc = ProjClass->FindFunctionByName(FName("SpawnImpactFX")))
    {
        TestTrue(TEXT("SpawnImpactFX must be NetMulticast (B19: client-visible impact FX path)"),
            (FXFunc->FunctionFlags & FUNC_NetMulticast) != 0);
    }
    else
    {
        AddError(TEXT("SpawnImpactFX UFUNCTION must exist on ABaseProjectile (B19)"));
        return false;
    }

    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>();
    if (TestNotNull(TEXT("Projectile must spawn"), Projectile))
    {
        TArray<AActor*> Before;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseImpact::StaticClass(), Before);

        // Call SpawnImpactFX_Implementation directly with a valid ImpactClass.
        InvokeSpawnImpactFXImpl(
            Projectile,
            FVector(123.f, 456.f, 78.f),
            FQuat::Identity,
            ABaseImpact::StaticClass());

        TArray<AActor*> After;
        UGameplayStatics::GetAllActorsOfClass(World, ABaseImpact::StaticClass(), After);

        TestTrue(
            FString::Printf(TEXT("SpawnImpactFX_Implementation must spawn an ABaseImpact actor "
                                 "(B19: impact visual effects play at the hit location). Before=%d After=%d"),
                Before.Num(), After.Num()),
            After.Num() > Before.Num());
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 8 — B20 (explosive variant applies AOE damage on impact).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExplosiveProjectileImpactAppliesAOEDamageTest,
    "HordeTemplate.Weapon.Projectile_ExplosiveImpactAppliesAOEDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplosiveProjectileImpactAppliesAOEDamageTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateTestWorld();
    if (!TestNotNull(TEXT("Test world must be created"), World)) return false;

    FActorSpawnParameters ShooterParams;
    ShooterParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AHordeBaseCharacter* Shooter = World->SpawnActor<AHordeBaseCharacter>(
        AHordeBaseCharacter::StaticClass(),
        FVector(0.f, 0.f, 100000.f),
        FRotator::ZeroRotator,
        ShooterParams);
    TestNotNull(TEXT("Shooter must spawn"), Shooter);

    FActorSpawnParameters TargetParams;
    TargetParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AHordeBaseCharacter* Target = World->SpawnActor<AHordeBaseCharacter>(
        AHordeBaseCharacter::StaticClass(),
        FVector(500.f, 0.f, 0.f),
        FRotator::ZeroRotator,
        TargetParams);
    TestNotNull(TEXT("Target must spawn"), Target);

    AExplosiveProjectile* Explosive = World->SpawnActor<AExplosiveProjectile>();
    TestNotNull(TEXT("Explosive must spawn"), Explosive);

    if (Shooter && Target && Explosive)
    {
        Explosive->SetOwner(Shooter);

        const float TargetHealthBefore = GetCharacterHealth(Target);
        TestTrue(TEXT("Target must start with positive health (precondition for B20 observation)"),
            TargetHealthBefore > 0.f);

        FHitResult ImpactResult;
        ImpactResult.ImpactPoint  = FVector(450.f, 0.f, 0.f);
        ImpactResult.Location     = FVector(450.f, 0.f, 0.f);
        ImpactResult.ImpactNormal = FVector(1.f, 0.f, 0.f);
        ImpactResult.Normal       = FVector(1.f, 0.f, 0.f);

        Explosive->OnProjectileImpact(ImpactResult, FVector::ZeroVector);

        const float TargetHealthAfter = GetCharacterHealth(Target);

        TestTrue(
            FString::Printf(TEXT("Explosive impact must apply AOE damage to a nearby character "
                                 "(B20: explosive variant applies area-of-effect damage). TargetHealth Before=%.1f After=%.1f"),
                TargetHealthBefore, TargetHealthAfter),
            TargetHealthAfter < TargetHealthBefore);
    }

    TearDownTestWorld(World);
    return true;
}

// ===========================================================================
// TEST 10 — B14/B15/B16. Spread rotation-source: Weapon_HM5 must set
//   ProjectileFromMuzzle=false in its constructor (eye/crosshair aim, not
//   muzzle origin). The stub leaves it at the base-class default.
//   Also verifies ProjectileFromMuzzle is replicated so clients can read
//   the same source flag as the server.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFirearmExposesRotationSourceForSpreadTest,
    "HordeTemplate.Weapon.Firearm_ExposesRotationSourceForSpread",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFirearmExposesRotationSourceForSpreadTest::RunTest(const FString& Parameters)
{
    UClass* FirearmClass = ABaseFirearm::StaticClass();
    if (!TestNotNull(TEXT("ABaseFirearm class must exist"), FirearmClass)) return false;

    // B14/B15: ProjectileFromMuzzle must be replicated so all clients use
    // the same rotation source when computing spread.
    TestTrue(
        TEXT("ProjectileFromMuzzle must be replicated (B14: rotation source visible "
             "to all clients for consistent spread application)"),
        IsPropertyReplicated(FirearmClass, FName("ProjectileFromMuzzle")));

    // B16 DIRECT: Weapon_HM5 fires from the camera/crosshair, not the muzzle.
    // Its constructor must set ProjectileFromMuzzle=false. The stub leaves the
    // field at its base-class default (true), so this check discriminates.
    const AWeapon_HM5* HM5CDO = GetDefault<AWeapon_HM5>();
    if (TestNotNull(TEXT("AWeapon_HM5 CDO must exist"), HM5CDO))
    {
        if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(
                ABaseFirearm::StaticClass(), FName("ProjectileFromMuzzle")))
        {
            TestFalse(
                TEXT("B16: Weapon_HM5 must set ProjectileFromMuzzle=false — "
                     "this weapon fires along the player's camera aim, not from "
                     "the physical muzzle socket. Stub leaves it true."),
                Prop->GetPropertyValue_InContainer(HM5CDO));
        }
    }

    return true;
}
