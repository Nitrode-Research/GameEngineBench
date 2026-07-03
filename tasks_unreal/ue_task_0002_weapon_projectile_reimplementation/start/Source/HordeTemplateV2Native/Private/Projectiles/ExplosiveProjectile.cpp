#include "ExplosiveProjectile.h"

AExplosiveProjectile::AExplosiveProjectile()
{
}

void AExplosiveProjectile::OnProjectileImpact(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
}

void AExplosiveProjectile::PlayWorldFX_Implementation(FVector Epicenter)
{
}

bool AExplosiveProjectile::PlayWorldFX_Validate(FVector Epicenter)
{
	return true;
}
