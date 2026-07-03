#include "BaseProjectile.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetReplicates(true);
	InitialLifeSpan = 6.f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement Component"));

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	ProjectileMovement->UpdatedComponent = CollisionSphere;

	TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tracer Mesh"));
	TracerMesh->SetupAttachment(CollisionSphere);
	TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
}

void ABaseProjectile::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
}

void ABaseProjectile::SpawnImpactFX_Implementation(FVector ImpactLocation, FQuat ImpactRotation, TSubclassOf<ABaseImpact> ImpactClass)
{
}

bool ABaseProjectile::SpawnImpactFX_Validate(FVector ImpactLocation, FQuat ImpactRotation, TSubclassOf<ABaseImpact> ImpactClass)
{
	return true;
}
